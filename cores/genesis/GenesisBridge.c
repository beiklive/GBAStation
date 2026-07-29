#include "osd.h"

#include "shared.h"
#include "md_ntsc.h"
#include "sms_ntsc.h"

#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

t_config config;

sms_ntsc_t* sms_ntsc = NULL;
md_ntsc_t* md_ntsc = NULL;

char GG_ROM[256];
char AR_ROM[256];
char SK_ROM[256];
char SK_UPMEM[256];
char MD_BIOS[256];
char GG_BIOS[256];
char MS_BIOS_EU[256];
char MS_BIOS_JP[256];
char MS_BIOS_US[256];
char CD_BIOS_EU[256];
char CD_BIOS_US[256];
char CD_BIOS_JP[256];

uint8_t cart_size;

static FILE* open_binary_file(const char* path)
{
#ifdef _WIN32
    const int length = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if (length <= 0)
        return NULL;

    wchar_t* wide_path = (wchar_t*)malloc((size_t)length * sizeof(wchar_t));
    if (!wide_path)
        return NULL;

    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, wide_path, length))
    {
        free(wide_path);
        return NULL;
    }

    FILE* file = _wfopen(wide_path, L"rb");
    free(wide_path);
    return file;
#else
    return fopen(path, "rb");
#endif
}

void gpgx_configure_defaults(void)
{
    int i;
    memset(&config, 0, sizeof(config));

    config.psg_preamp = 150;
    config.fm_preamp = 100;
    config.cdda_volume = 100;
    config.pcm_volume = 100;
    config.hq_fm = 1;
    config.hq_psg = 1;
    config.filter = 1;
    config.lp_range = 0x9999;
    config.low_freq = 880;
    config.high_freq = 5000;
    config.lg = 100;
    config.mg = 100;
    config.hg = 100;
    config.ym2612 = 0;
    config.ym2413 = 2;
    config.mono = 0;

    config.system = 0;
    config.region_detect = 0;
    config.vdp_mode = 0;
    config.master_clock = 0;
    config.force_dtack = 0;
    config.addr_error = 1;
    config.bios = 0;
    config.lock_on = 0;
    config.add_on = HW_ADDON_AUTO;
    config.no_sprite_limit = 0;
    config.enhanced_vscroll = 0;
    config.enhanced_vscroll_limit = 8;

    config.overscan = 0;
    config.aspect_ratio = 0;
    config.gg_extra = 0;
    config.ntsc = 0;
    config.lcd = 0;
    config.render = 0;
    config.left_border = 0;

    input.system[0] = SYSTEM_GAMEPAD;
    input.system[1] = SYSTEM_GAMEPAD;
    for (i = 0; i < MAX_INPUTS; ++i)
        config.input[i].padtype = DEVICE_PAD2B | DEVICE_PAD3B | DEVICE_PAD6B;
}

void osd_input_update(void)
{
}

void ROMCheatUpdate(void)
{
}

uint32_t gpgx_crc32(uint32_t crc, const unsigned char* data, unsigned int length)
{
    unsigned int i;
    crc = ~crc;
    while (length--)
    {
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)-(int32_t)(crc & 1u));
    }
    return ~crc;
}

void error(char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fputc('\n', stderr);
    va_end(args);
}

int load_archive(char* filename, unsigned char* buffer, int maxsize, char* extension)
{
    FILE* file;
    long file_size;
    size_t bytes_read;

    if (!filename || !buffer || maxsize <= 0)
        return 0;

    if (extension)
    {
        const char* dot = strrchr(filename, '.');
        memset(extension, 0, 4);
        if (dot && dot[1])
            strncpy(extension, dot + 1, 3);
    }

    file = open_binary_file(filename);
    if (!file)
        return 0;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return 0;
    }

    file_size = ftell(file);
    if (file_size <= 0 || file_size > maxsize || fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return 0;
    }

    bytes_read = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);
    return bytes_read == (size_t)file_size ? (int)bytes_read : 0;
}
