#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../../Platform.h"

#include <switch.h>

namespace Platform
{

void StopEmu()
{
}

FILE* OpenFile(const char* path, const char* mode, bool mustexist)
{
    FILE* ret;

    if (mustexist)
    {
        ret = fopen(path, "rb");
        if (ret) ret = freopen(path, mode, ret);
    }
    else
        ret = fopen(path, mode);

    return ret;
}


FILE* OpenLocalFile(const char* path, const char* mode)
{
    char finalPath[256];
    sprintf(finalPath, "/switch/melonds/%s", path);

    FILE* ret = fopen(finalPath, mode);
    if (ret)
        return ret;

    sprintf(finalPath, "/melonds/%s", path);

    ret = fopen(finalPath, mode);
    if (ret)
        return ret;

    return fopen(path, mode);
}

FILE* OpenDataFile(const char* path)
{
	return OpenLocalFile(path, "rb");
}

void Sleep(u64 usecs)
{
    svcSleepThread(usecs * 1000);
}

struct ThreadEntryData
{
    std::function<void()> EntryPoint;
};

void ThreadEntry(void* param)
{
    ThreadEntryData* entryData = (ThreadEntryData*)param;
    entryData->EntryPoint();
    delete entryData;
}

#define STACK_SIZE (1024 * 1024 * 4)

int nextCore = 1;

Thread* Thread_Create(std::function<void()> func)
{
    ::Thread* thread = new ::Thread();
    // this relies on the order of thread creation, very bad
    threadCreate(thread, ThreadEntry, new ThreadEntryData{func}, NULL, STACK_SIZE, 0x30, nextCore++);
    threadStart(thread);
    return (Thread*)thread;
}

void Thread_Free(Thread* thread)
{
    threadClose((::Thread*)thread);
    delete ((::Thread*)thread);
}

void Thread_Wait(Thread* thread)
{
    threadWaitForExit((::Thread*)thread);
}

struct MySemaphore
{
    ::CondVar condvar;
    ::Mutex mutex;
    u64 count;
};

Semaphore* Semaphore_Create()
{
    MySemaphore* sema = new MySemaphore();
    sema->count = 0;
    mutexInit(&sema->mutex);
    condvarInit(&sema->mutex);
    return (Semaphore*)sema;
}

void Semaphore_Free(Semaphore* sema)
{
    delete (::Semaphore*)sema;
}

void Semaphore_Reset(Semaphore* sema)
{
    MySemaphore* mysema = (MySemaphore*)sema;
    mutexLock(&mysema->mutex);
    mysema->count = 0;
    mutexUnlock(&mysema->mutex);
}

void Semaphore_Wait(Semaphore* sema)
{
    MySemaphore* mysema = (MySemaphore*)sema;
    mutexLock(&mysema->mutex);
    while (mysema->count == 0)
        condvarWait(&mysema->condvar, &mysema->mutex);
    mysema->count--;
    mutexUnlock(&mysema->mutex);
}

void Semaphore_Post(Semaphore* sema, int num)
{
    if (num <= 0) return;

    MySemaphore* mysema = (MySemaphore*)sema;
    mutexLock(&mysema->mutex);
    mysema->count += num;
    mutexUnlock(&mysema->mutex);
    condvarWake(&mysema->condvar, num);
}

Mutex* Mutex_Create()
{
    ::Mutex* mutex = new ::Mutex();
    mutexInit(mutex);
    return (Mutex*)mutex;
}

void Mutex_Free(Mutex* mutex)
{
    delete (::Mutex*)mutex;
}

void Mutex_Lock(Mutex* mutex)
{
    mutexLock((::Mutex*)mutex);
}

void Mutex_Unlock(Mutex* mutex)
{
    mutexUnlock((::Mutex*)mutex);
}

bool Mutex_TryLock(Mutex* mutex)
{
    return mutexTryLock((::Mutex*)mutex);
}

void* GL_GetProcAddress(const char* proc)
{
    return NULL;
}

bool MP_Init()
{
    return false;
}

void MP_DeInit()
{}

int MP_SendPacket(u8* data, int len)
{
    return 0;
}

int MP_RecvPacket(u8* data, bool block)
{
    return 0;
}

bool LAN_Init()
{
    return false;
}

void LAN_DeInit()
{}

int LAN_SendPacket(u8* data, int len)
{
    return 0;
}

int LAN_RecvPacket(u8* data)
{
    return 0;
}

}