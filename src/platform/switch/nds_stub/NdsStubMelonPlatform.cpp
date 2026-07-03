#include "platform/switch/nds_stub/NdsStubMelonPlatform.hpp"

#include "Platform.h"
#include "SPI_Firmware.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <switch.h>

namespace melonDS {
void HandleFault(u64 pc, u64 lr, u64 fp, u64 faultAddr, u32 desc)
{
    beiklive::nds_stub::appendStubLog(
        "melonDS JIT fault: pc=0x%llx lr=0x%llx fp=0x%llx addr=0x%llx desc=0x%x",
        static_cast<unsigned long long>(pc),
        static_cast<unsigned long long>(lr),
        static_cast<unsigned long long>(fp),
        static_cast<unsigned long long>(faultAddr),
        desc);
}
} // namespace melonDS

namespace {

const char* modeString(melonDS::Platform::FileMode mode)
{
    using melonDS::Platform::FileMode;
    const bool read = (mode & FileMode::Read) != 0;
    const bool write = (mode & FileMode::Write) != 0;
    const bool append = (mode & FileMode::Append) != 0;
    const bool preserve = (mode & FileMode::Preserve) != 0;
    const bool text = (mode & FileMode::Text) != 0;

    if (append && read)
        return text ? "a+" : "a+b";
    if (append)
        return text ? "a" : "ab";
    if (read && write && preserve)
        return text ? "r+" : "r+b";
    if (read && write)
        return text ? "w+" : "w+b";
    if (write)
        return text ? "w" : "wb";
    return text ? "r" : "rb";
}

bool writeWholeFile(const std::string& path, const void* data, size_t size)
{
    if (!data && size != 0)
        return false;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    FILE* fp = std::fopen(path.c_str(), "wb");
    if (!fp)
        return false;
    const bool ok = size == 0 || std::fwrite(data, 1, size, fp) == size;
    std::fclose(fp);
    return ok;
}

} // namespace

