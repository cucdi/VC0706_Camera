#ifndef filelib
#define filelib

#include "mbed.h"
#include <time.h>
#include <ff.h> //found in lib SDFileSystem->FATFileSystem/ChaN
#include "SDFileSystem.h"


/*
typedef enum {
    FR_OK = 0,              // (0) Succeeded
    FR_DISK_ERR,            // (1) A hard error occurred in the low level disk I/O layer
    FR_INT_ERR,             // (2) Assertion failed
    FR_NOT_READY,           // (3) The physical drive cannot work
    FR_NO_FILE,             // (4) Could not find the file
    FR_NO_PATH,             // (5) Could not find the path
    FR_INVALID_NAME,        // (6) The path name format is invalid
    FR_DENIED,              // (7) Access denied due to prohibited access or directory full
    FR_EXIST,               // (8) Access denied due to prohibited access
    FR_INVALID_OBJECT,      // (9) The file/directory object is invalid
    FR_WRITE_PROTECTED,     // (10) The physical drive is write protected
    FR_INVALID_DRIVE,       // (11) The logical drive number is invalid
    FR_NOT_ENABLED,         // (12) The volume has no work area
    FR_NO_FILESYSTEM,       // (13) There is no valid FAT volume
    FR_MKFS_ABORTED,        // (14) The f_mkfs() aborted due to any parameter error
    FR_TIMEOUT,             // (15) Could not get a grant to access the volume within defined period
    FR_LOCKED,              // (16) The operation is rejected according to the file sharing policy
    FR_NOT_ENOUGH_CORE,     // (17) LFN working buffer could not be allocated
    FR_TOO_MANY_OPEN_FILES, // (18) Number of open files > _FS_SHARE
    FR_INVALID_PARAMETER    // (19) Given parameter is invalid
} FRESULT;

*/
/*
mode_t values in octal
       S_IFREG       0100000
       S_IFDIR        040000
       S_IRUSR          0400    read permission, owner
       S_IWUSR          0200    write permission, owner
       S_IXUSR          0100    execute/search permission, owner
       S_IRGRP           040    read permission, group
       S_IWGRP           020    write permission, group
       S_IXGRP           010    execute/search permission, group
       S_IROTH            04    read permission, others
       S_IWOTH            02    write permission, others
       S_IXOTH            01    execute/search permission, others
*/
struct sMystat {
//               dev_t     st_dev;     /* ID of device containing file */
//               ino_t     st_ino;     /* inode number */
    mode_t    st_mode;    /* protection */
//               nlink_t   st_nlink;   /* number of hard links */
//               uid_t     st_uid;     /* user ID of owner */
//               gid_t     st_gid;     /* group ID of owner */
//               dev_t     st_rdev;    /* device ID (if special file) */
    //        off_t     st_size;    /* total size, in bytes */
    DWORD     st_size;    /* total size, in bytes */
//               blksize_t st_blksize; /* blocksize for file system I/O */
//               blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
//               time_t    st_atime;   /* time of last access */
    time_t    st_mtime;   /* time of last modification */
//               time_t    st_ctime;   /* time of last status change */
};

sMystat myStatBuf;  //store info for file
FILINFO finfo;      //global file info struct see ff.h
FATFS_DIR dinfo;    //global directoty info struct see ff.h

FRESULT get_fileInfo(const char *path)  //use finfo for get result fields
{
    FRESULT res = f_stat(path,&finfo);    /* Get file status */
    if (EnDebugMSG)
        if (res)
            printf("\r\n-->get_fileInfo:%s ,res=%d ,Not Found!",path,res);
        else
            printf("\r\n-->get_fileInfo:%s ,res=%d ,Found!",path,res);
    return res;
}

FRESULT get_dirInfo(const char *path)
{
    FRESULT res= f_opendir (&dinfo,path);   /* FR_OK(0): successful, !=0: error code */
    if (EnDebugMSG)
        if (res)
            printf("\r\n-->get_dirInfo :%s res=%d ,This is Not Directory!",path,res);
        else
            printf("\r\n-->get_dirInfo :%s res=%d ,This is Directory!",path,res);
    return res;
}

FRESULT Mystat(const char *path, struct sMystat *buf)
{
    FRESULT res = f_stat(path,&finfo);    /* Get file status */
    if (res == FR_OK) {
        buf->st_size = finfo.fsize;
        buf->st_mtime = finfo.ftime;    //fdate;
        buf->st_mode = 4;   //?
    }
    if (EnDebugMSG)
        printf("\r\n--Mystat Path:%s ,filesize:%14lld ,res=%d",path, buf->st_size, res);
    return res;
}


static char* get_mime_type( char* filename )
{
    char* extension;
    extension = strrchr( filename, '.' );   //get string after last .
    if (EnDebugMSG)
        printf("\r\n-->get_mime_tipe filename:%s, extension:%s",filename, extension);
    if (extension !=NULL) {
        if (strcasecmp(extension,".htm")==0  || strcasecmp(extension,".html") ==0)
            return "text/html; charset=iso-8859-1";
        if (strcasecmp(extension,".png")==0)
            return "image/png";
        if (strcasecmp(extension,".css")==0)
            return "text/css";
        if (strcasecmp(extension,".gif")==0)
            return "image/gif";
        if (strcasecmp(extension,".jpg")==0 || strcasecmp(extension,".jpeg" )==0)
            return "image/jpeg";
    }
    return "text/plain; charset=iso-8859-1";
}

#endif
