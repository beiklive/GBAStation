#pragma once

#include "Pico8Audio.hpp"
#include "Pico8Input.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace beiklive::pico8
{
    class Core
    {
    public:
        Core();
        ~Core();

        bool Initialize();
        void Shutdown();
        bool LoadGame(const std::string& path);
        void UnloadGame();
        bool RunFrame(float deltaSeconds);
        const uint8_t* GetFrameBuffer() const;
        void SetInput(const InputState& state);
        bool Reset();
        void Pause();
        void Resume();
        void SetAudioVolumes(float sfxVolume, float musicVolume);
        bool SaveState(std::vector<uint8_t>& output);
        bool LoadState(const uint8_t* data, size_t size);
        size_t GetStateSize() const;

        bool isInitialized() const;
        bool isGameLoaded() const;
        int targetFps() const;
        const std::string& lastError() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
