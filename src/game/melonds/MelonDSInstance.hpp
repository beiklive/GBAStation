#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <memory>

namespace melonDS
{
class NDS;
}

namespace beiklive::melonds
{

class MelonDSInstance
{
public:
    MelonDSInstance();
    ~MelonDSInstance();

    MelonDSInstance(const MelonDSInstance&) = delete;
    MelonDSInstance& operator=(const MelonDSInstance&) = delete;

    bool Init();
    void Shutdown();

    bool LoadROM(const uint8_t* romdata, uint32_t romlen,
                 const char* sramPath);

    void DirectBoot();

    void Start();
    void Stop();
    void Reset();

    uint32_t RunFrame();

    const uint32_t* GetFramebuffer(int screen) const;
    int GetFrontBufferIndex() const;

    int ReadAudio(int16_t* data, int samples);

    void SetKeyMask(uint32_t mask);
    void TouchScreen(uint16_t x, uint16_t y);
    void ReleaseScreen();

    void SetRTC(int year, int month, int day, int hour, int minute, int second);

    bool DoSavestate(const std::string& path, bool save);

    void FlushSave();

    bool IsInitialized() const { return m_initialized; }
    bool IsRunning() const { return m_running; }

    void SetStopCallback(std::function<void()> cb);

private:
    bool m_initialized = false;
    bool m_running = false;
    std::function<void()> m_stopCallback;
};

} // namespace beiklive::melonds
