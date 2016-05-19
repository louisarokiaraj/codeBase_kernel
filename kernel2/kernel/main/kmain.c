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

#include "types.h"
#include "globals.h"
#include "kernel.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "errno.h"

#include "fs/vnode.h"
#include "fs/file.h"

GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

#define LONGPATH "/TestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTestingTesting"
#define LONGNAME "TESTINGTESTINGTESTINGTESTINGTESTING"
static void       hard_shutdown(void);
static void      *bootstrap(int arg1, void *arg2);
static void      *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void      *initproc_run(int arg1, void *arg2);

static context_t bootstrap_context;
extern int gdb_wait;

extern void *sunghan_test(int, void*);
extern void *sunghan_deadlock_test(int, void*);
extern void *faber_thread_test(int, void*);

extern void *vfstest_main(int, void*);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);

/**
 * This is the first real C function ever called. It performs a lot of
 * hardware-specific initialization, then creates a pseudo-context to
 * execute the bootstrap function in.
 */
void
kmain()
{
        GDB_CALL_HOOK(boot);

        dbg_init();
        dbgq(DBG_CORE, "Kernel binary:\n");
        dbgq(DBG_CORE, "  text: 0x%p-0x%p\n", &kernel_start_text, &kernel_end_text);
        dbgq(DBG_CORE, "  data: 0x%p-0x%p\n", &kernel_start_data, &kernel_end_data);
        dbgq(DBG_CORE, "  bss:  0x%p-0x%p\n", &kernel_start_bss, &kernel_end_bss);

        page_init();

        pt_init();
        slab_init();
        pframe_init();

        acpi_init();
        apic_init();
        pci_init();
        intr_init();

        gdt_init();

        /* initialize slab allocators */
#ifdef __VM__
        anon_init();
        shadow_init();
#endif
        vmmap_init();
        proc_init();
        kthread_init();

#ifdef __DRIVERS__
        bytedev_init();
        blockdev_init();
#endif

        void *bstack = page_alloc();
        pagedir_t *bpdir = pt_get();
        KASSERT(NULL != bstack && "Ran out of memory while booting.");
        /* This little loop gives gdb a place to synch up with weenix.  In the
         * past the weenix command started qemu was started with -S which
         * allowed gdb to connect and start before the boot loader ran, but
         * since then a bug has appeared where breakpoints fail if gdb connects
         * before the boot loader runs.  See
         *
         * https://bugs.launchpad.net/qemu/+bug/526653
         *
         * This loop (along with an additional command in init.gdb setting
         * gdb_wait to 0) sticks weenix at a known place so gdb can join a
         * running weenix, set gdb_wait to zero  and catch the breakpoint in
         * bootstrap below.  See Config.mk for how to set GDBWAIT correctly.
         *
         * DANGER: if GDBWAIT != 0, and gdb is not running, this loop will never
         * exit and weenix will not run.  Make SURE the GDBWAIT is set the way
         * you expect.
         */
        while (gdb_wait) ;
        context_setup(&bootstrap_context, bootstrap, 0, NULL, bstack, PAGE_SIZE, bpdir);
        context_make_active(&bootstrap_context);

        panic("\nReturned to kmain()!!!\n");
}

/**
 * Clears all interrupts and halts, meaning that we will never run
 * again.
 */
