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

/*
 *  FILE: open.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Mon Apr  6 19:27:49 1998
 */

#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int
get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++) {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_VFS, "ERROR: get_empty_fd: out of file descriptors "
            "for pid %d\n", curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND or O_CREAT.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        oflags is not valid.
 *      o EMFILE
 *        The process already has the maximum number of files open.
 *      o ENOMEM
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG
 *        A component of filename was too long.
 *      o ENOENT
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */

int
do_open(const char *filename, int oflags)
{
    int fd = get_empty_fd(curproc);
    file_t *fresh = fget(-1);
    curproc->p_files[fd] = fresh;
    int mode =0;
    if(oflags == O_WRONLY){
          mode = FMODE_WRITE;
          dbg(DBG_PRINT, "(GRADING2B), Opening file for Write Only\n");
    }
    else if(oflags == (O_WRONLY | O_CREAT)){
          dbg(DBG_PRINT, "(GRADING2B), Opening file for Write Only and O_CREAT Flag\n");
          mode = FMODE_WRITE;
    }
    else if(oflags == O_RDONLY){
          mode = FMODE_READ;
          dbg(DBG_PRINT, "(GRADING2B), Opening file for Read only\n");
    }
    else if(oflags == (O_RDONLY | O_CREAT)){
          dbg(DBG_PRINT, "(GRADING2B), Opening file for Read only and O_CREAT\n");
          mode = FMODE_READ;
    }
    else if(oflags == O_RDWR){
          dbg(DBG_PRINT, "(GRADING2B), Opening file for READ and WRITE\n");
          mode = FMODE_READ | FMODE_WRITE;
    }
    else if(oflags == (O_RDWR | O_CREAT)){
          dbg(DBG_PRINT, "(GRADING2B), Opening file for READ and WRITE and O_CREAT\n");
          mode = FMODE_READ | FMODE_WRITE;
    }
    else if(oflags == (O_RDWR | O_APPEND)){
          dbg(DBG_PRINT, "(GRADING2B), Opening file for read and write with O_CREAT and O_APPEND\n");
          mode = FMODE_READ | FMODE_WRITE | FMODE_APPEND;
    }else{
          dbg(DBG_PRINT, "(GRADING2B), Invalid flag Input\n");
          curproc->p_files[fd] = NULL;
          fput(fresh);
          return -EINVAL;
    }
    fresh->f_mode=mode;
    int ret = open_namev(filename,oflags,&fresh->f_vnode,NULL);
    if(ret != 0){
        dbg(DBG_PRINT, "(GRADING2B), returning with Open namev failure %d\n",ret);
        curproc->p_files[fd] = 0;
        fput(fresh);
        return ret;
    }
    if (S_ISDIR(fresh->f_vnode->vn_mode)) {
        dbg(DBG_PRINT, "(GRADING2B), Open namev returned with a directory\n");
        if (oflags & O_RDWR || oflags & O_WRONLY) {
            dbg(DBG_PRINT, "(GRADING2B), Input file is Directory. Can only be read. \n");
            curproc->p_files[fd] = 0;
            fput(fresh);
            return -EISDIR;       
        }
    }
    fresh->f_pos = 0;
    dbg(DBG_PRINT, "(GRADING2B), f_pos is set to 0  \n");
    return fd;
}
