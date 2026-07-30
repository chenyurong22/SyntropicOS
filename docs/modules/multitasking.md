# Cooperative Multitasking

SyntropicOS provides a cooperative multitasking kernel built on protothreads — stackless coroutines where raw continuation state costs **2 bytes of RAM** (`uint16_t lc`), and full scheduled tasks with priority, delays, and event-blocking cost only **~16–28 bytes per task descriptor** (with zero per-task stack allocation).

## Protothreads

| Module | Header | Config |
|---|---|---|
| Protothreads | `pt/syn_pt.h` | `SYN_USE_PT` |
| Semaphores | `pt/syn_pt_sem.h` | `SYN_USE_PT` |

Protothreads are stackless cooperative coroutines implemented via the Duff's device trick (`switch`/`__LINE__` continuation). Each `SYN_PT` struct is a single `uint16_t`.

**Core macros:**

| Macro | Description |
|---|---|
| `PT_BEGIN(pt)` | Open a protothread body (must be first) |
| `PT_END(pt)` | Close and return `PT_EXITED` |
| `PT_WAIT_UNTIL(pt, cond)` | Block until condition is true |
| `PT_WAIT_WHILE(pt, cond)` | Block while condition is true |
| `PT_YIELD(pt)` | Yield control unconditionally |
| `PT_EXIT(pt)` | Terminate immediately |
| `PT_RESTART(pt)` | Reset and restart from the top |
| `PT_SPAWN(pt, child, func)` | Run a child protothread and block until it exits |
| `PT_DELAY_MS(pt, target, ms)` | Non-blocking delay (needs a `uint32_t*` for deadline storage) |
| `PT_TASK_DELAY_MS(pt, task, ms)` | Convenience form using `task->delay_until` |
| `PT_DEFER(pt, task)` | Defer to all ready tasks regardless of priority (one pass) |
| `PT_BLOCK_CONDITION(pt, task, cond)` | Block task execution (`SYN_TASK_BLOCKED`) until condition expression becomes true |
| `PT_BLOCK_EVENT(pt, task, grp, mask)` | Block task execution (`SYN_TASK_BLOCKED`) until event bit fires |

### Macro Expansion Under the Hood (Duff's Device)

Protothreads save execution position using a 2-byte line continuation variable (`uint16_t lc`) and C `switch`/`case` jump tables:

=== "High-Level C Code"
    ```c
    SYN_PT_Status blink_task(SYN_PT *pt)
    {
        PT_BEGIN(pt);
        while (1) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            PT_WAIT_UNTIL(pt, (syn_port_get_tick_ms() - last_tick) >= 500);
            last_tick = syn_port_get_tick_ms();
        }
        PT_END(pt);
    }
    ```

=== "Preprocessor Macro Expansion"
    ```c
    SYN_PT_Status blink_task(SYN_PT *pt)
    {
        char _pt_yield_flag = 1;
        switch (pt->lc) {
            case 0:
                while (1) {
                    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

                    /* PT_WAIT_UNTIL expansion */
                    pt->lc = 10; case 10:
                    if (!((syn_port_get_tick_ms() - last_tick) >= 500)) {
                        return PT_WAITING;
                    }

                    last_tick = syn_port_get_tick_ms();
                }
        }
        _pt_yield_flag = 0;
        pt->lc = 0;
        return PT_EXITED;
    }
    ```

### Writing Tasks: Rules and Gotchas

