#ifdef PLATFORM_SWITCH
#include "sqlite3.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

// =========================
// 内存分配（Memory）
// =========================
static void *devkit_malloc(int size){ return malloc(size); }
static void devkit_free(void *ptr){ free(ptr); }
static void *devkit_realloc(void *ptr,int size){ return realloc(ptr,size); }
static int devkit_size(void *ptr){ return ptr ? malloc_usable_size(ptr) : 0; } // 可返回0
static int devkit_roundup(int size){ return size; }
static int devkit_init(void*){ return SQLITE_OK; }
static void devkit_shutdown(void*){}

// =========================
// 互斥（Mutex）
// =========================
static sqlite3_mutex *devkit_mutex_alloc(int){ return (sqlite3_mutex*)0; }
static void devkit_mutex_free(sqlite3_mutex*){}
static void devkit_mutex_enter(sqlite3_mutex*){}
static int devkit_mutex_try(sqlite3_mutex*){ return SQLITE_OK; }
static void devkit_mutex_leave(sqlite3_mutex*){}
static int devkit_mutex_held(sqlite3_mutex*){ return 1; }
static int devkit_mutex_notheld(sqlite3_mutex*){ return 1; }

// =========================
// 文件系统（VFS）
// =========================
struct DevkitFile {
    FILE *fp;
};

static int devkit_xClose(sqlite3_file *pFile){
    DevkitFile *f = (DevkitFile*)pFile->pMethods;
    if(f->fp) fclose(f->fp);
    delete f;
    return SQLITE_OK;
}

static int devkit_xRead(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite3_int64 iOfst){
    DevkitFile *f = (DevkitFile*)pFile->pMethods;
    if(fseek(f->fp, iOfst, SEEK_SET)) return SQLITE_IOERR_READ;
    size_t n = fread(zBuf,1,iAmt,f->fp);
    if(n<iAmt) memset((char*)zBuf+n,0,iAmt-n);
    return SQLITE_OK;
}

static int devkit_xWrite(sqlite3_file *pFile,const void *zBuf,int iAmt,sqlite3_int64 iOfst){
    DevkitFile *f = (DevkitFile*)pFile->pMethods;
    if(fseek(f->fp, iOfst, SEEK_SET)) return SQLITE_IOERR_WRITE;
    size_t n = fwrite(zBuf,1,iAmt,f->fp);
    if(n<iAmt) return SQLITE_IOERR_WRITE;
    return SQLITE_OK;
}

static int devkit_xTruncate(sqlite3_file*, sqlite3_int64){ return SQLITE_OK; }
static int devkit_xSync(sqlite3_file*){ return SQLITE_OK; }
static int devkit_xFileSize(sqlite3_file *pFile, sqlite3_int64 *pSize){
    DevkitFile *f = (DevkitFile*)pFile->pMethods;
    long cur = ftell(f->fp);
    fseek(f->fp,0,SEEK_END);
    *pSize = ftell(f->fp);
    fseek(f->fp,cur,SEEK_SET);
    return SQLITE_OK;
}
static int devkit_xLock(sqlite3_file*){ return SQLITE_OK; }
static int devkit_xUnlock(sqlite3_file*){ return SQLITE_OK; }
static int devkit_xCheckReservedLock(sqlite3_file*,int *pResOut){ *pResOut=0; return SQLITE_OK; }
static int devkit_xFileControl(sqlite3_file*, int, void*){ return SQLITE_OK; }
static int devkit_xSectorSize(sqlite3_file*){ return 4096; }
static int devkit_xDeviceCharacteristics(sqlite3_file*){ return 0; }

// sqlite3_io_methods
static sqlite3_io_methods devkit_io = {
    1,
    devkit_xClose,
    devkit_xRead,
    devkit_xWrite,
    devkit_xTruncate,
    devkit_xSync,
    devkit_xFileSize,
    devkit_xLock,
    devkit_xUnlock,
    devkit_xCheckReservedLock,
    devkit_xFileControl,
    devkit_xSectorSize,
    devkit_xDeviceCharacteristics
};

// sqlite3_file methods
static int devkit_xOpen(sqlite3_vfs*, const char *zName, sqlite3_file *pFile, int, int*){
    DevkitFile *f = new DevkitFile;
    f->fp = fopen(zName,"r+b");
    if(!f->fp) f->fp = fopen(zName,"w+b");
    pFile->pMethods = (sqlite3_io_methods*)f;
    return SQLITE_OK;
}

static int devkit_xDelete(sqlite3_vfs*, const char *zName, int){ return remove(zName)==0 ? SQLITE_OK : SQLITE_IOERR; }
static int devkit_xAccess(sqlite3_vfs*, const char *zName, int, int* pResOut){
    FILE *f = fopen(zName,"r");
    if(f){ fclose(f); *pResOut=1; } else { *pResOut=0; }
    return SQLITE_OK;
}
static int devkit_xFullPathname(sqlite3_vfs*, const char *zName, int nOut, char *zOut){
    strncpy(zOut,zName,nOut); zOut[nOut-1]=0; return SQLITE_OK;
}
static void *devkit_xDlOpen(sqlite3_vfs*, const char*){ return 0; }
static void devkit_xDlError(sqlite3_vfs*, int, char*){}
static void (*devkit_xDlSym(sqlite3_vfs*, void*, const char*)){ return 0; }
static void devkit_xDlClose(sqlite3_vfs*, void*){}
static int devkit_xRandomness(sqlite3_vfs*, int nByte, char *zOut){ for(int i=0;i<nByte;i++) zOut[i]=rand()%256; return nByte; }
static int devkit_xSleep(sqlite3_vfs*, int microseconds){ return microseconds; }
static int devkit_xCurrentTime(sqlite3_vfs*, double *pTime){ *pTime=2440587.5; return SQLITE_OK; }

// sqlite3_vfs
static sqlite3_vfs devkit_vfs = {
    1,
    sizeof(DevkitFile),
    4096,
    0,
    "devkitVFS",
    0,
    devkit_xOpen,
    devkit_xDelete,
    devkit_xAccess,
    devkit_xFullPathname,
    devkit_xDlOpen,
    devkit_xDlError,
    devkit_xDlSym,
    devkit_xDlClose,
    devkit_xRandomness,
    devkit_xSleep,
    devkit_xCurrentTime
};

// =========================
// sqlite3_os_init / sqlite3_os_end
// =========================
extern "C" int sqlite3_os_init(void){ return SQLITE_OK; }
extern "C" int sqlite3_os_end(void){ return SQLITE_OK; }

// =========================
// 初始化入口
// =========================
extern "C" int sqlite3_devkit_initialize(){
    sqlite3_config(SQLITE_CONFIG_MALLOC, devkit_malloc, devkit_free, devkit_realloc, devkit_size, devkit_roundup, devkit_init, devkit_shutdown);
    sqlite3_config(SQLITE_CONFIG_MUTEX, devkit_mutex_alloc, devkit_mutex_free,
                   devkit_mutex_enter, devkit_mutex_try, devkit_mutex_leave,
                   devkit_mutex_held, devkit_mutex_notheld);
    sqlite3_vfs_register(&devkit_vfs, 1);
    return sqlite3_initialize();
}

#endif