static void
hard_shutdown()
{
#ifdef __DRIVERS__
        vt_print_shutdown();
#endif
        __asm__ volatile("cli; hlt");
}

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
bootstrap(int arg1, void *arg2)
{

        proc_t *idle_Process;
        kthread_t *idle_thread;
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_CORE, "SIGNATURE: 53616c7465645f5f701f798262a720d44992d909fdb2e367a9775c13ab9e45c4bf59aa1c3f335b77ae475d0a8462c7b3\n");
        /* necessary to finalize page table information */
        pt_template_init();
        idle_Process = proc_create("IDLE");
        curproc = idle_Process;
        KASSERT(NULL != curproc);
        dbg(DBG_PRINT, "(GRADING1A 1.a) the IDLE process has been created successfully\n");

        idle_Process->p_pid = PID_IDLE;
        KASSERT(PID_IDLE == curproc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.a) what has been created is the IDLE process\n");

        idle_thread = kthread_create(idle_Process,idleproc_run,arg1,arg2);
        curthr = idle_thread;
        KASSERT(NULL != curthr);
        dbg(DBG_PRINT, "(GRADING1A 1.a) thread for the IDLE process has been created successfully\n");

        context_make_active(&(idle_thread->kt_ctx));
        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        dbg(DBG_PRINT, "Returning bootstrap\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */

        /*NOT_YET_IMPLEMENTED("VFS: idleproc_run");*/
        curproc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);
        if(initthr!=NULL)
        {
          initthr->kt_proc->p_cwd = vfs_root_vn;
          vref(vfs_root_vn);
        }
        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        int x = do_mkdir("/dev");
        if(x == 0){
          if(do_mknod("/dev/null", S_IFCHR, MEM_NULL_DEVID) != 0){
            panic("Device null not created\n");
          }
          if(do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID) != 0){
            panic("Device zero not created\n");
          }
          if(do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2,0)) != 0){
            panic("Device /dev/tty0 not created\n");
          }
          if(do_mknod("/dev/tty1", S_IFCHR, MKDEVID(2,1)) != 0){
            panic("Device /dev/tty1 not created\n");
          }
        }
        else{
          KASSERT(x == -EEXIST);
        }
        /*NOT_YET_IMPLEMENTED("VFS: idleproc_ruvfs_root_vnn");*/
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

#ifdef __MTP__
        kthread_reapd_shutdown();
#endif


#ifdef __SHADOWD__
        /* wait for shadowd to shutdown */
        shadowd_shutdown();
#endif

#ifdef __VFS__
        /* Shutdown the vfs: */
        dbg_print("weenix: vfs shutdown...\n");
        vput(curproc->p_cwd);
        if (vfs_shutdown())
                panic("vfs shutdown FAILED!!\n");

#endif

        /* Shutdown the pframe system */
#ifdef __S5FS__
        pframe_shutdown();
#endif

        dbg_print("\nweenix: halted cleanly!\n");
        GDB_CALL_HOOK(shutdown);
        hard_shutdown();
        return NULL;
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
static kthread_t *
initproc_create(void)
{
        proc_t *init_process;
        kthread_t *init_thread;

        init_process = proc_create("INIT");
        KASSERT(NULL != init_process);
        dbg(DBG_PRINT, "(GRADING1A 1.b) the INIT process has been created successfully\n");

        init_process->p_pid = PID_INIT;
        KASSERT(PID_INIT == init_process->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.b) what has been created is the INIT process\n");

        init_thread = kthread_create(init_process,&initproc_run,0,NULL);
        KASSERT(init_thread != NULL);
        dbg(DBG_PRINT, "(GRADING1A 1.b) the thread for the INIT process has been created successfully\n");
        return init_thread;
}

int call_faber_test(kshell_t *inShell,int arg1, char **arg2){
        int ret_status;
	      proc_t *test_process_faber = proc_create("Faber_testing");
	      dbg(DBG_PRINT, "(GRADING1C) Faber Test Started\n");
	      kthread_t *test_thread_faber= kthread_create(test_process_faber,&faber_thread_test,0,NULL);
	      sched_make_runnable(test_thread_faber);
        do_waitpid(test_process_faber->p_pid,0,&ret_status);
        dbg(DBG_PRINT, "(GRADING1C) Faber Test Completed Successfully\n");
        return NULL;
}

int call_sunghan_test(kshell_t *inShell,int arg1, char **arg2){
        int ret_status;
	      proc_t *test_process_sungHan = proc_create("sunghan_test");
	      dbg(DBG_PRINT, "(GRADING1D 1) SungHan Test Started\n");
	      kthread_t *test_thread_sungHan = kthread_create(test_process_sungHan,&sunghan_test,0,NULL);
	      sched_make_runnable(test_thread_sungHan);
        do_waitpid(test_process_sungHan->p_pid,0,&ret_status);
        dbg(DBG_PRINT, "(GRADING1D 1) SungHan Test Completed Successfully\n");
        return NULL;
}

