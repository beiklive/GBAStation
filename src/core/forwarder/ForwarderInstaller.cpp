#include "core/forwarder/ForwarderInstaller.hpp"

#include "core/common.h"

#include <borealis.hpp>

#ifdef __SWITCH__
#include <borealis/extern/nanovg/stb_image.h>
#include <jpeglib.h>
#include <switch.h>

#include <csetjmp>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace sphaira
{
    Result installForwarder(const std::string& nroPath, const std::string& args,
                            const std::string& name, const std::string& author,
                            const std::vector<u8>& icon,
                            const std::string& legacyArgs);
}
#endif

namespace beiklive::forwarder
{
namespace
{
#ifdef __SWITCH__
    struct JpegError
    {
        jpeg_error_mgr base{};
        std::jmp_buf jump{};
    };

    void jpegErrorExit(j_common_ptr context)
    {
        auto* error = reinterpret_cast<JpegError*>(context->err);
        std::longjmp(error->jump, 1);
    }

    std::vector<unsigned char> readFile(const std::string& path)
    {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input)
            return {};

        const auto size = input.tellg();
        if (size <= 0)
            return {};

        std::vector<unsigned char> data(static_cast<size_t>(size));
        input.seekg(0);
        input.read(reinterpret_cast<char*>(data.data()), size);
        return input ? data : std::vector<unsigned char>{};
    }

    std::vector<unsigned char> resizeRgba(const unsigned char* source, int width, int height)
    {
        constexpr int outputSize = 256;
        std::vector<unsigned char> output(outputSize * outputSize * 4);
        for (int y = 0; y < outputSize; ++y)
        {
            const int sourceY = y * height / outputSize;
            for (int x = 0; x < outputSize; ++x)
            {
                const int sourceX = x * width / outputSize;
                const auto* src = source + (sourceY * width + sourceX) * 4;
                auto* dst = output.data() + (y * outputSize + x) * 4;
                std::memcpy(dst, src, 4);
            }
        }
        return output;
    }

    std::vector<unsigned char> encodeJpeg(const std::vector<unsigned char>& rgba)
    {
        jpeg_compress_struct compressor{};
        JpegError error{};
        compressor.err = jpeg_std_error(&error.base);
        error.base.error_exit = jpegErrorExit;
        if (setjmp(error.jump))
        {
            jpeg_destroy_compress(&compressor);
            return {};
        }

        jpeg_create_compress(&compressor);
        unsigned char* output = nullptr;
        unsigned long outputSize = 0;
        jpeg_mem_dest(&compressor, &output, &outputSize);
        compressor.image_width = 256;
        compressor.image_height = 256;
        compressor.input_components = 3;
        compressor.in_color_space = JCS_RGB;
        jpeg_set_defaults(&compressor);
        jpeg_set_quality(&compressor, 93, TRUE);
        jpeg_start_compress(&compressor, TRUE);

        std::vector<unsigned char> row(256 * 3);
        while (compressor.next_scanline < compressor.image_height)
        {
            const auto* source = rgba.data() + compressor.next_scanline * 256 * 4;
            for (int x = 0; x < 256; ++x)
            {
                row[x * 3] = source[x * 4];
                row[x * 3 + 1] = source[x * 4 + 1];
                row[x * 3 + 2] = source[x * 4 + 2];
            }
            JSAMPROW rows[] = {row.data()};
            jpeg_write_scanlines(&compressor, rows, 1);
        }

        jpeg_finish_compress(&compressor);
        std::vector<unsigned char> result(output, output + outputSize);
        std::free(output);
        jpeg_destroy_compress(&compressor);
        return result;
    }

    std::vector<unsigned char> loadForwarderIcon(const std::string& path)
    {
        const auto encoded = readFile(path);
        if (encoded.empty())
            return {};

        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* pixels = stbi_load_from_memory(
            encoded.data(), static_cast<int>(encoded.size()), &width, &height, &channels, 4);
        if (!pixels || width <= 0 || height <= 0)
        {
            if (pixels)
                stbi_image_free(pixels);
            return {};
        }

        auto resized = resizeRgba(pixels, width, height);
        stbi_image_free(pixels);
        return encodeJpeg(resized);
    }

    std::string quoteArgument(const std::string& value)
    {
        std::string quoted(1, '"');
        for (char ch : value)
        {
            if (ch == '\\' || ch == '"')
                quoted.push_back('\\');
            quoted.push_back(ch);
        }
        quoted.push_back('"');
        return quoted;
    }

