// SPDX-FileCopyrightText: Copyright (c) 2022 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>

#if defined(__SWITCH__)
#define u128 libnx_u128
#include <switch.h>
#undef u128
#ifdef BIT
#undef BIT
#endif
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#include <libkern/OSCacheControl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#else
#include <sys/mman.h>
#endif

namespace oaknut {

class CodeBlock {
public:
    explicit CodeBlock(std::size_t size) : m_size(size) {
#if defined(__SWITCH__)
        if (R_FAILED(jitCreate(&m_jit, size))) {
            throw std::bad_alloc{};
        }
        m_memory = static_cast<std::uint32_t*>(jitGetRwAddr(&m_jit));
        m_executable = static_cast<std::uint32_t*>(jitGetRxAddr(&m_jit));
#elif defined(_WIN32)
        m_memory = static_cast<std::uint32_t*>(
            VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
        m_executable = m_memory;
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
        m_memory = static_cast<std::uint32_t*>(
            mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0));
#else
        m_memory = static_cast<std::uint32_t*>(mmap(
            nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_ANON | MAP_PRIVATE | MAP_JIT, -1, 0));
#endif
        m_executable = m_memory;
#elif defined(__NetBSD__)
        m_memory = static_cast<std::uint32_t*>(mmap(
            nullptr, size, PROT_MPROTECT(PROT_READ | PROT_WRITE | PROT_EXEC),
            MAP_ANON | MAP_PRIVATE, -1, 0));
        m_executable = m_memory;
#elif defined(__OpenBSD__)
        m_memory = static_cast<std::uint32_t*>(
            mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0));
        m_executable = m_memory;
#else
        m_memory = static_cast<std::uint32_t*>(mmap(
            nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_ANON | MAP_PRIVATE, -1, 0));
        m_executable = m_memory;
#endif
        if (m_memory == nullptr) {
            throw std::bad_alloc{};
        }
    }

    ~CodeBlock() {
        if (m_memory == nullptr) {
            return;
        }
#if defined(__SWITCH__)
        jitClose(&m_jit);
#elif defined(_WIN32)
        VirtualFree(static_cast<void*>(m_memory), 0, MEM_RELEASE);
#else
        munmap(m_memory, m_size);
#endif
    }

    CodeBlock(const CodeBlock&) = delete;
    CodeBlock& operator=(const CodeBlock&) = delete;
    CodeBlock(CodeBlock&&) = delete;
    CodeBlock& operator=(CodeBlock&&) = delete;

    std::uint32_t* ptr() const {
        return m_memory;
    }

    std::uint32_t* xptr() const {
        return m_executable;
    }

    void protect() {
#if defined(__SWITCH__)
        // CodeMemory exposes separate RW/RX aliases. Dynarmic explicitly flushes each emitted
        // block through invalidate(), while this transition flushes the entire JIT allocation.
        // Keep transitions only for the legacy single-mapping fallback.
        if (m_jit.type == JitType_SetProcessMemoryPermission) {
            jitTransitionToExecutable(&m_jit);
        }
#elif defined(__APPLE__) && !TARGET_OS_IPHONE
        pthread_jit_write_protect_np(1);
#elif defined(__APPLE__) || defined(__NetBSD__) || defined(__OpenBSD__)
        mprotect(m_memory, m_size, PROT_READ | PROT_EXEC);
#endif
    }

    void unprotect() {
#if defined(__SWITCH__)
        if (m_jit.type == JitType_SetProcessMemoryPermission) {
            jitTransitionToWritable(&m_jit);
        }
#elif defined(__APPLE__) && !TARGET_OS_IPHONE
        pthread_jit_write_protect_np(0);
#elif defined(__APPLE__) || defined(__NetBSD__) || defined(__OpenBSD__)
        mprotect(m_memory, m_size, PROT_READ | PROT_WRITE);
#endif
    }

    void invalidate(std::uint32_t* mem, std::size_t size) {
#if defined(__SWITCH__)
        const auto address = reinterpret_cast<std::uintptr_t>(mem);
        const auto rw_begin = reinterpret_cast<std::uintptr_t>(m_memory);
        const auto rx_begin = reinterpret_cast<std::uintptr_t>(m_executable);
        std::size_t offset = 0;
        if (address >= rx_begin && address < rx_begin + m_size) {
            offset = address - rx_begin;
        } else if (address >= rw_begin && address < rw_begin + m_size) {
            offset = address - rw_begin;
        }
        armDCacheFlush(reinterpret_cast<void*>(rw_begin + offset), size);
        armICacheInvalidate(reinterpret_cast<void*>(rx_begin + offset), size);
#elif defined(__APPLE__)
        sys_icache_invalidate(mem, size);
#elif defined(_WIN32)
        FlushInstructionCache(GetCurrentProcess(), mem, size);
#else
        static std::size_t icache_line_size = 0x10000;
        static std::size_t dcache_line_size = 0x10000;
        std::uint64_t ctr;
        __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
        const std::size_t isize =
            icache_line_size = std::min<std::size_t>(icache_line_size, 4 << ((ctr >> 0) & 0xf));
        const std::size_t dsize =
            dcache_line_size = std::min<std::size_t>(dcache_line_size, 4 << ((ctr >> 16) & 0xf));
        const std::uintptr_t end = reinterpret_cast<std::uintptr_t>(mem) + size;
        for (std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(mem) & ~(dsize - 1);
             addr < end; addr += dsize) {
            __asm__ volatile("dc cvau, %0" : : "r"(addr) : "memory");
        }
        __asm__ volatile("dsb ish\n" : : : "memory");
        for (std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(mem) & ~(isize - 1);
             addr < end; addr += isize) {
            __asm__ volatile("ic ivau, %0" : : "r"(addr) : "memory");
        }
        __asm__ volatile("dsb ish\nisb\n" : : : "memory");
#endif
    }

    void invalidate_all() {
        invalidate(m_memory, m_size);
    }

protected:
#if defined(__SWITCH__)
    Jit m_jit{};
#endif
    std::uint32_t* m_memory = nullptr;
    std::uint32_t* m_executable = nullptr;
    std::size_t m_size = 0;
};

} // namespace oaknut