int call_sunghan_deadlock(kshell_t *inShell,int arg1, char **arg2){
        int ret_status;
	      proc_t *test_process_sungHan_deadlock = proc_create("sunghan_test_deadlock");
	      dbg(DBG_PRINT, "(GRADING1D 2) SungHan Deadlock Test Started\n");
	      kthread_t *test_thread_sungHan_deadlock = kthread_create(test_process_sungHan_deadlock,&sunghan_deadlock_test,0,NULL);
	      sched_make_runnable(test_thread_sungHan_deadlock);
        do_waitpid(test_process_sungHan_deadlock->p_pid,0,&ret_status);
        dbg(DBG_PRINT, "(GRADING1D 2) SungHan Deadlock Test Completed Successfully\n");
        return NULL;
}

int call_vfstest_main(kshell_t *inShell,int arg1, char **arg2){
        int ret_status;
	      proc_t *vfstest = proc_create("vfstest");
	      dbg(DBG_PRINT, "(GRADING2B) VFS Test Started\n");
	      kthread_t *vfstest_thr = kthread_create(vfstest,&vfstest_main,1,NULL);
	      sched_make_runnable(vfstest_thr);
        do_waitpid(vfstest->p_pid,0,&ret_status);
        dbg(DBG_PRINT, "(GRADING2B) VFS Test Completed Successfully\n");
        return NULL;
}

void *vfs_self_thread(int arg1, void *arg2){
    const char *test_path = NULL;
    struct stat *test_bufer=NULL;
        do_mknod("/testing", S_IFREG, 5); 
        do_mknod("/testing/testing ", S_IFCHR, 5);
        do_mknod("/testing", S_IFCHR, 5); 
        do_mknod("/testing", S_IFCHR, 5);
        do_mknod(LONGPATH, S_IFCHR, 5);
        do_unlink(LONGPATH);
        do_mkdir(test_path);
        do_rmdir(test_path);
        do_mkdir(LONGPATH);
        do_rmdir(LONGPATH);
        do_stat(LONGPATH,test_bufer);
        int test_file=do_open("testing", O_WRONLY | O_CREAT);
        vnode_t *res;
        file_t *test_lookup=fget(test_file);
        lookup(test_lookup->f_vnode, "testing", strlen("testing"), &res);
        fput(test_lookup);
        do_close(test_file);
        do_mkdir("SelfTest");
        vnode_t *res2;
        lookup(curproc->p_cwd,LONGNAME, strlen(LONGNAME), &res2);
        do_rmdir("SelfTest");
    return NULL;
}

int VFS_Self_Test(kshell_t *inShell,int arg1, char **arg2){
        int ret_status;
        proc_t *vfs_self_test = proc_create("VFS_Self_Test");
        dbg(DBG_PRINT, "(GRADING2D) VFS Self Test Started\n");
        kthread_t *vfs_self_test_thr = kthread_create(vfs_self_test,&vfs_self_thread,0,NULL);
        sched_make_runnable(vfs_self_test_thr);
        do_waitpid(vfs_self_test->p_pid,0,&ret_status);
        dbg(DBG_PRINT, "(GRADING2D) VFS Self Completed Successfully\n");
        return NULL; 
}





/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
initproc_run(int arg1, void *arg2)
{
	#ifdef __DRIVERS__
	kshell_add_command("faber",call_faber_test,"Run faber_thread_test().");
	kshell_add_command("sunghan_test",call_sunghan_test,"Run sunghan_test().");
	kshell_add_command("sunghan_Deadlock_test",call_sunghan_deadlock,"Run sunghan_deadlock_test().");
  	kshell_add_command("VFS_Test", call_vfstest_main,"Run vfs_test().");
  	kshell_add_command("Thread_Test", faber_fs_thread_test, "Run faber_fs_thread_test().");
  	kshell_add_command("Directory_Test", faber_directory_test, "Run faber_directory_test().");
  	kshell_add_command("VFS_Self_Test", VFS_Self_Test, "Run VFS_Self_Test().");
	kshell_t *kshell = kshell_create(0);
	if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
	KASSERT(kshell != NULL);
	dbg(DBG_PRINT, "(GRADING1B) kshell created successfully\n");
	while (kshell_execute_next(kshell));
	kshell_destroy(kshell);
	dbg(DBG_PRINT, "(GRADING1B) kshell Destroyed successfully\n");
	#endif /* __DRIVERS__ */
	return NULL;
}
