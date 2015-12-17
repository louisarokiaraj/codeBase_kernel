Documentation for Kernel Assignment 3
=====================================

+-------------+
| BUILD & RUN |
+-------------+

--> make clean
--> make
+------------+
| USER SHELL |
+------------+

Is the following statement correct about your submission (please replace
    "(Comments?)" with either "yes" or "no")?  "We run /sbin/init from the
    kernel init process in our submission": Yes


+---------+
| GRADING |
+---------+

(A.1) In mm/pframe.c:
    (a) In pframe_get(): 2 out of 2 pt
    (b) In pframe_pin(): 1 out of 1 pt
    (c) In pframe_unpin(): 1 out of 1 pt

(A.2) In vm/mmap.c:
    (a) In do_mmap(): 2 out of 2 pts

(A.3) In vm/vmmap.c:
    (a) In vmmap_destroy(): 2 out of 2 pts
    (b) In vmmap_insert(): 4 out of 4 pts
    (c) In vmmap_lookup(): 2 out of 2 pt
    (d) In vmmap_map(): 3 out of 3 pts
    (e) In vmmap_is_range_empty(): 2 out of 2 pt

(A.4) In vm/anon.c:
    (a) In anon_init(): 1 out of 1 pt
    (b) In anon_ref(): 1 out of 1 pt
    (c) In anon_put(): 1 out of 1 pt
    (d) In anon_fillpage(): 1 out of 1 pt

(A.5) In vm/pagefault.c:
    (a) In handle_pagefault(): 2 out of 2 pts

(A.6) In vm/shadow.c:
    (a) In shadow_init(): 1 out of 1 pt
    (b) In shadow_ref(): 1 out of 1 pt
    (c) In shadow_put(): 1 out of 1 pts
    (d) In shadow_lookuppage(): 2 out of 2 pts
    (e) In shadow_fillpage(): 2 out of 2 pts

(A.7) In proc/fork.c:
    (a) In do_fork(): 6 out of 6 pts

(A.8) In proc/kthread.c:
    (a) In kthread_clone(): 2 out of 2 pts

(B.1) /usr/bin/hello (3 out of 3 pts)
(B.2) /usr/bin/args ab cde fghi j (3 out of 3 pts)
(B.3) /bin/uname -a (2 out of 2 pts)
(B.4) /bin/stat /README (1 out of 1 pt)
(B.5) /bin/stat /usr (1 out of 1 pt)
(B.6) /usr/bin/fork-and-wait (5 out of 5 pts)

(C.1) help (1 out of 1 pt)
(C.2) echo hello (1 out of 1 pt)
(C.3) cat /README (1 out of 1 pt)
(C.4) /bin/ls (1 out of 1 pt)
(C.5) segfault (1 out of 1 pt)

(D.1) /usr/bin/vfstest (6 out of 6 pts)
(D.2) /usr/bin/memtest (6 out of 6 pts)
(D.3) /usr/bin/eatmem (6 out of 6 pts)
(D.4) /usr/bin/forkbomb (6 out of 6 pts)
(D.5) /usr/bin/stress (6 out of 6 pts)

(E.1) /usr/bin/vfstest (1 out of 1 pt)
(E.2) /usr/bin/memtest (1 out of 1 pt)
(E.3) /usr/bin/eatmem (1 out of 1 pt)
(E.4) /usr/bin/forkbomb (1 out of 1 pt)
(E.5) /usr/bin/stress (1 out of 1 pt)

(F) Self-checks: (10 out of 10 pts)
    Comments: All the code paths were traversed by the given tests cases in the grading guidelines.

Missing required section(s) in README file (vm-README.txt): -0
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

Equal share by all the team members. 

+------------------+
| OTHER (Optional) |
+------------------+

Special DBG setting in Config.mk for certain tests: None, as per grading guidelines
Comments on deviation from spec (you will still lose points, but it's better to let the grader know): None
General comments on design decisions: As per kernel 3 specifications and grading guidelines

