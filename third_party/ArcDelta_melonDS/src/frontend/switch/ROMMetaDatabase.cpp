#include "ROMMetaDatabase.h"

#include <string>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include <unordered_map>

#include <arm_neon.h>

namespace ROMMetaDatabase
{

std::vector<char> TitlesData;
std::vector<ROMMeta> Database;
std::unordered_map<std::string, int> DatabaseByName;
Gfx::Atlas IconAtlas{DkImageFormat_RGB5A1_Unorm, 2};

int TitleLanguage;

const char* ROMMeta::Title(int lang)
{
    return &TitlesData[TitlesOffset[lang]];
}

void UpdateTexture()
{
    ROMMetaDatabase::IconAtlas.IssueUpload();
}

void Init()
{
    u64 langCode;
    setGetSystemLanguage(&langCode);
    char langCodeStr[8];
    memcpy(langCodeStr, &langCode, 8);
    if (!strcmp(langCodeStr, "ja"))
        TitleLanguage = ROMMetaDatabase::titleLang_Japanese;
    else if (!strcmp(langCodeStr, "fr") || !strcmp(langCodeStr, "fr-CA"))
        TitleLanguage = ROMMetaDatabase::titleLang_French;
    else if (!strcmp(langCodeStr, "de"))
        TitleLanguage = ROMMetaDatabase::titleLang_German;
    else if (!strcmp(langCodeStr, "it"))
        TitleLanguage = ROMMetaDatabase::titleLang_Italian;
    else if (!strcmp(langCodeStr, "es") || !strcmp(langCodeStr, "es-419"))
        TitleLanguage = ROMMetaDatabase::titleLang_Spanish;
    else // the rest of you get English
        TitleLanguage = ROMMetaDatabase::titleLang_English;
}

int QueryMeta(const char* romname, const char* path)
{
    auto it = DatabaseByName.find(romname);
    if (it != DatabaseByName.end())
    {
        return it->second;
    }

    ROMMeta meta;
    meta.HasIcon = false;

    FILE* rom = fopen(path, "rb");
    assert(rom && "rom needs to be readable!");

    u32 iconTitleOffset = 0;
    fseek(rom, 0x68, SEEK_SET);
    fread(&iconTitleOffset, 4, 1, rom);

    bool hasTitle = false;
    if (iconTitleOffset)
    {
        meta.HasIcon = true;
        fseek(rom, iconTitleOffset, SEEK_SET);
        u8 data[0xA40];
        fread(data, sizeof(data), 1, rom);

        for (u32 i = 0; i < titlesLang_Count; i++)
        {
            meta.TitlesOffset[i] = TitlesData.size();
            u16* string = (u16*)&data[0x240 + i * 0x100];

            hasTitle |= *string != 0;

            TitlesData.resize(TitlesData.size() + 3 * 129);
            u32 utf8Chars = utf16_to_utf8((u8*)&TitlesData[meta.TitlesOffset[i]], string, 3*128);
            TitlesData[utf8Chars] = '\0';
            TitlesData.resize(meta.TitlesOffset[i] + utf8Chars + 1);
        }

        // call me crazy, but I like to use neon for pretty trivial things
        // gotta go fast!

        u8* iconDst = IconAtlas.Pack(32, 32, meta.Icon);
        u32 offset = 0;

        uint8x16x2_t palette = vld2q_u8(&data[0x220]);

        // first palette entry is transparent, the rest is opaque
        palette.val[1] = vorrq_u8(palette.val[1], vdupq_n_u8(0x80));
        palette.val[1][0] &= ~0x80;

        uint8x16_t nibbleMask = vdupq_n_u8(0xF);
        for (u32 i = 0; i < 16; i++)
        {
            // there are 16 8*8 tiles
            uint8x16x2_t packed = vld1q_u8_x2(&data[0x20 + i * 8*8/2]);

            // each vector contains two lines of icon indices
            uint8x16_t unpacked0 = vzip1q_u8(vandq_u8(packed.val[0], nibbleMask), vshrq_n_u8(packed.val[0], 4));
            uint8x16_t unpacked1 = vzip2q_u8(vandq_u8(packed.val[0], nibbleMask), vshrq_n_u8(packed.val[0], 4));

            uint8x16_t unpacked2 = vzip1q_u8(vandq_u8(packed.val[1], nibbleMask), vshrq_n_u8(packed.val[1], 4));
            uint8x16_t unpacked3 = vzip2q_u8(vandq_u8(packed.val[1], nibbleMask), vshrq_n_u8(packed.val[1], 4));

            // time to palettise
            uint8x16_t palettisedLo = vqtbl1q_u8(palette.val[0], unpacked0);
            uint8x16_t palettisedHi = vqtbl1q_u8(palette.val[1], unpacked0);

            vst2_u8(iconDst + offset, {vget_low_u8(palettisedLo), vget_low_u8(palettisedHi)});
            vst2_u8(iconDst + offset + IconAtlas.PackStride(), {vget_high_u8(palettisedLo), vget_high_u8(palettisedHi)});

            palettisedLo = vqtbl1q_u8(palette.val[0], unpacked1);
            palettisedHi = vqtbl1q_u8(palette.val[1], unpacked1);

            vst2_u8(iconDst + offset + IconAtlas.PackStride() * 2, {vget_low_u8(palettisedLo), vget_low_u8(palettisedHi)});
            vst2_u8(iconDst + offset + IconAtlas.PackStride() * 3, {vget_high_u8(palettisedLo), vget_high_u8(palettisedHi)});

            palettisedLo = vqtbl1q_u8(palette.val[0], unpacked2);
            palettisedHi = vqtbl1q_u8(palette.val[1], unpacked2);

            vst2_u8(iconDst + offset + IconAtlas.PackStride() * 4, {vget_low_u8(palettisedLo), vget_low_u8(palettisedHi)});
            vst2_u8(iconDst + offset + IconAtlas.PackStride() * 5, {vget_high_u8(palettisedLo), vget_high_u8(palettisedHi)});
            
            palettisedLo = vqtbl1q_u8(palette.val[0], unpacked3);
            palettisedHi = vqtbl1q_u8(palette.val[1], unpacked3);

            vst2_u8(iconDst + offset + IconAtlas.PackStride() * 6, {vget_low_u8(palettisedLo), vget_low_u8(palettisedHi)});
            vst2_u8(iconDst + offset + IconAtlas.PackStride() * 7, {vget_high_u8(palettisedLo), vget_high_u8(palettisedHi)});

            offset += 8*2;
            if (offset == 32*2)
            {
                iconDst += IconAtlas.PackStride() * 8;
                offset = 0;
            }
        }
    }
    fclose(rom);

    if (!hasTitle)
    {
        for (u32 i = 0; i < titlesLang_Count; i++)
            meta.TitlesOffset[i] = TitlesData.size();

        TitlesData.resize(TitlesData.size() + strlen(romname) + 1);
        strcpy(&TitlesData[meta.TitlesOffset[0]], romname);
    }

    int result = Database.size();
    DatabaseByName[romname] = result;
    Database.push_back(meta);
    return result;
}

}