namespace melonDS::Platform {

struct FileHandle {
    FILE* fp = nullptr;
};

std::string GetLocalFilePath(const std::string& filename)
{
    return filename;
}

FileHandle* OpenFile(const std::string& path, FileMode mode)
{
    if ((mode & FileMode::NoCreate) && !FileExists(path))
        return nullptr;
    FILE* fp = std::fopen(path.c_str(), modeString(mode));
    if (!fp)
        return nullptr;
    return new FileHandle{fp};
}

FileHandle* OpenLocalFile(const std::string& path, FileMode mode)
{
    return OpenFile(path, mode);
}

bool FileExists(const std::string& name)
{
    std::error_code ec;
    return std::filesystem::exists(name, ec);
}

bool LocalFileExists(const std::string& name)
{
    return FileExists(name);
}

bool CheckFileWritable(const std::string& filepath)
{
    FileHandle* file = OpenFile(filepath, static_cast<FileMode>(FileMode::Write | FileMode::Preserve));
    if (!file)
        return false;
    return CloseFile(file);
}

bool CheckLocalFileWritable(const std::string& filepath)
{
    return CheckFileWritable(filepath);
}

bool CloseFile(FileHandle* file)
{
    if (!file)
        return false;
    const bool ok = !file->fp || std::fclose(file->fp) == 0;
    delete file;
    return ok;
}

bool IsEndOfFile(FileHandle* file)
{
    return !file || !file->fp || std::feof(file->fp) != 0;
}

bool FileReadLine(char* str, int count, FileHandle* file)
{
    return file && file->fp && std::fgets(str, count, file->fp) != nullptr;
}

u64 FilePosition(FileHandle* file)
{
    if (!file || !file->fp)
        return 0;
    const long pos = std::ftell(file->fp);
    return pos < 0 ? 0 : static_cast<u64>(pos);
}

bool FileSeek(FileHandle* file, s64 offset, FileSeekOrigin origin)
{
    if (!file || !file->fp)
        return false;
    int whence = SEEK_SET;
    if (origin == FileSeekOrigin::Current) whence = SEEK_CUR;
    if (origin == FileSeekOrigin::End) whence = SEEK_END;
    return std::fseek(file->fp, static_cast<long>(offset), whence) == 0;
}

void FileRewind(FileHandle* file)
{
    if (file && file->fp)
        std::rewind(file->fp);
}

u64 FileRead(void* data, u64 size, u64 count, FileHandle* file)
{
    if (!file || !file->fp)
        return 0;
    return static_cast<u64>(std::fread(data, static_cast<size_t>(size), static_cast<size_t>(count), file->fp));
}

bool FileFlush(FileHandle* file)
{
    return file && file->fp && std::fflush(file->fp) == 0;
}

u64 FileWrite(const void* data, u64 size, u64 count, FileHandle* file)
{
    if (!file || !file->fp)
        return 0;
    return static_cast<u64>(std::fwrite(data, static_cast<size_t>(size), static_cast<size_t>(count), file->fp));
}

u64 FileWriteFormatted(FileHandle* file, const char* fmt, ...)
{
    if (!file || !file->fp || !fmt)
        return 0;
    va_list args;
    va_start(args, fmt);
    const int written = std::vfprintf(file->fp, fmt, args);
    va_end(args);
    return written < 0 ? 0 : static_cast<u64>(written);
}

u64 FileLength(FileHandle* file)
{
    if (!file || !file->fp)
        return 0;
    const long pos = std::ftell(file->fp);
    if (std::fseek(file->fp, 0, SEEK_END) != 0)
        return 0;
    const long len = std::ftell(file->fp);
    std::fseek(file->fp, pos, SEEK_SET);
    return len < 0 ? 0 : static_cast<u64>(len);
}

void Log(LogLevel level, const char* fmt, ...)
{
    char buffer[2048] = {};
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    beiklive::nds_stub::appendStubLog("melonDS[%d]: %s", static_cast<int>(level), buffer);
}

struct Thread {
    std::thread thread;
};

Thread* Thread_Create(std::function<void()> func)
{
    try
    {
        return new Thread{std::thread([func = std::move(func)]() mutable {
            svcSetThreadCoreMask(CUR_THREAD_HANDLE, 3, 1ULL << 3);
            func();
        })};
    }
    catch (...)
    {
        return nullptr;
    }
}

void Thread_Free(Thread* thread)
{
    delete thread;
}

void Thread_Wait(Thread* thread)
{
    if (thread && thread->thread.joinable())
        thread->thread.join();
}

struct Semaphore {
    std::mutex mutex;
    std::condition_variable cv;
    int count = 0;
};

Semaphore* Semaphore_Create() { return new Semaphore(); }
void Semaphore_Free(Semaphore* sema) { delete sema; }

void Semaphore_Reset(Semaphore* sema)
{
    if (!sema)
        return;
    std::lock_guard<std::mutex> lock(sema->mutex);
    sema->count = 0;
}

void Semaphore_Wait(Semaphore* sema)
{
    if (!sema)
        return;
    std::unique_lock<std::mutex> lock(sema->mutex);
    sema->cv.wait(lock, [&] { return sema->count > 0; });
    --sema->count;
}

bool Semaphore_TryWait(Semaphore* sema, int timeout_ms)
{
    if (!sema)
        return false;
    std::unique_lock<std::mutex> lock(sema->mutex);
    if (timeout_ms == 0)
    {
        if (sema->count <= 0)
            return false;
    }
    else if (!sema->cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] { return sema->count > 0; }))
    {
        return false;
    }
    --sema->count;
    return true;
}

void Semaphore_Post(Semaphore* sema, int count)
{
    if (!sema || count <= 0)
        return;
    {
        std::lock_guard<std::mutex> lock(sema->mutex);
        sema->count += count;
    }
    sema->cv.notify_all();
}

struct Mutex {
    std::mutex mutex;
};

