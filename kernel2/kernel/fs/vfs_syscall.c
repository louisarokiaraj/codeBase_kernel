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
   *  FILE: vfs_syscall.c
   *  AUTH: mcc | jal
   *  DESC:
   *  DATE: Wed Apr  8 02:46:19 1998
   *  $Id: vfs_syscall.c,v 1.10 2014/12/22 16:15:17 william Exp $
   */

  #include "kernel.h"
  #include "errno.h"
  #include "globals.h"
  #include "fs/vfs.h"
  #include "fs/file.h"
  #include "fs/vnode.h"
  #include "fs/vfs_syscall.h"
  #include "fs/open.h"
  #include "fs/fcntl.h"
  #include "fs/lseek.h"
  #include "mm/kmalloc.h"
  #include "util/string.h"
  #include "util/printf.h"
  #include "fs/stat.h"
  #include "util/debug.h"

  /* To read a file:
   *      o fget(fd)
   *      o call its virtual read fs_op
   *      o update f_pos
   *      o fput() it
   *      o return the number of bytes read, or an error
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EBADF
   *        fd is not a valid file descriptor or is not open for reading.
   *      o EISDIR
   *        fd refers to a directory.
   *
   * In all cases, be sure you do not leak file refcounts by returning before
   * you fput() a file that you fget()'ed.
   */
  int
  do_read(int fd, void *buf, size_t nbytes)
  {
      file_t *f;   
      int retval; 
      if(fd<0 || fd>NFILES){
          dbg(DBG_PRINT, "(GRADING2B), Invalid File descriptor to read the file\n");
          return -EBADF;
      }if (curproc->p_files[fd]!=NULL){
          dbg(DBG_PRINT, "(GRADING2B), Checking file descriptor valid or not. It is VALID\n");
          f = fget(fd); 
          if ((f->f_mode & FMODE_READ) == FMODE_READ){
              dbg(DBG_PRINT, "(GRADING2B), Checking file descriptor mode. It is in Read Mode\n");
              if ((f->f_vnode->vn_mode)!=S_IFDIR){
                  dbg(DBG_PRINT, "(GRADING2B), checking File descriptor for Directory. It is not a directory\n");
                  int bytesread=f->f_vnode->vn_ops->read(f->f_vnode, f->f_pos, buf, nbytes);
                  f->f_pos+=bytesread;
                  retval=bytesread; 
              }else{ 
                  dbg(DBG_PRINT, "(GRADING2B), Invalid File descriptor to read the file, !! it is a Directory\n");
                  retval = -EISDIR;
              }
          }else{ 
              dbg(DBG_PRINT, "(GRADING2B), Invalid File descriptor to read the file, Permission Denied\n");
              retval = -EBADF;
          }
      fput(f);
      }else{
          dbg(DBG_PRINT, "(GRADING2B), Invalid File descriptor to read the file! \n");
          retval = -EBADF;  
      }
      dbg(DBG_PRINT, "(GRADING2B), returning the Bytes read !\n");
      return retval;      
  }

  /* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
   * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
   * fs_op, and fput the file.  As always, be mindful of refcount leaks.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EBADF
   *        fd is not a valid file descriptor or is not open for writing.
   */
  int
  do_write(int fd, const void *buf, size_t nbytes)
  {

      file_t *f=NULL;
      int retval;
      int ret_seek=0;
      if(!(fd<0 || fd>NFILES)){
          dbg(DBG_PRINT, "(GRADING2B), Checking file descriptor valid or not. It is VALID\n");
          if(curproc->p_files[fd]!=NULL){
              dbg(DBG_PRINT, "(GRADING2B), File descriptor is VALID\n");
              f = fget(fd);
              if((f->f_mode & FMODE_WRITE)==FMODE_WRITE){
                    dbg(DBG_PRINT, "(GRADING2B), Checking file descriptor mode. It is Write Mode\n");
                    if (f->f_mode & FMODE_APPEND){
                          dbg(DBG_PRINT, "(GRADING2B), Appending to End of the File\n");
                          ret_seek=do_lseek(fd,0,SEEK_END);
                          f->f_pos = ret_seek;
                    }
                    int byteswritten=f->f_vnode->vn_ops->write(f->f_vnode, f->f_pos, buf, nbytes);
                    if(byteswritten >=0){
                          KASSERT((S_ISCHR(f->f_vnode->vn_mode)) ||(S_ISBLK(f->f_vnode->vn_mode)) ||((S_ISREG(f->f_vnode->vn_mode)) && (f->f_pos <= f->f_vnode->vn_len)));
                          dbg(DBG_PRINT, "(GRADING2A 3.a), Updating the cursor postion in the File post Writing\n");
                          f->f_pos += byteswritten;  
                    }
                    fput(f);
                    dbg(DBG_PRINT, "(GRADING2B), Returning the Bytes Written\n");
                    return byteswritten;
              }else{
                  dbg(DBG_PRINT, "(GRADING2B), Invalid File Descriptor, No write Permissions \n");
                  fput(f);
                  return -EBADF;
              }
          }else{  
              dbg(DBG_PRINT, "(GRADING2B), Invalid File Descriptor\n");
              return -EBADF;
          }
      }else{
          dbg(DBG_PRINT, "(GRADING2B), Invalid File Descriptor. Not a positive Number\n");
          return -EBADF;   
      }
  }

  /*
   * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EBADF
   *        fd isn't a valid open file descriptor.
   */
  int
  do_close(int fd)
  {
      if ((fd >= 0 && fd < NFILES) && (curproc->p_files[fd] != NULL)){
          dbg(DBG_PRINT, "(GRADING2B), File Descriptor is Valid to close\n");
          file_t* f;
          f = curproc->p_files[fd];
          curproc->p_files[fd] = NULL;
          fput(f);
          return 0;
      }else{
          dbg(DBG_PRINT, "(GRADING2B), Invalid File Descriptor to close\n");
          return -EBADF;
      }
  }

  /* To dup a file:
   *      o fget(fd) to up fd's refcount
   *      o get_empty_fd()
   *      o point the new fd to the same file_t* as the given fd
   *      o return the new file descriptor
   *
   * Don't fput() the fd unless something goes wrong.  Since we are creating
   * another reference to the file_t*, we want to up the refcount.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EBADF
   *        fd isn't an open file descriptor.
   *      o EMFILE
   *        The process already has the maximum number of file descriptors open
   *        and tried to open a new one.
   */
  int
  do_dup(int fd)
  {
      if ((fd >= 0 && fd < NFILES) && (curproc->p_files[fd] != NULL)){
          dbg(DBG_PRINT, "(GRADING2B), Checking File Descriptor validity to Duplicate. It is Valid\n");
          int new_fd = get_empty_fd(curproc);
              file_t *f= fget(fd);
              curproc->p_files[new_fd] = f;
              dbg(DBG_PRINT, "(GRADING2B), returning New File Descriptor. Duplicate FD for %d is %d\n",fd,new_fd);
              return new_fd;
          }else{
              dbg(DBG_PRINT, "(GRADING2B), Invalid File Descriptor to Duplicate\n");
              return -EBADF;
      }
  }

  /* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
   * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
   * do_close() it first.  Then return the new file descriptor.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EBADF
   *        ofd isn't an open file descriptor, or nfd is out of the allowed
   *        range for file descriptors.
   */
  int
  do_dup2(int ofd, int nfd)
  {
        if ((nfd < 0 || nfd >= NFILES) || (ofd < 0 || ofd >= NFILES)) {
            dbg(DBG_PRINT, "(GRADING2B), Invalid File Descriptor to Duplicate \n");
            return -EBADF;
        }
        file_t* old_f = fget(ofd);
        if (old_f == NULL) {
            dbg(DBG_PRINT, "(GRADING2B), Input Old FD is invalid File descriptor\n");
            return -EBADF;
        }
        if (ofd == nfd) {
            dbg(DBG_PRINT, "(GRADING2B), ofd and nfd are same. Returning NFD\n");
            fput(old_f);
            return nfd;
        }
        file_t* new_f = fget(nfd);
        if (new_f != NULL) {
            dbg(DBG_PRINT, "(GRADING2B), NFS is already in use. Hence closing it.\n");
            fput(new_f);
            do_close(nfd);
        }
        curproc->p_files[nfd] = old_f;
        dbg(DBG_PRINT, "(GRADING2B), Returning the New File descriptor\n");
        return nfd;
  }

  /*
   * This routine creates a special file of the type specified by 'mode' at
   * the location specified by 'path'. 'mode' should be one of S_IFCHR or
   * S_IFBLK (you might note that mknod(2) normally allows one to create
   * regular files as well-- for simplicity this is not the case in Weenix).
   * 'devid', as you might expect, is the device identifier of the device
   * that the new special file should represent.
   *
   * You might use a combination of dir_namev, lookup, and the fs-specific
   * mknod (that is, the containing directory's 'mknod' vnode operation).
   * Return the result of the fs-specific mknod, or an error.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EINVAL
   *        mode requested creation of something other than a device special
   *        file.
   *      o EEXIST
   *        path already exists.
   *      o ENOENT
   *        A directory component in path does not exist.
   *      o ENOTDIR
   *        A component used as a directory in path is not, in fact, a directory.
   *      o ENAMETOOLONG
   *        A component of path was too long.
   */
  int
  do_mknod(const char *path, int mode, unsigned devid)
  {
        if(strlen(path) > MAXPATHLEN){
            dbg(DBG_PRINT, "(GRADING2D), Invalid path. Length is too long than %d\n",MAXPATHLEN);
            return -ENAMETOOLONG;
        }
        if(mode != S_IFCHR && mode != S_IFBLK){
            dbg(DBG_PRINT, "(GRADING2D), Checking mode. mknod mode is not S_IFCHR or S_IFBLK\n");
            return -EINVAL;
        }
        size_t namelen;
        const char *name;
        vnode_t *res_vnode;
        int ret_dir =  dir_namev(path, &namelen, &name,NULL, &res_vnode);
        if(ret_dir == 0){
            dbg(DBG_PRINT, "(GRADING2B), dir_namev returned with success\n");
            vnode_t *result;
            int ret_lookup = lookup(res_vnode, name, namelen, &result);
            if(ret_lookup == 0){
                dbg(DBG_PRINT, "(GRADING2D), lookup returned with success\n");
                vput(result);
                vput(res_vnode);
                dbg(DBG_PRINT, "(GRADING2D), node already exists\n");
                return -EEXIST;
            }else if (ret_lookup == -ENOENT){
                dbg(DBG_PRINT, "(GRADING2B), lookup returned with file not exist\n");
                KASSERT(NULL != res_vnode->vn_ops->mknod);
                dbg(DBG_PRINT, "(GRADING2A 3.b), mknod function exists\n");
                int ret_mknod = res_vnode->vn_ops->mknod(res_vnode, name, namelen, mode, devid);
                vput(res_vnode);
                return ret_mknod;
            }
        }else{
            dbg(DBG_PRINT, "(GRADING2B), dir_namev returned with error %d\n",ret_dir);
            return ret_dir;
        }
      return ret_dir;
  }

  /* Use dir_namev() to find the vnode of the dir we want to make the new
   * directory in.  Then use lookup() to make sure it doesn't already exist.
   * Finally call the dir's mkdir vn_ops. Return what it returns.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EEXIST
   *        path already exists.
   *      o ENOENT
   *        A directory component in path does not exist.
   *      o ENOTDIR
   *        A component used as a directory in path is not, in fact, a directory.
   *      o ENAMETOOLONG
   *        A component of path was too long.
   */
  int
  do_mkdir(const char *path)
  {
          if(path == NULL){
              dbg(DBG_PRINT, "(GRADING2D)  Path Missing \n");
              return -EINVAL;
          }
          if(strlen(path) > MAXPATHLEN){
              dbg(DBG_PRINT, "(GRADING2D) Path length greater than %d\n", MAXPATHLEN );
              return -ENAMETOOLONG;
          }
          size_t namelen;
          const char *name;
          vnode_t *res_vnode;
          int ret_dir =  dir_namev(path, &namelen, &name,NULL, &res_vnode);
          if(ret_dir<0){
              dbg(DBG_PRINT, "(GRADING2B) Error in dir_namev");
              return ret_dir;
          }
          vnode_t *result;
          int ret_lookup = lookup(res_vnode, name, namelen, &result);
          if(ret_lookup == 0){
              vput(result);
              vput(res_vnode);
              dbg(DBG_PRINT, "(GRADING2B) directory already exists %d\n", ret_lookup);
              return -EEXIST;
          }
          KASSERT(NULL != res_vnode->vn_ops->mkdir);
          dbg(DBG_PRINT, "(GRADING2A 3.c), mkdir function exists\n"); 
          int ret_mkdir = res_vnode->vn_ops->mkdir(res_vnode, name, namelen);
          vput(res_vnode);
          return ret_mkdir;
  }

  /* Use dir_namev() to find the vnode of the directory containing the dir to be
   * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
   * return an error if the dir to be removed does not exist or is not empty, so
   * you don't need to worry about that here. Return the value of the v_op,
   * or an error.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EINVAL
   *        path has "." as its final component.
   *      o ENOTEMPTY
   *        path has ".." as its final component.
   *      o ENOENT
   *        A directory component in path does not exist.
   *      o ENOTDIR
   *        A component used as a directory in path is not, in fact, a directory.
   *      o ENAMETOOLONG
   *        A component of path was too long.
   */

  int
  do_rmdir(const char *path)
  {
        size_t namelen;
        const char *name;
        vnode_t *res_vnode;
        if(path == NULL){
            dbg(DBG_PRINT, "(GRADING2D) Path length greater missing\n");
            return -EINVAL;
        }
        if(strlen(path) > MAXPATHLEN){
            dbg(DBG_PRINT, "(GRADING2D)  Path length greater than %d\n", MAXPATHLEN );
            return -ENAMETOOLONG;
        }
        int ret_dir =  dir_namev(path, &namelen, &name,NULL, &res_vnode);
        if(ret_dir <0){
            dbg(DBG_PRINT, "(GRADING2B)  dirnamev says Not a directory !!\n" );
            return ret_dir;
        }
        if(strcmp(name, "." ) == 0){
            dbg(DBG_PRINT, "(GRADING2B)  Invalid name !!\n" );
            vput(res_vnode);
            return -EINVAL;
        }
        if(strcmp(name, ".." ) == 0){
            dbg(DBG_PRINT, "(GRADING2B) Directory not Empty !! Path has .. as final component\n" );
            vput(res_vnode);
            return -ENOTEMPTY;
        }

        vnode_t *result_lookup;
        int ret_lookup = lookup(res_vnode,name,namelen,&result_lookup);
        if(ret_lookup == 0){
            if(S_ISDIR(result_lookup->vn_mode)){
                KASSERT(NULL != res_vnode->vn_ops->rmdir);
                dbg(DBG_PRINT, "(GRADING2A 3.d), rmdir function exists\n");
                dbg(DBG_PRINT, "(GRADING2A), rmdir function exists\n");
                int ret_rmdir = result_lookup->vn_ops->rmdir(res_vnode, name, namelen);
                vput(result_lookup);
                vput(res_vnode);
                return ret_rmdir;
             }else{
                dbg(DBG_PRINT, "(GRADING2B) lookup says Not a directory !!\n" );
                vput(res_vnode);
                vput(result_lookup);
                return -ENOTDIR;
            }
        }else{
            dbg(DBG_PRINT, "(GRADING2B) Looking returned with error %d\n",ret_lookup);
            vput(res_vnode);
            return ret_lookup;
        }
}

  /*
   * Same as do_rmdir, but for files.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EISDIR
   *        path refers to a directory.
   *      o ENOENT
   *        A component in path does not exist.
   *      o ENOTDIR
   *        A component used as a directory in path is not, in fact, a directory.
   *      o ENAMETOOLONG
   *        A component of path was too long.
   */
  int
  do_unlink(const char *path)
  {
      if(strlen(path) > MAXPATHLEN){
          dbg(DBG_PRINT, "(GRADING2D)  Path length greater than %d\n", MAXPATHLEN );
          return -ENAMETOOLONG;
      }
      size_t namelen;
      const char *name;
      vnode_t *res_vnode;

      int ret_dir =  dir_namev(path, &namelen, &name,NULL, &res_vnode);
          dbg(DBG_PRINT, "(GRADING2B)  directory vnode is returned\n");
          vnode_t *result;
          int ret_lookup = lookup(res_vnode, name, namelen, &result);
          if(ret_lookup == 0){
              dbg(DBG_PRINT, "(GRADING2B)  file with 'name'found in lookup\n");
              if(S_ISDIR(result->vn_mode)){
                  dbg(DBG_PRINT, "(GRADING2B)  file with 'name',found in lookup is a directory\n");
                  vput(res_vnode);
                  vput(result);
                  return -EISDIR;
              }
              vput(result);
              KASSERT(NULL != res_vnode->vn_ops->unlink);
              dbg(DBG_PRINT, "(GRADING2A 3.e), unlink function exists\n");
              dbg(DBG_PRINT, "(GRADING2A), unlink function exists\n");
              int ret_unlink = res_vnode->vn_ops->unlink(res_vnode, name, namelen);
              vput(res_vnode);
              return ret_unlink;
          }else{
              dbg(DBG_PRINT, "(GRADING2B) file with 'name' not found in lookup !!\n");
              vput(res_vnode);
              return ret_lookup;
          }
  }

  /* To link:
   *      o open_namev(from)
   *      o dir_namev(to)
   *      o call the destination dir's (to) link vn_ops.
   *      o return the result of link, or an error
   *
   * Remember to vput the vnodes returned from open_namev and dir_namev.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EEXIST
   *        to already exists.
   *      o ENOENT
   *        A directory component in from or to does not exist.
   *      o ENOTDIR
   *        A component used as a directory in from or to is not, in fact, a
   *        directory.
   *      o ENAMETOOLONG
   *        A component of from or to was too long.
   *      o EISDIR
   *        from is a directory.
   */
  int
  do_link(const char *from, const char *to)
  {
          NOT_YET_IMPLEMENTED("VFS: do_link");
          return -1;
  }

  /*      o link newname to oldname
   *      o unlink oldname
   *      o return the value of unlink, or an error
   *
   * Note that this does not provide the same behavior as the
   * Linux system call (if unlink fails then two links to the
   * file could exist).
   */
  int
  do_rename(const char *oldname, const char *newname)
  {
          NOT_YET_IMPLEMENTED("VFS: do_rename");
          return -1;
  }

  /* Make the named directory the current process's cwd (current working
   * directory).  Don't forget to down the refcount to the old cwd (vput()) and
   * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
   * success.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o ENOENT
   *        path does not exist.
   *      o ENAMETOOLONG
   *        A component of path was too long.
   *      o ENOTDIR
   *        A component of path is not a directory.
   */
  int
  do_chdir(const char *path)
  {
        vnode_t *res_vnode;
        int flag = 0;
        int retval1 = open_namev(path,flag,&res_vnode,NULL);
        if(retval1 == 0){
            dbg(DBG_PRINT, "(GRADING2B) got the vnode\n");
            if(S_ISDIR(res_vnode->vn_mode)){
                dbg(DBG_PRINT, "(GRADING2B)  vnode is a directory\n");
                vput(curproc->p_cwd);
                curproc->p_cwd = res_vnode;
            }else{
                dbg(DBG_PRINT, "(GRADING2B) It is not a directory\n");
                vput(res_vnode);
                return -ENOTDIR;
            }
        }
        dbg(DBG_PRINT, "(GRADING2B) Open namev returned with error %d\n",retval1);
        return retval1;
  }

  /* Call the readdir fs_op on the given fd, filling in the given dirent_t*.
   * If the readdir fs_op is successful, it will return a positive value which
   * is the number of bytes copied to the dirent_t.  You need to increment the
   * file_t's f_pos by this amount.  As always, be aware of refcounts, check
   * the return value of the fget and the virtual function, and be sure the
   * virtual function exists (is not null) before calling it.
   *
   * Return either 0 or sizeof(dirent_t), or -errno.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EBADF
   *        Invalid file descriptor fd.
   *      o ENOTDIR
   *        File descriptor does not refer to a directory.
   */
  int
  do_getdent(int fd, struct dirent *dirp)
  {
          file_t *f_ptr;
          off_t dir_off = 0;
          if(fd < 0 || fd > NFILES){
                 dbg(DBG_PRINT, "(GRADING2B)  Invalid File descriptor !\n");
                 return -EBADF;
          }
          if(curproc->p_files[fd]==NULL){
                 dbg(DBG_PRINT, "(GRADING2B)  Invalid File descriptor !\n");
                 return -EBADF;
          }
          f_ptr = fget(fd);
          if(!S_ISDIR(f_ptr->f_vnode->vn_mode)){
                dbg(DBG_PRINT, "(GRADING2B) In getdent, fd does not belong to a dir !\n");
                 fput(f_ptr);
                 return -ENOTDIR;
          }
          dir_off = f_ptr->f_vnode->vn_ops->readdir(f_ptr->f_vnode, f_ptr->f_pos,dirp);
          if(dir_off <= 0){
                 dbg(DBG_PRINT, "(GRADING2B)  dir entry offset is <= 0 !\n");
                fput(f_ptr);
                return 0;
          }else{
                dbg(DBG_PRINT, "(GRADING2B) dir entry offset > 0, is added to f_pos\n");
                f_ptr->f_pos = f_ptr->f_pos+dir_off;
                fput(f_ptr);
                return (int)sizeof(*dirp);
          }
  }

  /*
   * Modify f_pos according to offset and whence.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o EBADF
   *        fd is not an open file descriptor.
   *      o EINVAL
   *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
   *        file offset would be negative.
   */
  int
  do_lseek(int fd, int offset, int whence)
  {
          if(fd < 0 || fd > NFILES){
          dbg(DBG_PRINT, "(GRADING2B) Invalid File descriptor !\n");
          return -EBADF;
          }

          if(curproc->p_files[fd]==NULL){
          dbg(DBG_PRINT, "(GRADING2B) Invalid File descriptor !\n");
          return -EBADF;
          }

          if(whence != SEEK_END && whence != SEEK_CUR && whence != SEEK_SET){
          dbg(DBG_PRINT, "(GRADING2B)  Invalid Input !\n");
          return -EINVAL;
          }

          switch(whence){
          case SEEK_SET:
                  dbg(DBG_PRINT,"(GRADING2B)  whence is SEEK_SET \n");
                  curproc->p_files[fd]->f_pos=offset;
                  if(curproc->p_files[fd]->f_pos<0){
                          curproc->p_files[fd]->f_pos =0;
                          dbg(DBG_PRINT, "(GRADING2B)  resulting file offset is negative ! Returing from do_lseek with ERROR\n");
                          return -EINVAL;
                  }
                  break;
          case SEEK_CUR:
                  dbg(DBG_PRINT,"(GRADING2B)  whence is SEEK_CUR \n");
                  curproc->p_files[fd]->f_pos+=offset;
                  if(curproc->p_files[fd]->f_pos<0){
                          curproc->p_files[fd]->f_pos = 0;
                          dbg(DBG_PRINT, "(GRADING2B) resulting file offset is negative ! Returing from do_lseek with ERROR\n");
                          return -EINVAL;
                  }
                  break;
          case SEEK_END:
                  dbg(DBG_PRINT,"(GRADING2B)  whence is SEEK_END \n");
                  curproc->p_files[fd]->f_pos=curproc->p_files[fd]->f_vnode->vn_len + offset;
                  if(curproc->p_files[fd]->f_pos<0){
                          curproc->p_files[fd]->f_pos = 0;
                          dbg(DBG_PRINT, "(GRADING2B) resulting file offset is negative ! Returing from do_lseek with ERROR\n");
                          return -EINVAL;
                  }
                  break;
          }
          dbg(DBG_PRINT, "(GRADING2B) Returning lseek. without any error\n");
          return curproc->p_files[fd]->f_pos;
  }

  /*
   * Find the vnode associated with the path, and call the stat() vnode operation.
   *
   * Error cases you must handle for this function at the VFS level:
   *      o ENOENT
   *        A component of path does not exist.
   *      o ENOTDIR
   *        A component of the path prefix of path is not a directory.
   *      o ENAMETOOLONG
   *        A component of path was too long.
   */
  int
  do_stat(const char *path, struct stat *buf)
  {
          int lookup_out = 0;
          int stat_out=0;
          const char *f_name;
          vnode_t *vnode_out_dir;
          vnode_t *vnode_out_lookup;
          size_t f_len = 0;
          if(*path == 0){
              dbg(DBG_PRINT,"(GRADING2B) Path is null\n");
              return -EINVAL;
          }
          if(strlen(path) > MAXPATHLEN){
              dbg(DBG_PRINT, "(GRADING2D)  Path length greater than %d\n", MAXPATHLEN );
              return -ENAMETOOLONG;
          }
          int dirNamev_out = dir_namev(path, &f_len, &f_name,NULL,&vnode_out_dir);
          lookup_out = lookup(vnode_out_dir,f_name,f_len,&vnode_out_lookup);
          if(lookup_out==0){
              dbg(DBG_PRINT,"(GRADING2B) lookup on dir is successful. \n");
              KASSERT(vnode_out_dir->vn_ops->stat);
              dbg(DBG_PRINT, "(GRADING2A 3.f),stat function exists\n");
              dbg(DBG_PRINT, "(GRADING2A),stat function exists\n");
              stat_out = vnode_out_dir->vn_ops->stat(vnode_out_lookup,buf);
              vput(vnode_out_lookup);
              vput(vnode_out_dir);
              return stat_out;
          }else{
              dbg(DBG_PRINT,"(GRADING2B) lookup on dir failed!! \n");
              vput(vnode_out_dir);
              return lookup_out;
          }
  }

  #ifdef __MOUNTING__
  /*
   * Implementing this function is not required and strongly discouraged unless
   * you are absolutely sure your Weenix is perfect.
   *
   * This is the syscall entry point into vfs for mounting. You will need to
   * create the fs_t struct and populate its fs_dev and fs_type fields before
   * calling vfs's mountfunc(). mountfunc() will use the fields you populated
   * in order to determine which underlying filesystem's mount function should
   * be run, then it will finish setting up the fs_t struct. At this point you
   * have a fully functioning file system, however it is not mounted on the
   * virtual file system, you will need to call vfs_mount to do this.
   *
   * There are lots of things which can go wrong here. Make sure you have good
   * error handling. Remember the fs_dev and fs_type buffers have limited size
   * so you should not write arbitrary length strings to them.
   */
  int
  do_mount(const char *source, const char *target, const char *type)
  {
          NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
          return -EINVAL;
  }

  /*
   * Implementing this function is not required and strongly discouraged unless
   * you are absolutley sure your Weenix is perfect.
   *
   * This function delegates all of the real work to vfs_umount. You should not worry
   * about freeing the fs_t struct here, that is done in vfs_umount. All this function
   * does is figure out which file system to pass to vfs_umount and do good error
   * checking.
   */
  int
  do_umount(const char *target)
  {
          NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
          return -EINVAL;
  }
  #endif
