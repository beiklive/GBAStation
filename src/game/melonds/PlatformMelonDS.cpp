#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>

#include <borealis.hpp>

#include "Platform.h"
#include "types.h"
#include "SPI_Firmware.h"

namespace melonDS::Platform
{

// ---- 文件 I/O ----------------------------------------------------------

FileHandle* OpenFile(const std::string& path, FileMode mode)
{
    if ((mode & FileMode::ReadWrite) == FileMode::None)
        return nullptr;

    std::string modeStr;
    if (mode & FileMode::Write)
    {
        if (mode & FileMode::Read)
            modeStr = (mode & FileMode::Preserve) ? "r+b" : "w+b";
        else
            modeStr = (mode & FileMode::Preserve) ? "ab" : "wb";
    }
    else
    {
        modeStr = "rb";
    }

    if (mode & FileMode::Text)
        modeStr.back() = '\0';
    modeStr.push_back('\0');

    FILE* f = fopen(path.c_str(), modeStr.c_str());
    return reinterpret_cast<FileHandle*>(f);
}

FileHandle* OpenLocalFile(const std::string& path, FileMode mode)
{
    std::string configDir;
#if defined(__SWITCH__)
    configDir = "sdmc:/switch/GBAStation/";
#elif defined(_WIN32)
    configDir = ".";
#else
    const char* home = getenv("HOME");
    if (home)
        configDir = std::string(home) + "/.config/GBAStation/";
    else
        configDir = ".";
#endif

    std::string fullPath = configDir + path;
    FileHandle* f = OpenFile(fullPath, mode);
    if (!f && !configDir.empty())
        f = OpenFile(path, mode);
    return f;
}

bool CloseFile(FileHandle* file)
{
    if (!file) return false;
    return fclose(reinterpret_cast<FILE*>(file)) == 0;
}

bool IsEndOfFile(FileHandle* file)
{
    if (!file) return true;
    return feof(reinterpret_cast<FILE*>(file)) != 0;
}

bool FileReadLine(char* str, int count, FileHandle* file)
{
    if (!file) return false;
    return fgets(str, count, reinterpret_cast<FILE*>(file)) != nullptr;
}

bool FileExists(const std::string& name)
{
    FILE* f = fopen(name.c_str(), "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

bool LocalFileExists(const std::string& name)
{
    FileHandle* f = OpenLocalFile(name, FileMode::Read);
    if (!f) return false;
    CloseFile(f);
    return true;
}

bool FileSeek(FileHandle* file, s64 offset, FileSeekOrigin origin)
{
    if (!file) return false;
    int stdOrigin = SEEK_SET;
    switch (origin)
    {
    case FileSeekOrigin::Start:   stdOrigin = SEEK_SET; break;
    case FileSeekOrigin::Current: stdOrigin = SEEK_CUR; break;
    case FileSeekOrigin::End:     stdOrigin = SEEK_END; break;
    }
    return fseek(reinterpret_cast<FILE*>(file), offset, stdOrigin) == 0;
}

void FileRewind(FileHandle* file)
{
    if (!file) return;
    rewind(reinterpret_cast<FILE*>(file));
}

u64 FileRead(void* data, u64 size, u64 count, FileHandle* file)
{
    if (!file) return 0;
    return fread(data, size, count, reinterpret_cast<FILE*>(file));
}

bool FileFlush(FileHandle* file)
{
    if (!file) return false;
    return fflush(reinterpret_cast<FILE*>(file)) == 0;
}

u64 FileWrite(const void* data, u64 size, u64 count, FileHandle* file)
{
    if (!file) return 0;
    return fwrite(data, size, count, reinterpret_cast<FILE*>(file));
}

u64 FileWriteFormatted(FileHandle* file, const char* fmt, ...)
{
    if (!file || !fmt) return 0;
    va_list args;
    va_start(args, fmt);
    u64 ret = vfprintf(reinterpret_cast<FILE*>(file), fmt, args);
    va_end(args);
    return ret;
}

u64 FileLength(FileHandle* file)
{
    if (!file) return 0;
    FILE* f = reinterpret_cast<FILE*>(file);
    long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, pos, SEEK_SET);
    return len;
}

// ---- 日志 --------------------------------------------------------------

void Log(LogLevel level, const char* fmt, ...)
{
    if (!fmt) return;
    va_list args;
    va_start(args, fmt);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    switch (level)
    {
    case LogLevel::Debug: brls::Logger::debug("{}", buf); break;
    case LogLevel::Info:  brls::Logger::info("{}", buf);  break;
    case LogLevel::Warn:  brls::Logger::warning("{}", buf); break;
    case LogLevel::Error: brls::Logger::error("{}", buf); break;
    }
}

// ---- 线程 --------------------------------------------------------------

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

// ---- 信号量 ------------------------------------------------------------

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
    sema->cv.notify_all();
}

// ---- 互斥锁 ------------------------------------------------------------

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

void Sleep(u64 usecs)
{
    std::this_thread::sleep_for(std::chrono::microseconds(usecs));
}

// ---- 存档 / 固件写入 ---------------------------------------------------

// 由 CoreMelonDS 通过静态变量告知 Platform 如何保存数据
static struct {
    std::string ndsSavePath;
    std::string gbaSavePath;
    std::string firmwarePath;
    std::function<void()> stopCallback;
} g_ctx;

void SetNDSSavePath(const std::string& path) { g_ctx.ndsSavePath = path; }
void SetGBASavePath(const std::string& path) { g_ctx.gbaSavePath = path; }
void SetFirmwarePath(const std::string& path) { g_ctx.firmwarePath = path; }
void SetStopCallback(std::function<void()> cb) { g_ctx.stopCallback = std::move(cb); }

void WriteNDSSave(const u8* savedata, u32 savelen, u32 writeoffset, u32 writelen)
{
    if (g_ctx.ndsSavePath.empty() || !savedata) return;
    FILE* f = fopen(g_ctx.ndsSavePath.c_str(), "r+b");
    if (!f)
        f = fopen(g_ctx.ndsSavePath.c_str(), "wb");
    if (!f) return;
    fwrite(savedata, savelen, 1, f);
    fclose(f);
}

void WriteGBASave(const u8* savedata, u32 savelen, u32 writeoffset, u32 writelen)
{
    if (g_ctx.gbaSavePath.empty() || !savedata) return;
    FILE* f = fopen(g_ctx.gbaSavePath.c_str(), "r+b");
    if (!f)
        f = fopen(g_ctx.gbaSavePath.c_str(), "wb");
    if (!f) return;
    fwrite(savedata, savelen, 1, f);
    fclose(f);
}

void WriteFirmware(const Firmware& firmware, u32 writeoffset, u32 writelen)
{
    if (g_ctx.firmwarePath.empty()) return;
    FILE* f = fopen(g_ctx.firmwarePath.c_str(), "wb");
    if (!f) return;
    fwrite(firmware.Buffer(), firmware.Length(), 1, f);
    fclose(f);
}

void WriteDateTime(int year, int month, int day, int hour, int minute, int second)
{
    (void)year; (void)month; (void)day;
    (void)hour; (void)minute; (void)second;
}

// ---- 初始化 / 停止 -----------------------------------------------------

void Init(int argc, char** argv) {}
void DeInit() {}

void SignalStop(StopReason reason)
{
    if (g_ctx.stopCallback)
        g_ctx.stopCallback();
}

int InstanceID()
{
    return 0;
}

std::string InstanceFileSuffix()
{
    return "";
}

// ---- 本地联机 (stub) ---------------------------------------------------

bool MP_Init() { return false; }
void MP_DeInit() {}
void MP_Begin() {}
void MP_End() {}
int MP_SendPacket(u8*, int, u64) { return 0; }
int MP_RecvPacket(u8*, u64*) { return 0; }
int MP_SendCmd(u8*, int, u64) { return 0; }
int MP_SendReply(u8*, int, u64, u16) { return 0; }
int MP_SendAck(u8*, int, u64) { return 0; }
int MP_RecvHostPacket(u8*, u64*) { return 0; }
u16 MP_RecvReplies(u8*, u64, u16) { return 0; }

// ---- LAN 联机 (stub) ---------------------------------------------------

bool LAN_Init() { return false; }
void LAN_DeInit() {}
int LAN_SendPacket(u8*, int) { return 0; }
int LAN_RecvPacket(u8*) { return 0; }

// ---- 摄像头 (stub) -----------------------------------------------------

void Camera_Start(int) {}
void Camera_Stop(int) {}
void Camera_CaptureFrame(int, u32*, int, int, bool) {}

// ---- 动态库 (stub) -----------------------------------------------------

DynamicLibrary* DynamicLibrary_Load(const char*) { return nullptr; }
void DynamicLibrary_Unload(DynamicLibrary*) {}
void* DynamicLibrary_LoadFunction(DynamicLibrary*, const char*) { return nullptr; }

} // namespace melonDS::Platform
