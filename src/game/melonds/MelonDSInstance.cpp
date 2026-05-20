#include "MelonDSInstance.hpp"

#include "NDS.h"
#include "NDSCart.h"
#include "GPU.h"
#include "SPU.h"
#include "RTC.h"
#include "SPI.h"
#include "Savestate.h"

#include <borealis.hpp>
#include <cstring>

namespace beiklive::melonds
{

MelonDSInstance::MelonDSInstance()
{
}

MelonDSInstance::~MelonDSInstance()
{
    Shutdown();
}

bool MelonDSInstance::Init()
{
    if (m_initialized)
        return true;

    if (!NDS::Init())
    {
        brls::Logger::error("MelonDSInstance: NDS::Init() failed");
        return false;
    }

    m_initialized = true;
    brls::Logger::info("MelonDSInstance: subsystem initialized");
    return true;
}

void MelonDSInstance::Shutdown()
{
    if (!m_initialized)
        return;

    if (m_running)
        Stop();

    NDS::DeInit();
    m_initialized = false;
}

bool MelonDSInstance::LoadROM(const uint8_t* romdata, uint32_t romlen,
                               const char* sramPath)
{
    if (!m_initialized)
    {
        brls::Logger::error("MelonDSInstance: not initialized");
        return false;
    }

    brls::Logger::info("MelonDSInstance: calling NDS::LoadROM ({} bytes, direct=true)", romlen);

    if (!NDS::LoadROM(romdata, romlen, sramPath, true))
    {
        brls::Logger::error("MelonDSInstance: NDS::LoadROM() failed");
        return false;
    }

    brls::Logger::info("MelonDSInstance: ROM loaded successfully ({} bytes)", romlen);
    return true;
}

void MelonDSInstance::DirectBoot()
{
    brls::Logger::info("MelonDSInstance: DirectBoot called");
    NDS::SetupDirectBoot();
    brls::Logger::info("MelonDSInstance: DirectBoot done");
}

void MelonDSInstance::Start()
{
    if (!m_initialized || m_running)
        return;

    m_running = true;
    brls::Logger::info("MelonDSInstance: started");
}

void MelonDSInstance::Stop()
{
    m_running = false;
}

void MelonDSInstance::Reset()
{
    if (!m_initialized)
        return;

    NDS::Reset();
}

uint32_t MelonDSInstance::RunFrame()
{
    if (!m_initialized || !m_running)
        return 0;

    return NDS::RunFrame();
}

const uint32_t* MelonDSInstance::GetFramebuffer(int screen) const
{
    if (!m_initialized)
        return nullptr;

    int fb = GPU::FrontBuffer;
    return GPU::Framebuffer[screen][fb];
}

int MelonDSInstance::GetFrontBufferIndex() const
{
    return GPU::FrontBuffer;
}

int MelonDSInstance::ReadAudio(int16_t* data, int samples)
{
    if (!m_initialized)
        return 0;

    return SPU::ReadOutput(data, samples);
}

void MelonDSInstance::SetKeyMask(uint32_t mask)
{
    if (!m_initialized)
        return;

    NDS::SetKeyMask(mask);
}

void MelonDSInstance::TouchScreen(uint16_t x, uint16_t y)
{
    if (!m_initialized)
        return;

    NDS::TouchScreen(x, y);
}

void MelonDSInstance::ReleaseScreen()
{
    if (!m_initialized)
        return;

    NDS::ReleaseScreen();
}

void MelonDSInstance::SetRTC(int year, int month, int day, int hour, int minute, int second)
{
    (void)year; (void)month; (void)day; (void)hour; (void)minute; (void)second;
}

bool MelonDSInstance::DoSavestate(const std::string& path, bool save)
{
    if (!m_initialized)
        return false;

    Savestate ss(path.c_str(), save);
    if (ss.Error)
        return false;

    return NDS::DoSavestate(&ss);
}

void MelonDSInstance::FlushSave()
{
    if (!m_initialized)
        return;

    NDSCart::FlushSRAMFile();
}

void MelonDSInstance::SetStopCallback(std::function<void()> cb)
{
    m_stopCallback = std::move(cb);
}

} // namespace beiklive::melonds
