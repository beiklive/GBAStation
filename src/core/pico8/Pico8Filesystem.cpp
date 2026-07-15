#include "Pico8Filesystem.hpp"

#include "core/common.h"

#include "host.h"
#include "lodepng.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace
{
    uint8_t g_inputDown = 0;
    uint8_t g_inputHeld = 0;

    constexpr std::array<std::array<uint8_t, 4>, 16> PICO8_PALETTE{{
        {{2, 4, 8, 255}},       {{29, 43, 83, 255}},
        {{126, 37, 83, 255}},   {{0, 135, 81, 255}},
        {{171, 82, 54, 255}},   {{95, 87, 79, 255}},
        {{194, 195, 199, 255}}, {{255, 241, 232, 255}},
        {{255, 0, 77, 255}},    {{255, 163, 0, 255}},
        {{255, 236, 39, 255}},  {{0, 228, 54, 255}},
        {{41, 173, 255, 255}},  {{131, 118, 156, 255}},
        {{255, 119, 168, 255}}, {{255, 204, 170, 255}},
    }};

    std::string lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return value;
    }

    bool isPico8Cart(const fs::path& path)
    {
        const std::string filename = lower(path.filename().string());
        return filename.size() > 3 &&
            (filename.rfind(".p8") == filename.size() - 3 ||
             filename.rfind(".png") == filename.size() - 4);
    }

    std::string displayName(const fs::path& path)
    {
        std::string name = path.filename().string();
        const std::string lowered = lower(name);
        if (lowered.size() > 7 &&
            lowered.rfind(".p8.png") == lowered.size() - 7)
            name.resize(name.size() - 7);
        else if (lowered.size() > 4 &&
                 lowered.rfind(".png") == lowered.size() - 4)
            name.resize(name.size() - 4);
        else if (lowered.size() > 3 &&
                 lowered.rfind(".p8") == lowered.size() - 3)
            name.resize(name.size() - 3);
        return name;
    }

    std::string safeCacheName(const std::string& name)
    {
        std::string result;
        result.reserve(name.size());
        for (unsigned char ch : name) {
            if (std::isalnum(ch) || ch == '-' || ch == '_')
                result.push_back(static_cast<char>(ch));
            else
                result.push_back('_');
        }
        if (result.empty())
            result = "cart";
        return result + "_label.png";
    }

    uint64_t stablePathHash(const std::string& value)
    {
        uint64_t hash = 1469598103934665603ULL;
        for (unsigned char ch : value) {
            hash ^= ch;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    std::string safeStateName(const std::string& cartPath)
    {
        std::string stem = displayName(fs::path(cartPath));
        for (char& ch : stem) {
            const unsigned char value = static_cast<unsigned char>(ch);
            if (!std::isalnum(value) && ch != '-' && ch != '_')
                ch = '_';
        }
        if (stem.empty())
            stem = "cart";
        std::ostringstream name;
        name << stem << '_' << std::hex << std::setw(16) << std::setfill('0')
             << stablePathHash(cartPath) << ".p8state";
        return name.str();
    }
}

namespace beiklive::pico8
{
    std::string Filesystem::rootPath()
    {
#ifdef __SWITCH__
        return "sdmc:/GBAStation/pico8";
#else
        return (fs::path(beiklive::path::GetRootPath()) /
                "GBAStation" / "pico8").string();
#endif
    }

    std::string Filesystem::gamesPath()
    {
        return (fs::path(rootPath()) / "games").string();
    }

    std::string Filesystem::corePath()
    {
        return (fs::path(rootPath()) / "core").string();
    }

    std::string Filesystem::cachePath()
    {
        return (fs::path(rootPath()) / "cache").string();
    }

    std::string Filesystem::cartDataPath()
    {
        return (fs::path(rootPath()) / "cdata").string();
    }

    std::string Filesystem::statesPath()
    {
        return (fs::path(rootPath()) / "states").string();
    }

    std::string Filesystem::quickStatePath(const std::string& cartPath)
    {
        return (fs::path(statesPath()) / safeStateName(cartPath)).string();
    }

    std::string Filesystem::runtimePath()
    {
        return (fs::path(corePath()) / "pico8.dat").string();
    }

    std::string Filesystem::fontPath()
    {
        return (fs::path(corePath()) / "font.ttf").string();
    }

    bool Filesystem::ensureDirectories()
    {
        std::error_code ec;
        fs::create_directories(gamesPath(), ec);
        if (ec) return false;
        fs::create_directories(corePath(), ec);
        if (ec) return false;
        fs::create_directories(cachePath(), ec);
        if (ec) return false;
        fs::create_directories(cartDataPath(), ec);
        if (ec) return false;
        fs::create_directories(statesPath(), ec);
        return !ec;
    }

    std::string Filesystem::buildLabelCache(const std::string& cartPath,
                                            const std::string& gameName)
    {
        const fs::path cacheFile = fs::path(cachePath()) /
            safeCacheName(gameName);
        std::error_code ec;
        if (fs::exists(cacheFile, ec) &&
            fs::last_write_time(cacheFile, ec) >=
                fs::last_write_time(cartPath, ec))
            return cacheFile.string();

        std::ifstream input(cartPath, std::ios::binary);
        if (!input)
            return {};

        std::string line;
        bool inLabel = false;
        std::vector<std::string> rows;
        rows.reserve(128);
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (!inLabel) {
                if (line == "__label__")
                    inLabel = true;
                continue;
            }
            if (line.size() >= 2 && line.front() == '_' && line.back() == '_')
                break;
            if (line.size() >= 128) {
                rows.push_back(line.substr(0, 128));
                if (rows.size() == 128)
                    break;
            }
        }
        if (rows.size() != 128)
            return {};

        std::vector<unsigned char> rgba(128 * 128 * 4, 0);
        for (size_t y = 0; y < 128; ++y) {
            for (size_t x = 0; x < 128; ++x) {
                const char ch = rows[y][x];
                int color = 0;
                if (ch >= '0' && ch <= '9') color = ch - '0';
                else if (ch >= 'a' && ch <= 'f') color = ch - 'a' + 10;
                else if (ch >= 'A' && ch <= 'F') color = ch - 'A' + 10;
                const size_t offset = (y * 128 + x) * 4;
                rgba[offset + 0] = PICO8_PALETTE[color][0];
                rgba[offset + 1] = PICO8_PALETTE[color][1];
                rgba[offset + 2] = PICO8_PALETTE[color][2];
                rgba[offset + 3] = 255;
            }
        }

        fs::create_directories(cacheFile.parent_path(), ec);
        if (lodepng_encode32_file(cacheFile.string().c_str(), rgba.data(),
                                  128, 128) != 0)
            return {};
        return cacheFile.string();
    }

    std::vector<GameEntry> Filesystem::scanGames()
    {
        ensureDirectories();
        std::vector<GameEntry> games;
        std::error_code ec;
        for (const auto& item : fs::directory_iterator(gamesPath(), ec)) {
            if (ec || !item.is_regular_file() || !isPico8Cart(item.path()))
                continue;
            GameEntry game;
            game.name = displayName(item.path());
            game.path = item.path().string();
            const std::string lowered = lower(item.path().filename().string());
            if (lowered.size() > 4 &&
                lowered.rfind(".png") == lowered.size() - 4) {
                game.coverPath = game.path;
            } else {
                const fs::path cacheFile = fs::path(cachePath()) /
                    safeCacheName(game.name);
                std::error_code cacheError;
                if (fs::exists(cacheFile, cacheError) && !cacheError &&
                    fs::last_write_time(cacheFile, cacheError) >=
                        fs::last_write_time(item.path(), cacheError) &&
                    !cacheError)
                    game.coverPath = cacheFile.string();
            }
            games.push_back(std::move(game));
        }
        std::sort(games.begin(), games.end(),
            [](const GameEntry& lhs, const GameEntry& rhs) {
                return lower(lhs.name) < lower(rhs.name);
            });
        return games;
    }

    std::vector<std::string> Filesystem::listGamePaths()
    {
        ensureDirectories();
        std::vector<std::string> paths;
        std::error_code ec;
        for (const auto& item : fs::directory_iterator(gamesPath(), ec)) {
            if (ec || !item.is_regular_file() || !isPico8Cart(item.path()))
                continue;
            paths.push_back(item.path().string());
        }
        std::sort(paths.begin(), paths.end(),
            [](const std::string& lhs, const std::string& rhs) {
                return lower(lhs) < lower(rhs);
            });
        return paths;
    }

    std::string Filesystem::resolveCover(const GameEntry& game)
    {
        if (!game.coverPath.empty())
            return game.coverPath;
        const std::string lowered = lower(fs::path(game.path).filename().string());
        if (lowered.size() > 4 &&
            lowered.rfind(".png") == lowered.size() - 4)
            return game.path;
        return buildLabelCache(game.path, game.name);
    }

    namespace host_bridge
    {
        void setInput(uint8_t down, uint8_t held)
        {
            g_inputDown = down;
            g_inputHeld = held;
        }
    }
}

