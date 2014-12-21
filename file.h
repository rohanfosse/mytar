#ifndef FILE_H
#define FILE_H
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>
extern int errno;
#define BUF_SIZE 8192
/* Commands */
#define TAR_CREATE 1
#define TAR_EXTRACT 2
#define TAR_LIST 3
#define TAR_UPDATE 4
#define TAR_APPEND 5
/* Flags */
#define TAR_VERBOSE 1
#define TAR_FILE 1<<1
#define TAR_FOLLOW_SYMLINKS 1<<2
int tar_flag;
int tar_cmd;
#ifndef _POSIX_PATH_MAX
# define _POSIX_PATH_MAX 256
#endif
/* Structure to be stored in a tarball for each file */

typedef struct
{
struct stat st;
char path[_POSIX_PATH_MAX+1];
}
file_struct;
#endif
