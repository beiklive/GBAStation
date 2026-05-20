#pragma once

#include <cstdint>
#include <string>

namespace beiklive::core
{

class IEmulatorCore
{
public:
    virtual ~IEmulatorCore() = default;

    virtual bool LoadRom(const std::string& path) = 0;

    virtual void Reset() = 0;

    virtual void RunFrame() = 0;

    virtual void Stop() = 0;

    virtual bool IsRunning() const = 0;

    virtual const uint32_t* GetTopScreen() = 0;

    virtual const uint32_t* GetBottomScreen() = 0;

    virtual int GetScreenWidth() const = 0;

    virtual int GetScreenHeight() const = 0;

    virtual int GetTopScreenHeight() const = 0;

    virtual int GetBottomScreenHeight() const = 0;

    virtual void PushInput(int key, bool pressed) = 0;

    virtual void SaveState(const std::string& path) = 0;

    virtual void LoadState(const std::string& path) = 0;
};

} // namespace beiklive::core