Mutex* Mutex_Create() { return new Mutex(); }
void Mutex_Free(Mutex* mutex) { delete mutex; }
void Mutex_Lock(Mutex* mutex) { if (mutex) mutex->mutex.lock(); }
void Mutex_Unlock(Mutex* mutex) { if (mutex) mutex->mutex.unlock(); }
bool Mutex_TryLock(Mutex* mutex) { return mutex && mutex->mutex.try_lock(); }

void Sleep(u64 usecs)
{
    svcSleepThread(static_cast<int64_t>(usecs * 1000ULL));
}

u64 GetMSCount()
{
    return armTicksToNs(armGetSystemTick()) / 1000000ULL;
}

u64 GetUSCount()
{
    return armTicksToNs(armGetSystemTick()) / 1000ULL;
}

void SignalStop(StopReason reason, void*)
{
    Log(reason == StopReason::External ? LogLevel::Info : LogLevel::Warn,
        "emulation stopped, reason=%d", static_cast<int>(reason));
}

void WriteNDSSave(const u8* savedata, u32 savelen, u32, u32, void* userdata)
{
    auto* data = static_cast<beiklive::nds_stub::MelonPlatformData*>(userdata);
    if (data && !data->savePath.empty() && savedata)
        writeWholeFile(data->savePath, savedata, savelen);
}

void WriteGBASave(const u8*, u32, u32, u32, void*) {}

void WriteFirmware(const Firmware& firmware, u32, u32, void* userdata)
{
    auto* data = static_cast<beiklive::nds_stub::MelonPlatformData*>(userdata);
    if (data && !data->firmwarePath.empty() && firmware.Length() > 0)
        writeWholeFile(data->firmwarePath, firmware.Buffer(), firmware.Length());
}

void WriteDateTime(int, int, int, int, int, int, void*) {}
void MP_Begin(void*) {}
void MP_End(void*) {}
int MP_SendPacket(u8*, int, u64, void*) { return 0; }
int MP_RecvPacket(u8*, u64*, void*) { return 0; }
int MP_SendCmd(u8*, int, u64, void*) { return 0; }
int MP_SendReply(u8*, int, u64, u16, void*) { return 0; }
int MP_SendAck(u8*, int, u64, void*) { return 0; }
int MP_RecvHostPacket(u8* data, u64* timestamp, void* userdata)
{
    (void)data;
    (void)timestamp;
    (void)userdata;
    return 0;
}
u16 MP_RecvReplies(u8*, u64, u16, void*) { return 0; }
int Net_SendPacket(u8*, int, void*) { return 0; }
int Net_RecvPacket(u8*, void*) { return 0; }
void Camera_Start(int, void*) {}
void Camera_Stop(int, void*) {}
void Camera_CaptureFrame(int, u32* frame, int width, int height, bool, void*)
{
    if (frame)
        std::fill(frame, frame + width * height, 0);
}
void Mic_Start(void*) {}
void Mic_Stop(void*) {}
int Mic_ReadInput(s16* data, int maxlength, void*)
{
    if (data && maxlength > 0)
        std::fill(data, data + maxlength, 0);
    return maxlength;
}
AACDecoder* AAC_Init() { return nullptr; }
void AAC_DeInit(AACDecoder*) {}
bool AAC_Configure(AACDecoder*, int, int) { return false; }
bool AAC_DecodeFrame(AACDecoder*, const void*, int, void*, int) { return false; }
bool Addon_KeyDown(KeyType, void*) { return false; }
void Addon_RumbleStart(u32, void*) {}
void Addon_RumbleStop(void*) {}
float Addon_MotionQuery(MotionQueryType, void*) { return 0.0f; }
DynamicLibrary* DynamicLibrary_Load(const char*) { return nullptr; }
void DynamicLibrary_Unload(DynamicLibrary*) {}
void* DynamicLibrary_LoadFunction(DynamicLibrary*, const char*) { return nullptr; }

} // namespace melonDS::Platform
