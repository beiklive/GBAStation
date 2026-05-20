#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "Platform.h"
#include "platform/PlatformCommon.hpp"
#include "platform/PlatformFilesystem.hpp"

#include <borealis.hpp>

namespace Platform
{

// =========================================================
// 文件 I/O — 桥接到 PlatformFilesystem
// =========================================================
FILE* OpenFile(const char* path, const char* mode, bool mustexist)
{
    if (!path || !mode) return nullptr;
    if (mustexist && !beiklive::melonds::platform::fileExists(path))
        return nullptr;
    return fopen(path, mode);
}

FILE* OpenLocalFile(const char* path, const char* mode)
{
    if (!path || !mode) return nullptr;
    return beiklive::melonds::platform::openLocalFile(path, mode);
}

FILE* OpenDataFile(const char* path)
{
    return OpenLocalFile(path, "rb");
}

// =========================================================
// 线程
// =========================================================
struct Thread
{
    std::thread thread;
};

Thread* Thread_Create(std::function<void()> func)
{
    Thread* t = new Thread();
    t->thread = std::thread(func);
    return t;
}

void Thread_Free(Thread* thread)
{
    if (!thread) return;
    if (thread->thread.joinable())
        thread->thread.join();
    delete thread;
}

void Thread_Wait(Thread* thread)
{
    if (!thread) return;
    if (thread->thread.joinable())
        thread->thread.join();
}

// =========================================================
// 信号量
// =========================================================
struct Semaphore
{
    std::mutex mutex;
    std::condition_variable cv;
    int count = 0;
};

Semaphore* Semaphore_Create()
{
    return new Semaphore();
}

void Semaphore_Free(Semaphore* sema)
{
    delete sema;
}

void Semaphore_Reset(Semaphore* sema)
{
    if (!sema) return;
    std::lock_guard<std::mutex> lock(sema->mutex);
    sema->count = 0;
}

void Semaphore_Wait(Semaphore* sema)
{
    if (!sema) return;
    std::unique_lock<std::mutex> lock(sema->mutex);
    sema->cv.wait(lock, [sema] { return sema->count > 0; });
    sema->count--;
}

void Semaphore_Post(Semaphore* sema, int count)
{
    if (!sema) return;
    {
        std::lock_guard<std::mutex> lock(sema->mutex);
        sema->count += count;
    }
    if (count > 0)
        sema->cv.notify_all();
}

// =========================================================
// 互斥锁
// =========================================================
struct Mutex
{
    std::mutex mutex;
};

Mutex* Mutex_Create()
{
    return new Mutex();
}

void Mutex_Free(Mutex* mutex)
{
    delete mutex;
}

void Mutex_Lock(Mutex* mutex)
{
    if (!mutex) return;
    mutex->mutex.lock();
}

void Mutex_Unlock(Mutex* mutex)
{
    if (!mutex) return;
    mutex->mutex.unlock();
}

bool Mutex_TryLock(Mutex* mutex)
{
    if (!mutex) return false;
    return mutex->mutex.try_lock();
}

// =========================================================
// 睡眠
// =========================================================
void Sleep(u64 usecs)
{
    std::this_thread::sleep_for(std::chrono::microseconds(usecs));
}

// =========================================================
// 初始化 / 停止 / 生命周期
// =========================================================
void Init(int argc, char** argv)
{
    (void)argc; (void)argv;
}

void DeInit()
{
}

void StopEmu()
{
    beiklive::melonds::platform::signalStop(0);
}

// =========================================================
// 联机 (stub)
// =========================================================
bool MP_Init() { return false; }
void MP_DeInit() {}
int MP_SendPacket(u8*, int) { return 0; }
int MP_RecvPacket(u8*, bool) { return 0; }

bool LAN_Init() { return false; }
void LAN_DeInit() {}
int LAN_SendPacket(u8*, int) { return 0; }
int LAN_RecvPacket(u8*) { return 0; }

} // namespace Platform
