/******************************************************************************/
/* Important Fall 2015 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "config.h"
#include "globals.h"

#include "errno.h"

#include "util/init.h"
#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"

#include "mm/slab.h"
#include "mm/page.h"

kthread_t *curthr; /* global */
static slab_allocator_t *kthread_allocator = NULL;

#ifdef __MTP__
/* Stuff for the reaper daemon, which cleans up dead detached threads */
static proc_t *reapd = NULL;
static kthread_t *reapd_thr = NULL;
static ktqueue_t reapd_waitq;
static list_t kthread_reapd_deadlist; /* Threads to be cleaned */

static void *kthread_reapd_run(int arg1, void *arg2);
#endif

void
kthread_init()
{
        kthread_allocator = slab_allocator_create("kthread", sizeof(kthread_t));
        KASSERT(NULL != kthread_allocator);
}

/**
 * Allocates a new kernel stack.
 *
 * @return a newly allocated stack, or NULL if there is not enough
 * memory available
 */
static char *
alloc_stack(void)
{
        /* extra page for "magic" data */
        char *kstack;
        int npages = 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT);
        kstack = (char *)page_alloc_n(npages);

        return kstack;
}

/**
 * Frees a stack allocated with alloc_stack.
 *
 * @param stack the stack to free
 */
static void
free_stack(char *stack)
{
        page_free_n(stack, 1 + (DEFAULT_STACK_SIZE >> PAGE_SHIFT));
}

void
kthread_destroy(kthread_t *t)
{
        KASSERT(t && t->kt_kstack);
        free_stack(t->kt_kstack);
        if (list_link_is_linked(&t->kt_plink))
                list_remove(&t->kt_plink);

        slab_obj_free(kthread_allocator, t);
}

/*
 * Allocate a new stack with the alloc_stack function. The size of the
 * stack is DEFAULT_STACK_SIZE.
 *
 * Don't forget to initialize the thread context with the
 * context_setup function. The context should have the same pagetable
 * pointer as the process.
 */
kthread_t *
kthread_create(struct proc *p, kthread_func_t func, long arg1, void *arg2)
{
  KASSERT(NULL != p);
  dbg(DBG_PRINT, "(GRADING1A 3.a) new thread to be created has an associated process pid = %d\n",p->p_pid);
	kthread_t *Newthread = (kthread_t *)slab_obj_alloc(kthread_allocator);
  KASSERT(Newthread != NULL);
  memset(Newthread,0,sizeof(kthread_t));
	Newthread->kt_kstack = alloc_stack();
  list_link_init(&Newthread->kt_plink);
  list_link_init(&Newthread->kt_qlink);
	context_setup(&(Newthread->kt_ctx), func, arg1, arg2,(Newthread->kt_kstack), DEFAULT_STACK_SIZE,p->p_pagedir);
	Newthread->kt_proc = p;
  Newthread->kt_retval = NULL;
	Newthread->kt_cancelled = 0;
	Newthread->kt_wchan = NULL;
	Newthread->kt_state = KT_RUN;
  list_insert_tail(&(p->p_threads),&(Newthread->kt_plink));
  return Newthread;
}

/*
 * If the thread to be cancelled is the current thread, this is
 * equivalent to calling kthread_exit. Otherwise, the thread is
 * sleeping and we need to set the cancelled and retval fields of the
 * thread.
 *
 * If the thread's sleep is cancellable, cancelling the thread should
 * wake it up from sleep.
 *
 * If the thread's sleep is not cancellable, we do nothing else here.
 */
void
kthread_cancel(kthread_t *kthr, void *retval)
{
  KASSERT(NULL != kthr);
  dbg(DBG_PRINT, "(GRADING1A 3.b) Passed Thread is NOT NULL\n");
  dbg(DBG_PRINT, "(GRADING1A 3.b) Passed Thread is NOT NULL\n");
  kthr->kt_retval=retval;
  kthr->kt_cancelled = 1;
    if(kthr==curthr){
      dbg(DBG_PRINT, "(GRADING1C 1) Calling Kthread Exit on the current Thread\n");
      kthread_exit(retval);
    }else{
      dbg(DBG_PRINT, "(GRADING1C 5) Calling Kthread Exit on another thread\n");
        if(kthr->kt_state == KT_SLEEP_CANCELLABLE){
          dbg(DBG_PRINT, "(GRADING1C 5) Calling Sched Cancel on the another thread because it is in KT_SLEEP_CANCELLABLE\n");
          sched_cancel(kthr);
        }
    }
}