Host::Host(int, int)
{
    beiklive::pico8::Filesystem::ensureDirectories();
    setPlatformParams(128, 128, 0, 0, 0,
        beiklive::pico8::Filesystem::rootPath() + "/",
        "cartpath = \"" + beiklive::pico8::Filesystem::gamesPath() + "/\"\n",
        beiklive::pico8::Filesystem::gamesPath());
    setUpPaletteColors();
}

void Host::setUpPaletteColors()
{
    const Color palette[16] = {
        COLOR_00, COLOR_01, COLOR_02, COLOR_03,
        COLOR_04, COLOR_05, COLOR_06, COLOR_07,
        COLOR_08, COLOR_09, COLOR_10, COLOR_11,
        COLOR_12, COLOR_13, COLOR_14, COLOR_15,
    };
    std::copy(std::begin(palette), std::end(palette), _paletteColors);
    for (int i = 16; i < 128; ++i) _paletteColors[i] = {0, 0, 0, 0};
    const Color extended[16] = {
        COLOR_128, COLOR_129, COLOR_130, COLOR_131,
        COLOR_132, COLOR_133, COLOR_134, COLOR_135,
        COLOR_136, COLOR_137, COLOR_138, COLOR_139,
        COLOR_140, COLOR_141, COLOR_142, COLOR_143,
    };
    std::copy(std::begin(extended), std::end(extended), _paletteColors + 128);
}

