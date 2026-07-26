/**
 * @file sys_init.h
 * @brief Background OS Scheduler Initialization for Arduino Examples.
 *
 * Hides OS task scheduling and port initialization into a supporting tab
 * so main example sketches (.ino) remain clean and easy to read.
 */

#ifndef SYS_INIT_H
#define SYS_INIT_H

#include <SyntropicOS.h>
#include <syntropic/port/syn_port_serial.h>
#include <syntropic/sched/syn_sched.h>

static SYN_Sched sys_sched;
static SYN_Task sys_tasks[1];

/* Background task function pointer provided by the sketch */
typedef void (*SysBgTaskFn)(void);
static SysBgTaskFn g_bg_fn = NULL;

static SYN_PT_Status sys_bg_pt(SYN_PT *pt, SYN_Task *task)
{
    PT_BEGIN(pt);
    for (;;) {
        if (g_bg_fn != NULL) {
            g_bg_fn();
        }
        PT_TASK_DELAY_MS(pt, task, 10);
    }
    PT_END(pt);
}

/** Initialize Serial Port, Logger, and Background Scheduler. */
inline void sys_init(SysBgTaskFn bg_fn)
{
    syn_port_serial_init(115200);
    syn_log_init(SYN_LOG_INFO);
    g_bg_fn = bg_fn;

    syn_task_create(&sys_tasks[0], "sys_bg", sys_bg_pt, 0, NULL);
    syn_sched_init(&sys_sched, sys_tasks, 1);
}

/** Run the background scheduler loop. */
inline void sys_run(void)
{
    if (!syn_sched_run(&sys_sched)) {
        syn_port_delay_ms(1);
    }
}

#endif /* SYS_INIT_H */
