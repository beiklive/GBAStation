#pragma once
// 转发器 NPDM 可配置项（移植自 Sphaira owo.hpp，GPL-3.0-or-later）。
//
// 目前只提供能力与默认值，不暴露 UI 入口：
//   - address_space 默认 Bit36（与现有行为一致），Bit39 预留给后续新核心
//   - core_mode 默认 Three（3 核），后续新核心接入时再约定

#include <cstdint>

namespace sphaira
{
    /// NPDM AddressSpaceType，直接写入 meta.flags。
    ///
    /// 32 位空间从 0x00200000 起（而不是 0x08000000），总 VA 上限 4 GiB，
    /// 用于需要低位固定映像（例如 0x00400000）的程序。
    enum class ForwarderAddressSpace : std::uint8_t
    {
        Bit32 = 0,
        Bit36 = 1,
        Bit32NoAlias = 2,
        Bit39 = 3,
    };

    /// NPDM 内核能力位里的 CPU 核心数。
    enum class ForwarderCoreMode : std::uint8_t
    {
        Three = 3,
        Four = 4,
    };
}
