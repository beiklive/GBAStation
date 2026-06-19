/*
    Copyright 2024 Jesse Talavera

    melonDS DS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS DS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS DS. If not, see http://www.gnu.org/licenses/.
*/

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <NDS.h>

#ifdef HAVE_NETWORKING
#ifdef HAVE_NETWORKING_DIRECT_MODE
#include <Net_PCap.h>
#endif

#include "Net.h"
#endif

#include "config/types.hpp"
#include "std/span.hpp"

namespace melonDS
{
    struct AdapterData;
    class NetDriver;
}

namespace MelonDsDs
{
    class CoreConfig;

    class NetState
    {
    public:
        NetState();
        ~NetState() noexcept;
        NetState(const NetState&) = delete;
        NetState(NetState&&) = delete;
        NetState& operator=(const NetState&) = delete;
        NetState& operator=(NetState&&) = delete;

        int SendPacket(std::span<std::byte> data) noexcept;
        int RecvPacket(melonDS::u8* data) noexcept;
#ifdef HAVE_NETWORKING
        [[nodiscard]] std::vector<melonDS::AdapterData> GetAdapters() const noexcept;
#endif
        void Apply(const CoreConfig& config) noexcept;
        [[nodiscard]] NetworkMode GetNetworkMode() const noexcept;
    private:
#ifdef HAVE_NETWORKING
        melonDS::Net _net;
#ifdef HAVE_NETWORKING_DIRECT_MODE
        std::optional<melonDS::LibPCap> _pcap;
        std::optional<melonDS::AdapterData> _adapter;
#endif
#endif
    };
}
