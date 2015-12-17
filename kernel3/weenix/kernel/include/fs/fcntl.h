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

/* fcntl.h - File access bits
 * mcc, jal
 */

#pragma once

/* Kernel and user header (via symlink) */

/* File access modes for open(). */
#define O_RDONLY        0
#define O_WRONLY        1
#define O_RDWR          2

/* File status flags for open(). */
#define O_CREAT         0x100   /* Create file if non-existent. */
#define O_TRUNC         0x200   /* Truncate to zero length. */
#define O_APPEND        0x400   /* Append to file. */
