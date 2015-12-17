Documentation for Kernel Assignment 2
=====================================

+-------------+
| BUILD & RUN |
+-------------+

Comments: Use "make" command to build/compile all the files. And "make clean" to clean all the binary files. In the Kshell, you can use the following commands to run all the required tests

VFS_Test : Runs VFS test which is specified in the specifications

Thread_Test: Runs the Faber Tests for file system mentioned in the specifications. Please give commmand line arguments for this test, as per given in the specs. 

Directory_Test: Runs the Faber Tests for the directory system. Please give commmand line arguments for this test, as per given in the specs.

VFS_Self_Test: Runs the self-check tests for the code paths which are not covered by the previous given tests.

exit : To terminate the Kshell. 

+-----------------+
| SKIP (Optional) |
+-----------------+

None

+---------+
| GRADING |
+---------+

(A.1) In fs/vnode.c:
    (a) In special_file_read(): 6 out of 6 pts
    (b) In special_file_write(): 6 out of 6 pts

(A.2) In fs/namev.c:
    (a) In lookup(): 6 out of 6 pts
    (b) In dir_namev(): 10 out of 10 pts
    (c) In open_namev(): 2 out of 2 pts

(A.3) In fs/vfs_syscall.c:
    (a) In do_write(): 6 out of 6 pts
    (b) In do_mknod(): 2 out of 2 pts
    (c) In do_mkdir(): 2 out of 2 pts
    (d) In do_rmdir(): 2 out of 2 pts
    (e) In do_unlink(): 2 out of 2 pts
    (f) In do_stat(): 2 out of 2 pts

(B) vfstest: 39 out of 39 pts
    Comments: None

(C.1) faber_fs_thread_test (3 out of 3 pts)
(C.2) faber_directory_test (2 out of 2 pts)

(D) Self-checks: (10 out of 10 pts)
    Comments: Paths uncovered are covered in the self-check tests. In the kshell, use command "VFS_Self_Test" to run the self check tests.

Missing required section(s) in README file (vfs-README.txt): -0
Submitted binary file : -0
Submitted extra (unmodified) file : -0
Wrong file location in submission : -0
Altered or removed top comment block in a .c file : -0
Use dbg_print(...) instead of dbg(DBG_PRINT, ...) : -0
Not properly indentify which dbg() printout is for which item in the grading guidelines : -0
Cannot compile : -0
Compiler warnings : -0
"make clean" : -0
Useless KASSERT : -0
Insufficient/Confusing dbg : -0
Kernel panic : -0
Cannot halt kernel cleanly : -0

+------+
| BUGS |
+------+

Comments: None

+---------------------------+
| CONTRIBUTION FROM MEMBERS |
+---------------------------+

Equal Share contribution from all the 4 members of our team

+------------------+
| OTHER (Optional) |
+------------------+

Special DBG setting in Config.mk for certain tests: None, abides the given specifications.
Comments on deviation from spec (you will still lose points, but it's better to let the grader know): None, abides the given specifications.
General comments on design decisions: None

