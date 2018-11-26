/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <k_api.h>

#if (RHINO_CONFIG_KOBJ_DYN_ALLOC > 0)

#if (RHINO_CONFIG_USER_SPACE > 0)
static proc_free(ktask_t *task)
{
    CPSR_ALLOC();

    ktask_t      *proc;
    kqueue_t     *res_q;
    cpu_stack_t  *u_stack;
    klist_t      *head = &task->kobj.task_head;

    u_stack = task->task_ustack_base;
    proc = task->proc_addr;

    if (task->is_proc == 1u) {
        res_q = proc->res_q;

        RHINO_CRITICAL_ENTER();
        klist_rm(&task->task_user);
        if (head->next == head) {
            RHINO_CRITICAL_EXIT();
            k_proc_unload(task->pid);
        }
        RHINO_CRITICAL_EXIT();
        krhino_queue_back_send(res_q, task->task_ustack_base);
    }
    else {
        if (task->proc_addr != 0) {
            res_q = proc->res_q;

            RHINO_CRITICAL_ENTER();
            klist_rm(&task->task_user);
            if (head->next == head) {
                RHINO_CRITICAL_EXIT();
                k_proc_unload(task->pid);
            }
            RHINO_CRITICAL_EXIT();
            krhino_queue_back_send(res_q, task->task_ustack_base);
            
            
        }
    }    
}
#endif

void dyn_mem_proc_task(void *arg)
{
    CPSR_ALLOC();

    size_t      i;
    kstat_t     ret;
    res_free_t *res_free;
    res_free_t  tmp;

    (void)arg;

    while (1) {
        ret = krhino_sem_take(&g_res_sem, RHINO_WAIT_FOREVER);
        if (ret != RHINO_SUCCESS) {
            k_err_proc(RHINO_DYN_MEM_PROC_ERR);
        }

        while (1) {
            RHINO_CRITICAL_ENTER();
            if (!is_klist_empty(&g_res_list)) {
                res_free = krhino_list_entry(g_res_list.next, res_free_t, res_list);
                klist_rm(&res_free->res_list);
                RHINO_CRITICAL_EXIT();
#if (RHINO_CONFIG_USER_SPACE > 0)
                proc_free((ktask_t *)(res_free->res[1]));
#endif
                memcpy(&tmp, res_free, sizeof(res_free_t));
                for (i = 0; i < tmp.cnt; i++) {
                    krhino_mm_free(tmp.res[i]);
                }
            }
            else {
                RHINO_CRITICAL_EXIT();
                break;
            }
        }
    }
}

void dyn_mem_proc_task_start(void)
{
    krhino_task_create(&g_dyn_task, "dyn_mem_proc_task", 0, RHINO_CONFIG_K_DYN_MEM_TASK_PRI,
                        0, g_dyn_task_stack, RHINO_CONFIG_K_DYN_TASK_STACK,
                        dyn_mem_proc_task, 1);
}
#endif