Protothreads use a `switch`/`__LINE__` continuation technique (sometimes
called Duff's device). This is invisible in normal use, but it creates two
constraints you must follow.

#### Rule 1: Local variables don't survive across yields or delays

When a task hits `PT_TASK_DELAY_MS()`, `PT_YIELD()`, or any `PT_WAIT_*`
macro, it **returns** from the function. When the scheduler calls it again,
execution resumes at the saved line — but local variables on the stack are
gone.

```c
// ❌ BAD: 'count' resets every time the task resumes
static SYN_PT_Status counter_task(SYN_PT *pt, SYN_Task *task)
{
    PT_BEGIN(pt);
    for (;;) {
        int count = 0;                          // Allocated on the stack
        count++;
        PT_TASK_DELAY_MS(pt, task, 1000);       // Task returns here → stack is gone
        // 'count' is undefined after this point
    }
    PT_END(pt);
}
```

**Fix:** Use `static` locals, file-scope globals, or `task->user_data`:

```c
// ✅ Option A: static local
static SYN_PT_Status counter_task(SYN_PT *pt, SYN_Task *task)
{
    PT_BEGIN(pt);
    static int count = 0;    // Lives in .bss, survives across yields
    for (;;) {
        count++;
        PT_TASK_DELAY_MS(pt, task, 1000);
    }
    PT_END(pt);
}

// ✅ Option B: user_data pointer (best for multiple task instances)
static SYN_PT_Status counter_task(SYN_PT *pt, SYN_Task *task)
{
    int *count = (int *)task->user_data;
    PT_BEGIN(pt);
    for (;;) {
        (*count)++;
        PT_TASK_DELAY_MS(pt, task, 1000);
    }
    PT_END(pt);
}
```

!!! warning "Local Variables & Resumption"
    Because protothreads are stackless and resume execution using a `switch` statement under the hood, **automatic local variables do not preserve their state across a yield**. 

    * **Across a Yield:** Any variable whose value must survive a yield (`PT_YIELD`, `PT_WAIT_UNTIL`, etc.) **must** be declared `static` or stored in a persistent structure (like `user_data`).
    * **Before `PT_BEGIN`:** Declaring a local variable before `PT_BEGIN` does **not** make it safe to use across a yield. Its declaration code will re-run on every invocation, resetting the variable to its initial value.
    * **Between Yields:** Local variables are only safe if their entire usage is self-contained **between** two consecutive yield points. To avoid compiler warnings about jumping over variable initializations, declare them inside a nested block `{ ... }` between the yields, or declare them before `PT_BEGIN` (if they do not need to persist across yields).

#### Rule 2: No `switch` statements inside a protothread body

Because `PT_BEGIN`/`PT_END` expand to a `switch`/`case` construct,
placing your own `switch` inside the protothread body produces nested
`switch` labels that confuse the compiler.

```c
// ❌ BAD: compiler error — nested switch conflicts with PT macros
static SYN_PT_Status my_task(SYN_PT *pt, SYN_Task *task)
{
    PT_BEGIN(pt);
    for (;;) {
        switch (mode) {           // Conflicts with PT_BEGIN's switch
            case 0: /* ... */ break;
            case 1: /* ... */ break;
        }
        PT_TASK_DELAY_MS(pt, task, 100);
    }
    PT_END(pt);
}
```

**Fix:** Extract the `switch` into a helper function, or use `if`/`else if`:

```c
// ✅ Option A: helper function
static void handle_mode(int mode) {
    switch (mode) {
        case 0: /* ... */ break;
        case 1: /* ... */ break;
    }
}

static SYN_PT_Status my_task(SYN_PT *pt, SYN_Task *task)
{
    PT_BEGIN(pt);
    for (;;) {
        handle_mode(mode);
        PT_TASK_DELAY_MS(pt, task, 100);
    }
    PT_END(pt);
}

// ✅ Option B: if/else if chain
static SYN_PT_Status my_task(SYN_PT *pt, SYN_Task *task)
{
    PT_BEGIN(pt);
    for (;;) {
        if (mode == 0)      { /* ... */ }
        else if (mode == 1) { /* ... */ }
        PT_TASK_DELAY_MS(pt, task, 100);
    }
    PT_END(pt);
}
```

#### Summary

| Constraint | Workaround |
|---|---|
| Local variables lost across yield/delay | Use `static`, globals, or `task->user_data` |
| No `switch` inside task body | Extract to a helper function, or use `if`/`else if` |
| Task must be a single function | Use `PT_SPAWN()` for subtask decomposition |

**Return values** (`SYN_PT_Status`):

- `PT_WAITING` — condition not met; scheduler marks the task `WAITING` for this tick and tries the next-best candidate (prevents starvation of lower-priority tasks)
- `PT_YIELDED` — voluntarily yielded
- `PT_EXITED` — ran to `PT_END`
- `PT_ENDED` — explicitly ended via `PT_EXIT`

## Scheduler

| Module | Header | Config |
|---|---|---|
| Task | `sched/syn_task.h` | `SYN_USE_SCHED` |
| Scheduler | `sched/syn_sched.h` | `SYN_USE_SCHED` |

The scheduler manages a caller-owned array of `SYN_Task` descriptors. Each call to `syn_sched_run()` selects and runs the **single highest-priority ready task** (0 = highest priority), with **per-priority round-robin** among equal-priority tasks.

```c
static SYN_Task tasks[3];
static SYN_Sched sched;

syn_task_create(&tasks[0], "blink",   blink_fn,   1, NULL);
syn_task_create(&tasks[1], "serial",  serial_fn,  0, NULL);
syn_task_create(&tasks[2], "monitor", monitor_fn, 2, NULL);

syn_sched_init(&sched, tasks, 3);
syn_sched_run_forever(&sched);  // never returns
```

### Priority and Round-Robin

The scheduler uses **strict priority** — the highest-priority ready task always runs first. Within a priority level, tasks rotate in **round-robin** order. Each priority level maintains its own independent rotation index, ensuring fair rotation even when tasks at different priorities interact.

The maximum number of priority levels defaults to 8 (priority 0–7) and is configurable:

```c
// syn_config.h
#define SYN_SCHED_PRIO_LEVELS 8  // default, increase if needed
```

### Deferring (`PT_DEFER`) & Starvation Prevention

In a cooperative scheduler with strict priority, understanding the exact behavior of yield operations is essential to prevent task starvation.

#### Execution Mechanism Comparison

| Macro | Task State Set | Round-Robin Effect | Can Lower Priority Run? |
|---|---|---|---|
| `PT_YIELD(pt)` | `SYN_TASK_READY` | Advances RR index within **same priority group** | ❌ **No.** If a higher priority task is ready, lower priorities are starved. |
| `PT_DEFER(pt, task)` | `SYN_TASK_DEFERRED` | **No RR advance.** Skipped for 1 pass; cleared to `READY` at end of tick | ✅ **Yes.** Guarantees 1 pass for lower priority tasks. |
| `PT_WAIT_UNTIL(pt, cond)` | `SYN_TASK_WAITING` (if false) | Re-scans next candidate **within the same tick** | ✅ **Yes.** Prevents waiting higher priority tasks from blocking work. |
| `PT_BLOCK_EVENT(pt, ...)` | `SYN_TASK_BLOCKED` | Removed from scan until event flag is set | ✅ **Yes.** Zero CPU overhead while waiting. |

#### Detailed Scenario: `PT_YIELD` vs `PT_DEFER`

Consider Task A (Priority 0) and Task B (Priority 1):

1. **Using `PT_YIELD` in Task A**:
   ```c
   // Task A (Prio 0)
   while (1) {
       check_sensor();
       PT_YIELD(pt); // Remains SYN_TASK_READY at Prio 0!
   }
   ```
   *Execution*: Scheduler scans Prio 0 → Task A is READY → Runs Task A → Task A yields → Scheduler scans Prio 0 again → Task A is READY → Runs Task A.
   *Result*: **Task B (Prio 1) NEVER runs (Starved).**

2. **Using `PT_DEFER` in Task A**:
   ```c
   // Task A (Prio 0)
   while (1) {
       check_sensor();
       PT_DEFER(pt, task); // Sets state to SYN_TASK_DEFERRED for 1 pass!
   }
   ```
   *Execution*: Scheduler scans Prio 0 → Task A is DEFERRED (skipped) → Scheduler scans Prio 1 → **Task B runs** → End of tick clears Task A back to READY → Next tick Task A runs again.
   *Result*: **Task B gets fair CPU execution time.**

    - Pass 1: Task A defers → Task B runs and defers.
    - Pass 2: Task A is cleared to READY → Task A runs and defers.
    - Result: Task A and Task B ping-pong deferrals every pass, keeping Priority 0 active continuously and **still starving Priority 1**.
    - *Fix*: Use `PT_TASK_DELAY_MS` or `PT_BLOCK_EVENT` when waiting for external timing or peripheral interrupts.


### True Task Blocking Primitives (`SYN_TASK_BLOCKED`)

Standard protothread `PT_WAIT_UNTIL` and `PT_WAIT_EVENT` use cooperative polling — the task stays `SYN_TASK_READY`, executes on every scheduler tick, checks its condition, and returns `SYN_PT_WAITING`. While functional, this wastes CPU cycles and prevents tickless sleep hardware low-power modes.

SyntropicOS provides **First-Class Native Task Blocking**. When a coroutine blocks using `PT_BLOCK_CONDITION` or a subsystem blocking primitive, its task state transitions to `SYN_TASK_BLOCKED`. The scheduler **skips the task entirely** on subsequent ticks (zero CPU overhead) until an ISR, event, post, or timer calls `syn_task_resume(task)`:

```c
#define EVT_DATA_READY  SYN_BIT(0)

static SYN_EventGroup uart_events;

// ISR:
void UART_IRQHandler(void) {
    syn_event_set(&uart_events, EVT_DATA_READY);  // ISR-safe, auto-resumes blocked tasks
}

// Task:
static SYN_PT_Status uart_task(SYN_PT *pt, SYN_Task *task)
{
    PT_BEGIN(pt);
    for (;;) {
        PT_BLOCK_EVENT(pt, task, &uart_events, EVT_DATA_READY);
        // Wakes here when EVT_DATA_READY is set (flag auto-cleared)
        process_uart_data();
    }
    PT_END(pt);
}
```

#### Native Task Blocking Matrix Across OS Services

| Subsystem / Layer | Polled Primitive | Native Task-Blocking Primitive | Wake / Resume Mechanism |
|---|---|---|---|
| **Core Expression** | `PT_WAIT_UNTIL(pt, cond)` | `PT_BLOCK_CONDITION(pt, task, cond)` | `syn_task_resume(task)` |
| **Event Flags** | `PT_WAIT_EVENT(pt, grp, mask)` | `PT_BLOCK_EVENT(pt, task, grp, mask)` | `syn_event_set(grp, mask)` |
| **Semaphores** | `PT_SEM_WAIT(pt, sem)` | `PT_SEM_BLOCK(pt, task, sem)` | `PT_SEM_SIGNAL(sem)` |
| **Stream I/O** | `PT_STREAM_WAIT(pt, stream)` | `PT_BLOCK_STREAM(pt, task, stream)` | `syn_stream_write_byte(stream, b)` |
| **SPSC Queue** | `PT_QUEUE_WAIT_POP(pt, q)` | `PT_BLOCK_QUEUE_POP(pt, task, q)` | `syn_spsc_queue_push(q, item)` |
| **Active Objects** | (Polled internal) | `syn_ao_pt_run()` (Built-in `PT_BLOCK_CONDITION`) | `syn_ao_post(ao, ev, p)` |
| **Ethernet Driver** | `PT_ETH_WAIT_FRAME(...)` | `PT_ETH_BLOCK_FRAME(pt, task, eth)` | `syn_port_eth_rx()` frame arrival |
| **DHCP Engine** | `PT_DHCP_WAIT_BOUND(pt, dhcp)` | `PT_DHCP_BLOCK_BOUND(pt, task, dhcp)` | `syn_dhcp_process_packet()` binding |
| **Network Config** | `PT_NETCFG_WAIT_BOUND(...)` | `PT_NETCFG_BLOCK_BOUND(pt, task, netcfg)` | IP assignment event |
| **AutoIP Service** | `PT_AUTOIP_WAIT_BOUND(...)` | `PT_AUTOIP_BLOCK_BOUND(pt, task, autoip)` | Link-local probe completion |
| **Button Input** | `PT_WAIT_BUTTON_PRESS(...)` | `PT_BUTTON_BLOCK_PRESS(pt, task, btn)` | `syn_button_update()` debounce |

| Approach | Scheduling cost | Tickless-safe | Use case |
|---|---|---|---|
| `PT_WAIT_*` (Polled) | Polled every pass | No — prevents sleep | Simple standalone / legacy coroutines |
| `PT_BLOCK_*` (Native) | Skipped entirely while blocked | Yes | Production low-power event-driven services |

### Yield-Safe Priority Boosting (`syn_task_boost_priority`)

Tasks can temporarily elevate their execution priority during critical hardware transactions (e.g. RS-485 frame bursts, SPI flash writes) without risk of stack variable corruption across yields:

```c
static SYN_PT_Status sensor_task(SYN_PT *pt, SYN_Task *task)
{
    PT_BEGIN(pt);
    for (;;) {
        PT_TASK_DELAY_MS(pt, task, 1000);

        // Elevate task from Prio 3 to Prio 0 (highest)
        syn_task_boost_priority(task, 0);

        send_sensor_request();
        PT_WAIT_UNTIL(pt, sensor_data_ready()); // Yield safe across ticks!
        read_sensor_response();

        // Restore back to configured base priority
        syn_task_restore_priority(task);
    }
    PT_END(pt);
}
```

- **`syn_task_boost_priority(task, temp_prio)`**: Elevates `task->priority` to `temp_prio`. Clamped so `temp_prio <= base_priority`.
- **`syn_task_restore_priority(task)`**: Restores `task->priority` back to `task->base_priority`.
- **`syn_task_set_base_priority(task, new_base_prio)`**: Updates `task->base_priority` and `task->priority` permanently.

### Task Delay Semantics: "At Least" Minimum Wait Guarantees

In a cooperative operating system like SyntropicOS, all delay macros (`PT_TASK_DELAY_MS`, `PT_DELAY_MS`, `PT_DELAY_US`) guarantee a **minimum wait duration** ("at least N ms / N us").

!!! important "Non-Preemptive Delay Guarantee"
    Delays do **not** guarantee an exact microsecond or millisecond wakeup time. Because execution is strictly cooperative, a task whose delay has expired will resume on the **next scheduler pass after the currently executing task yields**. Actual resumption latency equals `deadline + execution_time_of_running_tasks`.

#### Millisecond vs Microsecond Delays

1. **Millisecond Delays (`PT_TASK_DELAY_MS`)**:
   - Managed directly by the scheduler top-level loop (`task->delay_until` in ms).
   - Maximum single delay capacity: **24.8 days** ($2,147,483,647$ ms).
   - Ideal for periodic tasks, heartbeat timers, and protocol timeouts.

2. **Microsecond Delays (`PT_DELAY_US`)**:
   - Uses a dedicated, user-supplied persistent target variable (`static uint32_t us_target;`).
   - Does **not** modify `task->delay_until`, preventing top-level scheduler deadline conflicts.
   - Evaluated inside the protothread via `PT_WAIT_UNTIL` against `syn_port_get_tick_us()`. Returns `PT_WAITING` while the deadline is pending, allowing lower-priority tasks to run within the same pass.

```c
static SYN_PT_Status pulse_task(SYN_PT *pt, SYN_Task *task)
{
    static uint32_t us_target; // Persistent target for microsecond delay

    PT_BEGIN(pt);
    for (;;) {
        syn_gpio_write(LED_PIN, SYN_GPIO_HIGH);

        // 1. Microsecond delay: minimum 50 µs wait (uses us_target, task->delay_until stays 0)
        PT_DELAY_US(pt, &us_target, 50);

        syn_gpio_write(LED_PIN, SYN_GPIO_LOW);

        // 2. Millisecond delay: minimum 100 ms wait (managed by scheduler in ms)
        PT_TASK_DELAY_MS(pt, task, 100);
    }
    PT_END(pt);
}
```


**Task states:**


| State | Value | Description |
|---|---|---|
| `SYN_TASK_READY` | 0 | Eligible to run |
| `SYN_TASK_SUSPENDED` | 1 | Skipped until resumed |
| `SYN_TASK_DEAD` | 2 | Exited, will not run again |
| `SYN_TASK_DEFERRED` | 3 | Skipped for one pass (`PT_DEFER`), then cleared to READY |
| `SYN_TASK_BLOCKED` | 4 | Waiting on event — skipped until event fires |
| `SYN_TASK_WAITING` | 5 | Tick-local skip — set when `PT_WAIT_UNTIL` condition is false, cleared to READY at end of tick |

**Task control:**

- `syn_task_suspend(task)` — skip task until resumed
- `syn_task_resume(task)` — make task eligible again
- `syn_task_restart(task)` — reset protothread and set to READY
- `syn_task_is_alive(task)` — check if task is not DEAD
- `syn_sched_alive_count(sched)` — count living tasks

## Timers & Scheduling Extensions

| Module | Header | Config | Description |
|---|---|---|---|
| Timers | `sched/syn_timer.h` | `SYN_USE_TIMER` | Software timers — one-shot and periodic callbacks driven by `syn_port_get_tick_ms()` |
| Events | `util/syn_event.h` | `SYN_USE_EVENT` | 32-bit event flag groups for inter-task signaling |
| Watchdog | `sched/syn_watchdog.h` | `SYN_USE_WATCHDOG` | Per-task software check-in deadlines. Timeout events auto-record to `syn_errlog` if configured. |
| Sequencer | `sched/syn_sequencer.h` | `SYN_USE_SEQUENCER` | Step-based async sequence runner for timed action chains |
| Work Queue | `sched/syn_workqueue.h` | `SYN_USE_WORKQUEUE` | Deferred work queue — safely post work from ISR to main thread |
| Mailbox | `sched/syn_mailbox.h` | Always available | Typed single-producer/single-consumer (SPSC) message queue (header-only) |
| Active Object | `sched/syn_ao.h` | `SYN_USE_AO` | Active Object execution context — combines FSM + event queue + scheduler task |

## Tickless Idle

| Module | Header | Config |
|---|---|---|
| Tickless Idle | `sched/syn_sched.h` | `SYN_USE_TICKLESS` — Low-power sleep between task deadlines (requires: SCHED) |

Enabled via `SYN_USE_TICKLESS 1` in `syn_config.h`. Off by default — the scheduler is unchanged unless you opt in.

### How It Works

In a normal `syn_sched_run_forever()` loop, the CPU busy-loops when no tasks are ready. Tickless idle replaces that with low-power sleep:

```
┌────────────────────────────────────────────────────────┐
│                syn_sched_run_tickless()                │
│                                                        │
│  +---> Run all ready tasks (syn_sched_run)             │
│  |                                                     │
│  |    Any tasks ready NOW?                             │
│  |    |- Yes -> loop back, run them                    │
│  |    \- No  -> compute next wakeup deadline           │
│  |             |                                       │
│  |             |- Deadline exists -> sleep until it    │
│  |             |   syn_port_sleep_until(wake_tick)     │
│  |             |                                       │
│  |             \- No deadlines  -> light sleep         │
│  |                 syn_sleep_enter(sleep)              │
│  |                                                     │
│  |    <--- CPU wakes (timer alarm OR interrupt) --->   │
│  +───────────────────────────────────────────────────  │
└────────────────────────────────────────────────────────┘
```

### Interrupts and Wakeup

The key insight: **`syn_port_sleep_until()` returns on *any* interrupt, not just the timer alarm.** This is how interrupts integrate naturally:

| Wakeup Source | What Happens |
|---|---|
| **Timer alarm fires** | CPU wakes at the scheduled tick. The scheduler runs delayed tasks that are now due. |
| **EXTI / GPIO interrupt** | CPU wakes early. The ISR runs (e.g., button press, sensor data-ready). If the ISR posts work via `syn_workqueue_post()` or sets an event via `syn_event_set()`, the scheduler picks it up on the next loop iteration. |
| **UART / SPI / DMA interrupt** | Same — ISR runs, fills a ring buffer or signals a semaphore. The scheduler runs the task that was waiting on that data. |
| **Any other IRQ** | CPU wakes, ISR runs, scheduler re-evaluates. If nothing is ready, it goes back to sleep. |

The scheduler **doesn't need to know about your interrupts.** It simply re-checks task readiness every time it wakes up. The idle loop is:

1. Run ready tasks
2. No tasks ready? → Sleep until the next deadline (or forever)
3. Wake up (timer *or* interrupt) → goto 1

### Wakelocks

If a peripheral needs the CPU to stay awake (e.g., mid-DMA transfer), use the sleep coordinator's wakelocks:

```c
syn_sleep_lock(&sleep);    // Prevent sleep
// ... do time-critical work ...
syn_sleep_unlock(&sleep);  // Allow sleep again
```

While any wakelock is held, `syn_sched_run_tickless()` busy-loops instead of sleeping — same as `syn_sched_run_forever()`.

### Example: Button + Periodic Task

```c
#define SYN_USE_TICKLESS 1

static SYN_Task tasks[2];
static SYN_Sched sched;
static SYN_Sleep sleep;

// Task A: blink LED every 1 second
static SYN_PT_Status blink_task(SYN_PT *pt, SYN_Task *task) {
    PT_BEGIN(pt);
    for (;;) {
        syn_gpio_toggle(LED_PIN);
        PT_TASK_DELAY_MS(pt, task, 1000);
    }
    PT_END(pt);
}

// Task B: respond to button press (EXTI wakes CPU)
static SYN_PT_Status button_task(SYN_PT *pt, SYN_Task *task) {
    PT_BEGIN(pt);
    for (;;) {
        PT_WAIT_UNTIL(pt, button_pressed);
        handle_button();
        button_pressed = false;
    }
    PT_END(pt);
}

int main(void) {
    syn_task_create(&tasks[0], "blink",  blink_task,  1, NULL);
    syn_task_create(&tasks[1], "button", button_task, 0, NULL);
    syn_sched_init(&sched, tasks, 2);
    syn_sleep_init(&sleep);

    // CPU sleeps between 1-second blinks.
    // Button EXTI wakes it early when pressed.
    syn_sched_run_tickless(&sched, &sleep);
}
```

Between blinks, the CPU enters low-power mode for ~1 second. If a button EXTI fires at 500 ms, the CPU wakes immediately, the ISR sets `button_pressed`, and the scheduler runs `button_task`. Then it goes back to sleep for the remaining ~500 ms until the next blink.

### Port Requirement

Implement `syn_port_sleep_until(uint32_t wake_tick_ms)` in your platform port. This function must:

1. Program a hardware wake timer (RTC alarm, LPTIM compare, etc.) for `wake_tick_ms`
2. Enter a low-power mode (e.g., `__WFI()` on Cortex-M)
3. Return when the alarm fires **or** any interrupt wakes the CPU

The default weak stub falls back to `syn_port_sleep(SYN_SLEEP_LIGHT)` (no timer programming — just WFI).

### Timer-Aware Tickless (`run_tickless_ex`)

When using software timers alongside tickless idle, the basic `syn_sched_run_tickless()` only considers task `delay_until` deadlines. Software timer expirations won't wake the CPU — they'll fire late, after the next task wakes up.

`syn_sched_run_tickless_ex()` solves this by combining both:

```
                             syn_sched_run_tickless_ex()
┌────────────────────────────────────────────────────────────┐
│  1. Run scheduler (syn_sched_run)                          │
│  2. Service software timers (syn_timer_service)            │
│  3. sleep_until = min(next_task_wakeup, next_timer_expiry) │
│  4. Enter low-power sleep until sleep_until                │
│  5. goto 1                                                 │
└────────────────────────────────────────────────────────────┘
```

Requires both `SYN_USE_TICKLESS` and `SYN_USE_TIMER` to be enabled.

```c
// syn_config.h
#define SYN_USE_TICKLESS 1
#define SYN_USE_TIMER    1
```

```c
static SYN_Timer timers[2];
syn_timer_init(&timers[0], 100, true, sensor_poll_cb, NULL);
syn_timer_init(&timers[1], 5000, true, heartbeat_cb, NULL);
syn_timer_start(&timers[0]);
syn_timer_start(&timers[1]);

// CPU wakes for both task delays AND timer expirations
syn_sched_run_tickless_ex(&sched, &sleep, timers, 2);
```

`syn_timer_next_expiry()` is also available standalone if you need to query the earliest timer deadline:

```c
uint32_t next = syn_timer_next_expiry(timers, timer_count);
// UINT32_MAX if no active timers
```

---

## Multicore (AMP)

Enable `SYN_USE_MULTICORE` to add Asymmetric Multiprocessing support. Each core runs its own independent cooperative scheduler; cores communicate via the existing mailbox (upgraded with memory barriers) and protect shared peripherals with spinlocks.

**Key files:**

| File | Purpose |
|------|---------|
| `syn_barrier.h` | Acquire/release memory ordering primitives |
| `syn_port_spinlock.h` | Spinlock, core ID, and IPC notify port functions |
| `syn_spinlock.h` | Scoped `SYN_SPINLOCK_GUARD()` helper |
| `syn_mailbox.h` | SPSC mailbox (barrier-upgraded for cross-core safety) |

### Architecture

```
         Core 0                         Core 1
   ┌──────────────────┐          ┌──────────────────┐
   │ SYN_Sched sched0 │          │ SYN_Sched sched1 │
   │ tasks0[N]        │          │ tasks1[M]        │
   │                  │          │                  │
   │ run_forever()    │<-------->│ run_forever()    │
   │                  │ Mailbox  │                  │
   └──────────────────┘          └──────────────────┘
```

Each core owns its own scheduler, task array, and timers. No changes to the cooperative protothread model. Cross-core coordination uses only two primitives:
- **Mailbox** — typed message passing (SPSC, lock-free)
- **Spinlock** — mutual exclusion for shared hardware (UART, flash, etc.)

### Configuration

```c
// syn_config.h
#define SYN_USE_MULTICORE   1
#define SYN_SPINLOCK_COUNT  4  // number of spinlock IDs (default 4)
```

### Cross-Core Mailbox

The existing `syn_mailbox` is automatically upgraded with acquire/release barriers when `SYN_USE_MULTICORE=1`. On single-core builds, these compile to zero-cost plain volatile access.

```c
typedef struct { uint8_t id; int32_t value; } SensorMsg;

// Place in shared SRAM accessible to both cores
static SYN_MAILBOX_DEFINE(ipc_mbox, SensorMsg, 16);

// Optional: wake consumer core on post
syn_mailbox_set_notify(&ipc_mbox, true);

// Core 0 (producer):
SensorMsg msg = { .id = 1, .value = 42 };
syn_mailbox_post(&ipc_mbox, &msg);

// Core 1 (consumer):
SensorMsg rx;
if (syn_mailbox_receive(&ipc_mbox, &rx)) {
    handle(rx.id, rx.value);
}
```

> **Warning:** The mailbox is strictly **single-producer, single-consumer**. If you need multiple producers, serialize access with a spinlock.

### Spinlocks

Spinlocks protect shared resources (peripherals, log buffers) across cores. They disable interrupts on the acquiring core to prevent priority inversion.

```c
#include "syntropic/util/syn_spinlock.h"

// Scoped lock — guaranteed release on scope exit
SYN_SPINLOCK_GUARD(SYN_SPINLOCK_UART) {
    syn_port_uart_transmit(0, data, len, 10);
}

// Manual lock (for more control)
syn_port_spinlock_acquire(SYN_SPINLOCK_FLASH);
syn_port_flash_write(addr, buf, len);
syn_port_spinlock_release(SYN_SPINLOCK_FLASH);
```

Well-known lock IDs:

| ID | Macro | Purpose |
|----|-------|---------|
| 0 | `SYN_SPINLOCK_UART` | Shared UART |
| 1 | `SYN_SPINLOCK_FLASH` | Shared flash |
| 2 | `SYN_SPINLOCK_USER0` | Application use |
| 3 | `SYN_SPINLOCK_USER1` | Application use |

### Porting Guide

Implement these functions for your platform:

```c
// Required:
void     syn_port_spinlock_acquire(uint8_t id);   // disable IRQ + spin
void     syn_port_spinlock_release(uint8_t id);   // release + restore IRQ
bool     syn_port_spinlock_try_acquire(uint8_t id);
uint8_t  syn_port_core_id(void);                  // return 0 or 1
void     syn_port_memory_barrier(void);           // DMB on ARM, __sync_synchronize fallback

// Optional (no-op stub provided):
void     syn_port_ipc_notify(void);               // SEV on ARM, triggers WFE wakeup
```

**RP2040 example** — hardware spinlocks:

```c
#include "hardware/sync.h"

static spin_lock_t *locks[SYN_SPINLOCK_COUNT];
static uint32_t saved_irq[SYN_SPINLOCK_COUNT];

void syn_port_spinlock_acquire(uint8_t id) {
    saved_irq[id] = spin_lock_blocking(locks[id]);
}

void syn_port_spinlock_release(uint8_t id) {
    spin_unlock(locks[id], saved_irq[id]);
}

void syn_port_memory_barrier(void) {
    __dmb();  // Data Memory Barrier
}

void syn_port_ipc_notify(void) {
    __sev();  // Send Event — wakes other core from WFE
}
```
