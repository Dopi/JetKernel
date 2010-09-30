/*
 * Copyright © Marvell International Ltd. and/or its affiliates, 2003-2006
 */

#ifndef	__WLAN_THREAD_H_
#define	__WLAN_THREAD_H_

#include	<linux/kthread.h>

typedef struct
{
    struct task_struct *task;
    wait_queue_head_t waitQ;
    pid_t pid;
    void *priv;
} wlan_thread;

static inline void
wlan_activate_thread(wlan_thread * thr)
{
        /** Record the thread pid */
    thr->pid = current->pid;

        /** Initialize the wait queue */
    init_waitqueue_head(&thr->waitQ);
}

static inline void
wlan_deactivate_thread(wlan_thread * thr)
{
    ENTER();

    LEAVE();
}

static inline void
wlan_create_thread(int (*wlanfunc) (void *), wlan_thread * thr, char *name)
{
    thr->task = kthread_run(wlanfunc, thr, "%s", name);
}

static inline int
wlan_terminate_thread(wlan_thread * thr)
{
    ENTER();

    /* Check if the thread is active or not */
    if (!thr->pid) {
        PRINTM(INFO, "Thread does not exist\n");
        return -1;
    }
    kthread_stop(thr->task);

    LEAVE();
    return 0;
}

#endif
