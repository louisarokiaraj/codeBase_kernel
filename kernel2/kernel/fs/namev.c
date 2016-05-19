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

#include "kernel.h"
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function, but you may want to special case
 * "." and/or ".." here depnding on your implementation.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
        KASSERT(NULL != name);
        dbg(DBG_PRINT, "(GRADING2A 2.a), Input Name is not NULL\n");       
        KASSERT(NULL != dir);
        dbg(DBG_PRINT, "(GRADING2A 2.a), Input Directory is not NULL\n");
        if (len > NAME_LEN) {
                dbg(DBG_PRINT, "(GRADING2D), Given name larger than NAME_LEN in Lookup\n");
                return -ENAMETOOLONG;
        }
        if(!S_ISDIR(dir->vn_mode)){
                dbg(DBG_PRINT, "(GRADING2D), Not a directory inside lookup.\n");
                return -ENOTDIR;
        }
        if(len == 0){
                dbg(DBG_PRINT, "(GRADING2B), Given name is present working directory !!\n");
                *result = vget(dir->vn_fs,dir->vn_vno);
                return 0;
        }
        int val = dir->vn_ops->lookup(dir,name,len,result);
        KASSERT(NULL != result);
        dbg(DBG_PRINT, "(GRADING2A 2.a), Lookup result is Not NULL\n");
        return val;
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
        KASSERT(NULL != pathname);
        dbg(DBG_PRINT, "(GRADING2A 2.b), Input pathname is not NULL\n");
        KASSERT(NULL != namelen);
        dbg(DBG_PRINT, "(GRADING2A 2.b), Input namelen is not NULL\n");
        KASSERT(NULL != name);
        dbg(DBG_PRINT, "(GRADING2A 2.b), Input name is not NULL\n");
        KASSERT(NULL != res_vnode);
        dbg(DBG_PRINT, "(GRADING2A 2.b), Input res_vnode is not NULL\n");
        vnode_t *parent;
        int end_ptr=0;
        int start_ptr=0;
        if (base == NULL) {
                dbg(DBG_PRINT,"(GRADING2B), Base equal to NULL\n");
                parent=curproc->p_cwd;
        }
        if (pathname[0]=='/') {
                dbg(DBG_PRINT, "(GRADING2B), Pathname[0] is equal to '/' in dir_namev\n");
                parent=vfs_root_vn;
                start_ptr=1;
        }
        while(1) {
            while(pathname[end_ptr+start_ptr]!='/') {
                if (pathname[end_ptr+start_ptr]=='\0') {
                    if (end_ptr > NAME_LEN) {
                        dbg(DBG_PRINT, "(GRADING2B), File name too long\n");
                        return -ENAMETOOLONG;
                    }
                    if(S_ISDIR(parent->vn_mode)) {
                        dbg(DBG_PRINT, "(GRADING2B), Its a directory ! Going on !\n");
                        *res_vnode = vget(parent->vn_fs,parent->vn_vno);
                        KASSERT(NULL != *res_vnode);
                        dbg(DBG_PRINT, "(GRADING2A 2.b), Pointer to res_vnode of vnode is not NULL\n");
                        KASSERT(NULL != namelen);
                        dbg(DBG_PRINT, "(GRADING2A 2.b), Pointer to namelen of vnode is not NULL\n");
                        KASSERT(NULL != name);
                        dbg(DBG_PRINT, "(GRADING2A 2.b), Pointer to name of vnode is not NULL\n");
                        *namelen=end_ptr;
                        *name=&pathname[start_ptr];
                        dbg(DBG_PRINT, "(GRADING2B), Returning from dir_namev with success\n");
                        return 0;
                    }else{
                        dbg(DBG_PRINT, "(GRADING2B), Not a directory Failed in dir_namev\n");
                        return -ENOTDIR;
                    }
                }
                end_ptr++;
            }            
            int lookval = lookup(parent,&pathname[start_ptr],end_ptr,res_vnode);
            if (lookval==0) {
                dbg(DBG_PRINT, "(GRADING2B), Parent is a directory\n");
                parent=*res_vnode;
                vput(*res_vnode);
                start_ptr=start_ptr+end_ptr+1;
                end_ptr=0;
            }else{
                dbg(DBG_PRINT,"(GRADING2B), Lookup failed in dir_namev\n");
                return lookval;
            }
        }
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified, and the file does
 * not exist call create() in the parent directory vnode.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
        size_t namelen = 0;
        const char *name = NULL;
        vnode_t *result = NULL;
        int ret_dir = dir_namev(pathname,&namelen,&name,base,&result);
        if(ret_dir != 0){
          dbg(DBG_PRINT, "(GRADING2B), Error in dir_namev from Open_namev %d\n",ret_dir);
          return ret_dir;
        }
        int ret_lookup = lookup(result,name,namelen,res_vnode);
        if(ret_lookup == -ENOENT){
            dbg(DBG_PRINT, "(GRADING2B), Lookup returned with file does not exist\n");
            if((ret_lookup == -ENOENT) && (flag & O_CREAT)){
                dbg(DBG_PRINT, "(GRADING2B), Lookup returned with file does not exist and O_CREAT flag is set\n");
                KASSERT(NULL != result->vn_ops->create);
                dbg(DBG_PRINT, "(GRADING2A 2.c), File Does not exist, calling create\n");
                dbg(DBG_PRINT, "(GRADING2A 2B), File Does not exist, calling create\n");
                int ret_create = result->vn_ops->create(result,name,namelen,res_vnode);
                if (ret_create < 0) {
                    dbg(DBG_PRINT, "(GRADING2B), O_CREAT failed Returning!!\n");
                    vput(result);
                    return ret_create;
                }
            }else{
                dbg(DBG_PRINT, "(GRADING2B), Flag is not O_CREAT\n");
                vput(result);
                return ret_lookup;
            }
        }else{
          dbg(DBG_PRINT,"(GRADING2B), Lookup returned with error other than -ENOENT %d\n",ret_lookup);
          vput(result);
          return ret_lookup;
        }
        dbg(DBG_PRINT,"(GRADING2B), Returning from Open_namev , file already exists\n");
        vput(result);
        return ret_dir;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");
        return -ENOENT;
}
#endif /* __GETCWD__ */
