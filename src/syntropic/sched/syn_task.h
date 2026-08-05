/**
 * @file syn_task.h
 * @brief Task descriptor for the cooperative scheduler.
 *
 * Defines the task control block (SYN_Task) that pairs a protothread
 * with scheduling metadata: priority, state, name, delay target, and
 * optional event-wait fields for true blocking.
 *
 * Tasks are caller-owned — you allocate them however you like (static
 * array, global, on the stack). The scheduler just takes a pointer to
 * your array.
 *
 * @par Task restart and static local state
 * syn_task_restart() resets the protothread continuation (lc), delay
 * deadlines, and event wait masks. It does NOT reset static local variables
 * inside the task function. Place all state initialization after PT_BEGIN()
 * or reset static state inside task-private user_data structs to ensure clean
 * restarts.
 * @ingroup syn_sched
 */

#ifndef SYN_TASK_H
#define SYN_TASK_H

#include "../pt/syn_pt.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Task states ────────────────────────────────────────────────────────── */

/** @brief Cooperative task lifecycle state. */
typedef enum {
    SYN_TASK_READY = 0,     /**< Eligible to run on next scheduler tick */
    SYN_TASK_SUSPENDED = 1, /**< Paused — skipped by scheduler          */
    SYN_TASK_DEAD = 2,      /**< Exited — will not run again            */
    SYN_TASK_DEFERRED = 3,  /**< Deferred — skipped for one pass        */
    SYN_TASK_BLOCKED = 4,   /**< Blocked on event — skipped until fired */
    SYN_TASK_WAITING = 5,   /**< PT_WAIT condition false — skip this tick */
} SYN_TaskState;

/* ── Forward declaration ────────────────────────────────────────────────── */

struct SYN_Task;

/* ── Task function signature ────────────────────────────────────────────── */

/**
 * @brief Protothread task function.
 *
 * A task function receives its own protothread and task descriptor.
 * It must follow the PT_BEGIN / PT_END pattern.
 *
 * @param pt    Pointer to the task's protothread (same as &task->pt).
 * @param task  Pointer to the task descriptor (for user_data, delay, etc.).
 * @return PT status indicating whether the thread yielded, is waiting, or exited.
 */
typedef SYN_PT_Status (*SYN_TaskFunc)(SYN_PT *pt, struct SYN_Task *task);

/* ── Task control block ─────────────────────────────────────────────────── */

#include "syn_event_flags.h"

/**
 * @brief Task descriptor — binds a protothread to scheduler metadata.
 *
 * Typical size: ~28 bytes on a 32-bit target.
 */
typedef struct SYN_Task {
    SYN_PT pt;                  /**< Protothread continuation (2 bytes)      */
    SYN_TaskFunc func;          /**< The task's protothread function          */
    const char *name;           /**< Human-readable name (for debug/logging)  */
    uint8_t priority;           /**< Active priority (0 = highest)            */
    uint8_t base_priority;      /**< Configured base priority                 */
    volatile uint8_t state;     /**< SYN_TaskState                           */
    uint32_t delay_until;       /**< Tick deadline for PT_TASK_DELAY_MS       */
    void *user_data;            /**< Optional pointer to task-private state   */
    SYN_EventFlags *wait_event; /**< Event flags task blocks on (NULL if not blocking) */
    uint32_t wait_mask;         /**< Bitmask of event flags to wait for       */
} SYN_Task;

#ifdef __cplusplus
}
#endif

#endif /* SYN_TASK_H */