Color* Host::GetPaletteColors() { return _paletteColors; }
void Host::oneTimeSetup(Audio*) {}
void Host::oneTimeCleanup() {}
void Host::unpackCarts() {}
void Host::setTargetFps(int) {}
bool Host::shouldRunMainLoop() { return true; }
bool Host::shouldQuit() { return false; }
void Host::changeStretch() {}
void Host::forceStretch(StretchOption value) { stretch = value; }
void Host::waitForTargetFps() {}
void Host::drawFrame(uint8_t*, uint8_t*, uint8_t) {}
bool Host::shouldFillAudioBuff() { return false; }
void* Host::getAudioBufferPointer() { return nullptr; }
size_t Host::getAudioBufferSize() { return 0; }
void Host::playFilledAudioBuffer() {}
double Host::deltaTMs() { return 1000.0 / 60.0; }

InputState_t Host::scanInput()
{
    return {g_inputDown, g_inputHeld, 0, 0, 0, false, ""};
}

std::vector<std::string> Host::listcarts()
{
    return beiklive::pico8::Filesystem::listGamePaths();
}

std::vector<std::string> Host::listdirs()
{
    std::vector<std::string> result;
    std::error_code ec;
    for (const auto& item : fs::directory_iterator(_cartDirectory, ec))
        if (!ec && item.is_directory())
            result.push_back(item.path().filename().string());
    return result;
}

void Host::overrideLogFilePrefix(const char* prefix)
{
    _logFilePrefix = prefix ? prefix : "";
}
const char* Host::logFilePrefix() { return _logFilePrefix.c_str(); }
std::string Host::customBiosLua() { return _customBiosLua; }
std::string Host::getCartDirectory() { return _cartDirectory; }

std::string Host::getCartDataFile(std::string key)
{
    return (fs::path(beiklive::pico8::Filesystem::cartDataPath()) /
            (key + ".p8d.txt")).string();
}

std::string Host::getCartDataFileContents(std::string key)
{
    std::ifstream input(getCartDataFile(std::move(key)), std::ios::binary);
    return input ? std::string(std::istreambuf_iterator<char>(input), {}) : "";
}

void Host::saveCartData(std::string key, std::string contents)
{
    std::ofstream output(getCartDataFile(std::move(key)),
                         std::ios::binary | std::ios::trunc);
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

size_t Host::getFileContents(std::string filename, char* buffer)
{
    std::ifstream input((fs::path(beiklive::pico8::Filesystem::cartDataPath()) /
                         filename).string(), std::ios::binary);
    if (!input || !buffer) return 0;
    input.seekg(0, std::ios::end);
    const size_t size = static_cast<size_t>(input.tellg());
    input.seekg(0, std::ios::beg);
    input.read(buffer, static_cast<std::streamsize>(size));
    return size;
}

void Host::writeBufferToFile(std::string filename, char* buffer, size_t length)
{
    std::ofstream output((fs::path(beiklive::pico8::Filesystem::cartDataPath()) /
                          filename).string(), std::ios::binary | std::ios::trunc);
    if (buffer && output)
        output.write(buffer, static_cast<std::streamsize>(length));
}

void Host::setCartDirectory(std::string directory)
{
    _cartDirectory = std::move(directory);
}

int Host::getSetting(std::string name)
{
    if (name == "kbmode") return kbmode;
    if (name == "resizekey") return resizekey;
    if (name == "stretch") return stretch;
    if (name == "menustyle") return menustyle;
    if (name == "bgcolor") return bgcolor;
    if (name == "p8_bgcolor") return bgcolor == White ? 7 : 5;
    if (name == "p8_textcolor") return bgcolor == White ? 0 : 7;
    return 0;
}

void Host::setSetting(std::string name, int value)
{
    if (name == "kbmode") kbmode = static_cast<KeyboardOption>(value);
    else if (name == "resizekey") resizekey = static_cast<ResizekeyOption>(value);
    else if (name == "stretch") stretch = static_cast<StretchOption>(value);
    else if (name == "menustyle") menustyle = static_cast<MenuStyleOption>(value);
    else if (name == "bgcolor") bgcolor = static_cast<BgColorOption>(value);
}

void Host::setPlatformParams(int, int, uint32_t, uint32_t, uint32_t,
                             std::string logPrefix,
                             std::string biosLua,
                             std::string cartDirectory)
{
    _logFilePrefix = std::move(logPrefix);
    _customBiosLua = std::move(biosLua);
    _cartDirectory = std::move(cartDirectory);
}

void Host::loadSettingsIni() {}
void Host::saveSettingsIni() {}
