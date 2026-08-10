// 独立自测：构造一个最小的 PSP ISO（ISO9660）验证 PspMeta 解析。
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#include "../src/core/rom/PspMeta.hpp"

namespace fs = std::filesystem;

static void PutU32BE7(uint8_t* p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
static void PutU32LE(uint8_t* p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

// 生成最小 ISO：根目录含 PSP_GAME 目录，内含 PARAM.SFO（TITLE）与 ICON0.PNG（PNG 头）。
static std::vector<uint8_t> MakeFakeIso(const char* title)
{
    // PARAM.SFO: PSF 头 + 1 个 TITLE 条目（0x0404 UTF-8）。
    // 布局：0x20 头区，0x28 keyTable(entry 20B)，key 串在 entry 后，dataTable 紧随。
    const char* key = "TITLE";
    uint32_t vlen = (uint32_t)strlen(title) + 1;
    uint32_t keyTable = 0x28;
    uint32_t keyPos = keyTable + 20;                    // key 串起始
    uint32_t dataTable = (keyPos + (uint32_t)strlen(key) + 1 + 3) & ~3u; // 4 对齐
    uint32_t total = dataTable + vlen;
    std::vector<uint8_t> sfo(total, 0);
    sfo[0]='P'; sfo[1]='S'; sfo[2]='F';
    PutU32LE(&sfo[8], 0x101);        // version
    PutU32LE(&sfo[12], keyTable);
    PutU32LE(&sfo[16], dataTable);
    PutU32LE(&sfo[20], 1);
    uint8_t* e = sfo.data() + keyTable;
    PutU32LE(e + 0, 20);             // key offset (relative to keyTable)
    PutU32LE(e + 4, 0x0404);         // fmt: utf8
    PutU32LE(e + 8, vlen);           // data len
    PutU32LE(e + 12, vlen);          // data max
    PutU32LE(e + 16, 0);             // data offset (relative to dataTable)
    memcpy(sfo.data() + keyPos, key, strlen(key) + 1);
    memcpy(sfo.data() + dataTable, title, vlen);

    // ICON0.PNG: 最小 PNG 头 + IEND（解析只校验 8 字节签名 + 保存原样）。
    const uint8_t png[12] = {0x89,'P','N','G',0x0D,0x0A,0x1A,0x0A, 0,0,0,0};
    std::vector<uint8_t> icon(png, png + 12);

    // 扇区规划：
    // 16 扇区 PVD
    // 20 扇区 根目录（含 PSP_GAME 目录项）
    // 21 扇区 PSP_GAME 目录（含 PARAM.SFO / ICON0.PNG 文件项）
    // 30 扇区 PARAM.SFO 数据
    // 31 扇区 ICON0.PNG 数据
    const uint32_t S = 2048;
    std::vector<uint8_t> iso(64 * S, 0);

    // 目录记录构造器（ISO9660：记录连续存放，无偶数对齐）
    auto makeRecord = [&](std::vector<uint8_t>& out, const char* name, uint32_t lba, uint32_t size, uint8_t flags) {
        size_t start = out.size();
        out.resize(start + 34 + strlen(name));
        uint8_t* r = out.data() + start;
        uint8_t len = (uint8_t)(34 + strlen(name));
        r[0] = len;
        PutU32BE7(r + 2, lba);
        PutU32BE7(r + 10, size);
        r[25] = flags; // 0x02 = directory
        r[32] = (uint8_t)strlen(name);
        memcpy(r + 33, name, strlen(name));
    };

    // 根目录内容（20 扇区）：. .. PSP_GAME
    std::vector<uint8_t> rootDir;
    makeRecord(rootDir, "\x00", 20, S, 0x02);     // .
    makeRecord(rootDir, "\x01", 20, S, 0x02);     // ..
    makeRecord(rootDir, "PSP_GAME", 21, S, 0x02); // PSP_GAME
    memcpy(iso.data() + 20 * S, rootDir.data(), rootDir.size());

    // PSP_GAME 目录（21 扇区）：. .. PARAM.SFO ICON0.PNG
    std::vector<uint8_t> gameDir;
    makeRecord(gameDir, "\x00", 21, S, 0x02);
    makeRecord(gameDir, "\x01", 21, S, 0x02);
    makeRecord(gameDir, "PARAM.SFO", 30, (uint32_t)sfo.size(), 0);
    makeRecord(gameDir, "ICON0.PNG", 31, (uint32_t)icon.size(), 0);
    memcpy(iso.data() + 21 * S, gameDir.data(), gameDir.size());

    // 文件数据
    memcpy(iso.data() + 30 * S, sfo.data(), sfo.size());
    memcpy(iso.data() + 31 * S, icon.data(), icon.size());

    // PVD（16 扇区）：type=1, CD001, 根目录记录 at 156
    uint8_t* pvd = iso.data() + 16 * S;
    pvd[0] = 1;
    memcpy(pvd + 1, "CD001", 5);
    uint8_t* rr = pvd + 156;
    rr[0] = 34;
    PutU32BE7(rr + 2, 20); // root lba
    PutU32BE7(rr + 10, S); // root size
    rr[25] = 0x02;
    rr[32] = 0;

    return iso;
}

int main()
{
    const char* tmp = "psp_meta_test";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    // 1. ISO 测试
    auto iso = MakeFakeIso("Metal Gear Solid Portable Ops");
    {
        std::ofstream f(std::string(tmp) + "/test.iso", std::ios::binary);
        f.write((const char*)iso.data(), iso.size());
    }
    std::string title = beiklive::psp_meta::ExtractTitle(std::string(tmp) + "/test.iso");
    printf("ISO title: [%s]\n", title.c_str());
    if (title != "Metal Gear Solid Portable Ops") { printf("FAIL iso title\n"); return 1; }
    std::string icon = beiklive::psp_meta::ExtractIcon0(std::string(tmp) + "/test.iso", std::string(tmp) + "/cache");
    printf("ISO icon: [%s]\n", icon.c_str());
    if (!fs::exists(icon)) { printf("FAIL iso icon\n"); return 1; }

    // 2. PBP 测试：\x00PBP + 8 偏移 + PARAM.SFO/ICON0
    {
        std::vector<uint8_t> pbp(0x28, 0);
        pbp[0] = 0x00; pbp[1] = 'P'; pbp[2] = 'B'; pbp[3] = 'P';
        PutU32LE(&pbp[4], 0x10000);
        // 子文件偏移：0=PARAM_SFO 1=ICON0
        auto iso2 = MakeFakeIso("PBP Test Game");
        // 从 iso 里挖 PARAM.SFO / ICON0 数据（扇区 30/31 已知）
        std::vector<uint8_t> sfo(iso2.begin() + 30 * 2048, iso2.begin() + 30 * 2048 + 0x100);
        std::vector<uint8_t> ic(iso2.begin() + 31 * 2048, iso2.begin() + 31 * 2048 + 12);
        PutU32LE(&pbp[8 + 0 * 4], 0x28);
        PutU32LE(&pbp[8 + 1 * 4], 0x28 + (uint32_t)sfo.size());
        PutU32LE(&pbp[8 + 2 * 4], 0x28 + (uint32_t)sfo.size() + (uint32_t)ic.size());
        pbp.insert(pbp.end(), sfo.begin(), sfo.end());
        pbp.insert(pbp.end(), ic.begin(), ic.end());
        std::ofstream f(std::string(tmp) + "/test.pbp", std::ios::binary);
        f.write((const char*)pbp.data(), pbp.size());
    }
    title = beiklive::psp_meta::ExtractTitle(std::string(tmp) + "/test.pbp");
    printf("PBP title: [%s]\n", title.c_str());
    if (title != "PBP Test Game") { printf("FAIL pbp title\n"); return 1; }
    icon = beiklive::psp_meta::ExtractIcon0(std::string(tmp) + "/test.pbp", std::string(tmp) + "/cache");
    if (!fs::exists(icon)) { printf("FAIL pbp icon\n"); return 1; }

    // 3. 容错：非 PSP 文件返回空
    {
        std::ofstream f(std::string(tmp) + "/bad.iso", std::ios::binary);
        std::vector<uint8_t> junk(100, 0xAA);
        f.write((const char*)junk.data(), junk.size());
    }
    if (!beiklive::psp_meta::ExtractTitle(std::string(tmp) + "/bad.iso").empty()) {
        printf("FAIL bad iso\n"); return 1;
    }

    printf("ALL TESTS PASSED\n");
    fs::remove_all(tmp);
    return 0;
}