    std::string normalizeNroPath(std::string path)
    {
        if (!path.empty() && path.front() == '/')
            path.insert(0, "sdmc:");
        return path;
    }
#endif
}

bool isSupported()
{
#ifdef __SWITCH__
    return true;
#else
    return false;
#endif
}

InstallResult installGame(const beiklive::GameEntry& entry)
{
#ifdef __SWITCH__
    if (entry.path.empty() || entry.title.empty())
        return {false, 0, "游戏信息不完整"};
    if (entry.logoPath.empty())
        return {false, 0, "该游戏没有可用的封面图"};

    const auto icon = loadForwarderIcon(entry.logoPath);
    if (icon.empty())
        return {false, 0, "封面图读取或转换失败"};

    const bool isNds =
        entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS);
    const bool isThreeDs =
        entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS);
    const bool isArcade =
        entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuArcade);
    const bool isDreamcast =
        entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast);
    const bool isPsp =
        entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP);
    const bool isPs1 =
        entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuPS1);
    const bool isSaturn =
        entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuSaturn);
    const bool isDolphin =
        entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuDolphin);
    const std::string mainNro = "sdmc:/switch/GBAStation.nro";
    std::string nroPath = mainNro;
    if (isNds)
        nroPath = normalizeNroPath(GET_SETTING_KEY_STR(
            "nds.externalNro.path", "/GBAStation/core/GBAStationNDSStub.nro"));
    else if (isThreeDs)
        nroPath = normalizeNroPath(GET_SETTING_KEY_STR(
            "3ds.externalNro.path", "/GBAStation/core/GBAStation3DSStub.nro"));
    else if (isArcade)
        nroPath = normalizeNroPath(GET_SETTING_KEY_STR(
            "arcade.externalNro.path", "/GBAStation/core/GBAStationFBNeoStub.nro"));
    else if (isDreamcast)
        nroPath = normalizeNroPath(GET_SETTING_KEY_STR(
            "dc.externalNro.path", "/GBAStation/core/GBAStationFlycastStub.nro"));
    else if (isPsp)
        nroPath = normalizeNroPath(GET_SETTING_KEY_STR(
            "psp.externalNro.path", "/GBAStation/core/GBAStationPPSSPPStub.nro"));
    else if (isPs1)
        nroPath = normalizeNroPath(GET_SETTING_KEY_STR(
            "core.ps1.externalNro.path", "/GBAStation/core/GBAStationDuckStationStub.nro"));
    else if (isSaturn)
        nroPath = normalizeNroPath(GET_SETTING_KEY_STR(
            "saturn.externalNro.path", "/GBAStation/core/GBAStationYabaSanshiroStub.nro"));
    else if (isDolphin)
        nroPath = normalizeNroPath(GET_SETTING_KEY_STR(
            "dolphin.externalNro.path", "/GBAStation/core/GBAStationDolphinStub.nro"));

    std::string args = quoteArgument(entry.path);
    std::string legacyArgs;
    if (isNds || isThreeDs)
    {
        legacyArgs = args + " --return " + quoteArgument(mainNro);
        args += " --exit-to-home";
    }
    else if (isArcade || isDreamcast || isPsp || isPs1 || isSaturn || isDolphin)
    {
        legacyArgs = args + " --return " + quoteArgument(mainNro);
    }

    const Result rc = sphaira::installForwarder(
        nroPath, args, entry.title, "GBAStation", icon, legacyArgs);
    if (R_FAILED(rc))
    {
        char buffer[96]{};
        std::snprintf(buffer, sizeof(buffer), "安装失败，错误码 0x%08X", rc);
        return {false, rc, buffer};
    }
    return {true, rc, "已安装到 Switch 主界面"};
#else
    (void)entry;
    return {false, 0, "该功能仅支持 Switch"};
#endif
}

void showInstallDialog(const beiklive::GameEntry& entry)
{
    auto* dialog = new brls::Dialog("是否安装游戏到 Switch 主界面？");
    dialog->addButton("确定", [entry]() {
        auto* installingDialog = new brls::Dialog("正在安装游戏前端...");
        installingDialog->setCancelable(false);
        installingDialog->open();

        brls::delay(100, [entry, installingDialog]() {
            const auto result = installGame(entry);
            installingDialog->close([result]() {
                auto* resultDialog = new brls::Dialog(
                    result.success ? "安装完成" : result.message);
                resultDialog->addButton("确认", []() {});
                resultDialog->open();
            });
        });
    });
    dialog->addButton("取消", []() {});
    dialog->open();
}
}