/*
 * You need to set the thread's retval field, set its state to
 * KT_EXITED, and alert the current process that a thread is exiting
 * via proc_thread_exited.
 *
 * It may seem unneccessary to push the work of cleaning up the thread
 * over to the process. However, if you implement MTP, a thread
 * exiting does not necessarily mean that the process needs to be
 * cleaned up.
 */
void
kthread_exit(void *retval)
{
    KASSERT(curthr->kt_proc == curproc);
    dbg(DBG_PRINT, "(GRADING1A 3.c) current process pid =%d is the parent of current thread\n",curproc->p_pid);
    kthread_t *curr_thread = curthr;
    curr_thread->kt_retval = retval;
    list_link_init(&curr_thread->kt_qlink);
    KASSERT(!curthr->kt_qlink.l_next && !curthr->kt_qlink.l_prev);
    dbg(DBG_PRINT, "(GRADING1A 3.c) kt_qlink queue is empty\n");
    curthr->kt_wchan = NULL;
    KASSERT(!curthr->kt_wchan);
    dbg(DBG_PRINT, "(GRADING1A 3.c) curthr is not in any queue\n");
    curr_thread->kt_state = KT_EXITED;
    proc_thread_exited(retval);
}

/*
 * The new thread will need its own context and stack. Think carefully
 * about which fields should be copied and which fields should be
 * freshly initialized.
 *
 * You do not need to worry about this until VM.
 */
kthread_t *
kthread_clone(kthread_t *thr)
{
        /*NOT_YET_IMPLEMENTED("VM: kthread_clone");
        return NULL;*/
        KASSERT(KT_RUN == thr->kt_state);
        dbg(DBG_PRINT, "(GRADING3A 8.a) the input thread's kt_state is KT_RUN\n");
        kthread_t* new_Thread = (kthread_t*) slab_obj_alloc(kthread_allocator);
        new_Thread->kt_kstack = alloc_stack();
        context_setup(&(new_Thread->kt_ctx),NULL,0,0,new_Thread->kt_kstack,DEFAULT_STACK_SIZE,thr->kt_proc->p_pagedir);
        new_Thread->kt_retval = thr->kt_retval;
        new_Thread->kt_proc   = thr->kt_proc;
        new_Thread->kt_cancelled = thr->kt_cancelled;
        new_Thread->kt_errno  = thr->kt_errno;
        new_Thread->kt_wchan  = thr->kt_wchan;
        list_link_init(&(new_Thread->kt_qlink));
        list_link_init(&(new_Thread->kt_plink));
        new_Thread->kt_state  = thr->kt_state;
        KASSERT(KT_RUN == new_Thread->kt_state);
        dbg(DBG_PRINT, "(GRADING3A 8.a) the cloned thread's kt_state is KT_RUN\n");
        return new_Thread;
}

/*
 * The following functions will be useful if you choose to implement
 * multiple kernel threads per process. This is strongly discouraged
 * unless your weenix is perfect.
 */
#ifdef __MTP__
int
kthread_detach(kthread_t *kthr)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_detach");
        return 0;
}

int
kthread_join(kthread_t *kthr, void **retval)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_join");
        return 0;
}

/* ------------------------------------------------------------------ */
/* -------------------------- REAPER DAEMON ------------------------- */
/* ------------------------------------------------------------------ */
static __attribute__((unused)) void
kthread_reapd_init()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_init");
}
init_func(kthread_reapd_init);
init_depends(sched_init);

void
kthread_reapd_shutdown()
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_shutdown");
}

static void *
kthread_reapd_run(int arg1, void *arg2)
{
        NOT_YET_IMPLEMENTED("MTP: kthread_reapd_run");
        return (void *) 0;
}
#endif
