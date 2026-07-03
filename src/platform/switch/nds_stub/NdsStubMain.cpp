#include <cstdarg>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <iterator>
#include <optional>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>
#include <switch.h>

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct UiFrame {
    uint32_t* pixels = nullptr;
    int stridePixels = 0;
    int width = kScreenWidth;
    int height = kScreenHeight;
};

struct GameInfo {
    std::string romPath;
    std::string title;
    std::string savePath;
    std::string cheatPath;
    int internalResolution = 1;
};

enum class MenuAction {
    Resume,
    SaveState,
    LoadState,
    Cheats,
    Display,
    Reset,
    Exit,
};

struct MenuItem {
    const char* label;
    MenuAction action;
};

constexpr MenuItem kMenuItems[] = {
    {"Resume Game", MenuAction::Resume},
    {"Save State", MenuAction::SaveState},
    {"Load State", MenuAction::LoadState},
    {"Cheats", MenuAction::Cheats},
    {"Display Settings", MenuAction::Display},
    {"Reset Game", MenuAction::Reset},
    {"Exit Game", MenuAction::Exit},
};

void appendLog(const char* format, ...)
{
    char line[1024] = {};

    va_list args;
    va_start(args, format);
    std::vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    constexpr const char* paths[] = {
        "sdmc:/GBAStation/log/GBAStationNDSStub.log",
        "/GBAStation/log/GBAStationNDSStub.log",
        "sdmc:/GBAStationNDSStub.log",
        "/GBAStationNDSStub.log",
    };

    for (const char* path : paths)
    {
        FILE* fp = std::fopen(path, "a");
        if (!fp)
            continue;

        std::fprintf(fp, "%s\n", line);
        std::fflush(fp);
        std::fclose(fp);
    }
}

std::string quoteArg(const std::string& value)
{
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    for (char c : value)
    {
        if (c == '"' || c == '\\')
            out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

bool fileExists(const std::string& path)
{
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp)
        return false;
    std::fclose(fp);
    return true;
}

std::string normalizePathForCompare(std::string path)
{
    for (char& c : path)
    {
        if (c == '\\')
            c = '/';
    }

    if (path.rfind("sdmc:", 0) == 0)
        path.erase(0, 5);

    while (path.size() > 1 && path[0] == '/' && path[1] == '/')
        path.erase(0, 1);

    return path;
}

bool endsWithNoCase(const std::string& value, const char* suffix)
{
    const size_t suffixLen = std::strlen(suffix);
    if (value.size() < suffixLen)
        return false;

    const size_t offset = value.size() - suffixLen;
    for (size_t i = 0; i < suffixLen; ++i)
    {
        const char a = value[offset + i];
        const char b = suffix[i];
        if (std::tolower(static_cast<unsigned char>(a)) !=
            std::tolower(static_cast<unsigned char>(b)))
            return false;
    }
    return true;
}

std::string jsonString(const nlohmann::json& item, const char* key)
{
    const auto it = item.find(key);
    if (it == item.end() || !it->is_string())
        return "";
    return it->get<std::string>();
}

int jsonInt(const nlohmann::json& item, const char* key, int fallback)
{
    const auto it = item.find(key);
    if (it == item.end() || !it->is_number_integer())
        return fallback;
    return it->get<int>();
}

bool jsonBool(const nlohmann::json& item, const char* key, bool fallback)
{
    const auto it = item.find(key);
    if (it == item.end() || !it->is_boolean())
        return fallback;
    return it->get<bool>();
}

std::optional<nlohmann::json> loadNdsGameDbRecord(const std::string& romPath)
{
    constexpr const char* dbPaths[] = {
        "/GBAStation/data/GameData_NDS.json",
        "sdmc:/GBAStation/data/GameData_NDS.json",
    };

    const std::string normalizedRomPath = normalizePathForCompare(romPath);

    for (const char* dbPath : dbPaths)
    {
        appendLog("GBAStationNDSStub: try GameDB path=%s", dbPath);
        if (!fileExists(dbPath))
        {
            appendLog("GBAStationNDSStub: GameDB file missing path=%s", dbPath);
            continue;
        }

        try
        {
            std::ifstream file(dbPath, std::ios::binary);
            if (!file.is_open())
            {
                appendLog("GBAStationNDSStub: GameDB open failed path=%s", dbPath);
                continue;
            }

            nlohmann::json data;
            file >> data;
            if (!data.is_array())
            {
                appendLog("GBAStationNDSStub: GameDB root is not array path=%s", dbPath);
                continue;
            }

            appendLog("GBAStationNDSStub: GameDB loaded path=%s count=%zu", dbPath, data.size());
            for (const auto& item : data)
            {
                if (!item.is_object())
                    continue;

                const std::string itemPath = jsonString(item, "path");
                if (itemPath == romPath || normalizePathForCompare(itemPath) == normalizedRomPath)
                {
                    appendLog("GBAStationNDSStub: GameDB match path=%s", itemPath.c_str());
                    return item;
                }
            }

            appendLog("GBAStationNDSStub: GameDB no match romPath=%s normalized=%s",
                romPath.c_str(), normalizedRomPath.c_str());
        }
        catch (const std::exception& e)
        {
            appendLog("GBAStationNDSStub: GameDB exception path=%s error=%s", dbPath, e.what());
        }
        catch (...)
        {
            appendLog("GBAStationNDSStub: GameDB unknown exception path=%s", dbPath);
        }
    }

    return std::nullopt;
}

GameInfo buildGameInfo(const std::string& romPath, const std::optional<nlohmann::json>& record)
{
    GameInfo info;
    info.romPath = romPath;

    if (record.has_value())
    {
        info.title = jsonString(*record, "title");
        info.savePath = jsonString(*record, "savePath");
        info.cheatPath = jsonString(*record, "cheatPath");
        info.internalResolution = jsonInt(*record, "ndsInternalResolution", 1);
    }

    if (info.title.empty())
    {
        const size_t slash = romPath.find_last_of("/\\");
        std::string name = slash == std::string::npos ? romPath : romPath.substr(slash + 1);
        const size_t dot = name.find_last_of('.');
        info.title = dot == std::string::npos ? name : name.substr(0, dot);
    }

    if (info.title.empty())
        info.title = "NDS Game";

    return info;
}

void logGameDbRecord(const nlohmann::json& item)
{
    appendLog("GBAStationNDSStub: gameDb.found=1");
    appendLog("GBAStationNDSStub: gameDb.title=%s", jsonString(item, "title").c_str());
    appendLog("GBAStationNDSStub: gameDb.path=%s", jsonString(item, "path").c_str());
    appendLog("GBAStationNDSStub: gameDb.savePath=%s", jsonString(item, "savePath").c_str());
    appendLog("GBAStationNDSStub: gameDb.cheatPath=%s", jsonString(item, "cheatPath").c_str());
    appendLog("GBAStationNDSStub: gameDb.screenShotPath=%s", jsonString(item, "screenShotPath").c_str());
    appendLog("GBAStationNDSStub: gameDb.ndsInternalResolution=%d", jsonInt(item, "ndsInternalResolution", 1));
    appendLog("GBAStationNDSStub: gameDb.ndsScreenLayout=%s", jsonString(item, "ndsScreenLayout").c_str());
    appendLog("GBAStationNDSStub: gameDb.ndsScreenOrientation=%s", jsonString(item, "ndsScreenOrientation").c_str());
    appendLog("GBAStationNDSStub: gameDb.ndsIntegerScale=%d", jsonBool(item, "ndsIntegerScale", false) ? 1 : 0);
}

uint32_t color(uint8_t r, uint8_t g, uint8_t b)
{
    return RGBA8(r, g, b, 255);
}

uint32_t blendColor(uint32_t dst, uint32_t src, uint8_t alpha)
{
    const uint32_t inv = 255 - alpha;
    const uint32_t sr = src & 0xff;
    const uint32_t sg = (src >> 8) & 0xff;
    const uint32_t sb = (src >> 16) & 0xff;
    const uint32_t dr = dst & 0xff;
    const uint32_t dg = (dst >> 8) & 0xff;
    const uint32_t db = (dst >> 16) & 0xff;
    return RGBA8(
        static_cast<uint8_t>((sr * alpha + dr * inv) / 255),
        static_cast<uint8_t>((sg * alpha + dg * inv) / 255),
        static_cast<uint8_t>((sb * alpha + db * inv) / 255),
        255);
}

void fillRect(UiFrame& frame, Rect rect, uint32_t rgba)
{
    const int x0 = std::max(0, rect.x);
    const int y0 = std::max(0, rect.y);
    const int x1 = std::min(frame.width, rect.x + rect.w);
    const int y1 = std::min(frame.height, rect.y + rect.h);
    for (int y = y0; y < y1; ++y)
    {
        uint32_t* row = frame.pixels + y * frame.stridePixels;
        for (int x = x0; x < x1; ++x)
            row[x] = rgba;
    }
}

void fillRectAlpha(UiFrame& frame, Rect rect, uint32_t rgba, uint8_t alpha)
{
    const int x0 = std::max(0, rect.x);
    const int y0 = std::max(0, rect.y);
    const int x1 = std::min(frame.width, rect.x + rect.w);
    const int y1 = std::min(frame.height, rect.y + rect.h);
    for (int y = y0; y < y1; ++y)
    {
        uint32_t* row = frame.pixels + y * frame.stridePixels;
        for (int x = x0; x < x1; ++x)
            row[x] = blendColor(row[x], rgba, alpha);
    }
}

void drawRect(UiFrame& frame, Rect rect, uint32_t rgba, int thickness = 2)
{
    fillRect(frame, {rect.x, rect.y, rect.w, thickness}, rgba);
    fillRect(frame, {rect.x, rect.y + rect.h - thickness, rect.w, thickness}, rgba);
    fillRect(frame, {rect.x, rect.y, thickness, rect.h}, rgba);
    fillRect(frame, {rect.x + rect.w - thickness, rect.y, thickness, rect.h}, rgba);
}

uint8_t glyphRow(char ch, int row)
{
    if (ch >= 'a' && ch <= 'z')
        ch = static_cast<char>(ch - 'a' + 'A');

    switch (ch)
    {
    case 'A': { constexpr uint8_t g[7] = {14,17,17,31,17,17,17}; return g[row]; }
    case 'B': { constexpr uint8_t g[7] = {30,17,17,30,17,17,30}; return g[row]; }
    case 'C': { constexpr uint8_t g[7] = {14,17,16,16,16,17,14}; return g[row]; }
    case 'D': { constexpr uint8_t g[7] = {30,17,17,17,17,17,30}; return g[row]; }
    case 'E': { constexpr uint8_t g[7] = {31,16,16,30,16,16,31}; return g[row]; }
    case 'F': { constexpr uint8_t g[7] = {31,16,16,30,16,16,16}; return g[row]; }
    case 'G': { constexpr uint8_t g[7] = {14,17,16,23,17,17,15}; return g[row]; }
    case 'H': { constexpr uint8_t g[7] = {17,17,17,31,17,17,17}; return g[row]; }
    case 'I': { constexpr uint8_t g[7] = {14,4,4,4,4,4,14}; return g[row]; }
    case 'J': { constexpr uint8_t g[7] = {7,2,2,2,18,18,12}; return g[row]; }
    case 'K': { constexpr uint8_t g[7] = {17,18,20,24,20,18,17}; return g[row]; }
    case 'L': { constexpr uint8_t g[7] = {16,16,16,16,16,16,31}; return g[row]; }
    case 'M': { constexpr uint8_t g[7] = {17,27,21,21,17,17,17}; return g[row]; }
    case 'N': { constexpr uint8_t g[7] = {17,25,21,19,17,17,17}; return g[row]; }
    case 'O': { constexpr uint8_t g[7] = {14,17,17,17,17,17,14}; return g[row]; }
    case 'P': { constexpr uint8_t g[7] = {30,17,17,30,16,16,16}; return g[row]; }
    case 'Q': { constexpr uint8_t g[7] = {14,17,17,17,21,18,13}; return g[row]; }
    case 'R': { constexpr uint8_t g[7] = {30,17,17,30,20,18,17}; return g[row]; }
    case 'S': { constexpr uint8_t g[7] = {15,16,16,14,1,1,30}; return g[row]; }
    case 'T': { constexpr uint8_t g[7] = {31,4,4,4,4,4,4}; return g[row]; }
    case 'U': { constexpr uint8_t g[7] = {17,17,17,17,17,17,14}; return g[row]; }
    case 'V': { constexpr uint8_t g[7] = {17,17,17,17,17,10,4}; return g[row]; }
    case 'W': { constexpr uint8_t g[7] = {17,17,17,21,21,21,10}; return g[row]; }
    case 'X': { constexpr uint8_t g[7] = {17,17,10,4,10,17,17}; return g[row]; }
    case 'Y': { constexpr uint8_t g[7] = {17,17,10,4,4,4,4}; return g[row]; }
    case 'Z': { constexpr uint8_t g[7] = {31,1,2,4,8,16,31}; return g[row]; }
    case '0': { constexpr uint8_t g[7] = {14,17,19,21,25,17,14}; return g[row]; }
    case '1': { constexpr uint8_t g[7] = {4,12,4,4,4,4,14}; return g[row]; }
    case '2': { constexpr uint8_t g[7] = {14,17,1,2,4,8,31}; return g[row]; }
    case '3': { constexpr uint8_t g[7] = {30,1,1,14,1,1,30}; return g[row]; }
    case '4': { constexpr uint8_t g[7] = {2,6,10,18,31,2,2}; return g[row]; }
    case '5': { constexpr uint8_t g[7] = {31,16,16,30,1,1,30}; return g[row]; }
    case '6': { constexpr uint8_t g[7] = {14,16,16,30,17,17,14}; return g[row]; }
    case '7': { constexpr uint8_t g[7] = {31,1,2,4,8,8,8}; return g[row]; }
    case '8': { constexpr uint8_t g[7] = {14,17,17,14,17,17,14}; return g[row]; }
    case '9': { constexpr uint8_t g[7] = {14,17,17,15,1,1,14}; return g[row]; }
    case ':': { constexpr uint8_t g[7] = {0,4,4,0,4,4,0}; return g[row]; }
    case '-': { constexpr uint8_t g[7] = {0,0,0,31,0,0,0}; return g[row]; }
    case '.': { constexpr uint8_t g[7] = {0,0,0,0,0,12,12}; return g[row]; }
    case '/': { constexpr uint8_t g[7] = {1,1,2,4,8,16,16}; return g[row]; }
    case '[': { constexpr uint8_t g[7] = {14,8,8,8,8,8,14}; return g[row]; }
    case ']': { constexpr uint8_t g[7] = {14,2,2,2,2,2,14}; return g[row]; }
    default: return 0;
    }
}

void drawText(UiFrame& frame, int x, int y, const std::string& text, uint32_t rgba, int scale = 3)
{
    int cursor = x;
    for (char ch : text)
    {
        if (ch == ' ')
        {
            cursor += 4 * scale;
            continue;
        }

        for (int row = 0; row < 7; ++row)
        {
            const uint8_t bits = glyphRow(ch, row);
            for (int col = 0; col < 5; ++col)
            {
                if (bits & (1 << (4 - col)))
                    fillRect(frame, {cursor + col * scale, y + row * scale, scale, scale}, rgba);
            }
        }
        cursor += 6 * scale;
    }
}

void drawGameLayer(UiFrame& frame, const GameInfo& game, int tick, const std::string& status)
{
    fillRect(frame, {0, 0, frame.width, frame.height}, color(10, 13, 18));
    fillRect(frame, {0, 0, frame.width, 74}, color(18, 26, 34));
    drawText(frame, 38, 28, "GBASTATION NDS", color(218, 239, 255), 3);
    drawText(frame, 940, 30, "PLUS MENU", color(128, 180, 210), 2);

    const int screenW = 512;
    const int screenH = 384;
    const int gap = 42;
    const int startX = (frame.width - screenW * 2 - gap) / 2;
    const int screenY = 138;
    const uint32_t topTint = color(31, 92, 126);
    const uint32_t bottomTint = color(45, 82, 63);

    Rect top{startX, screenY, screenW, screenH};
    Rect bottom{startX + screenW + gap, screenY, screenW, screenH};
    fillRect(frame, top, topTint);
    fillRect(frame, bottom, bottomTint);

    for (int i = 0; i < 12; ++i)
    {
        const int stripe = (tick * 2 + i * 56) % (screenW + 120) - 120;
        fillRectAlpha(frame, {top.x + stripe, top.y, 28, top.h}, color(154, 214, 238), 38);
        fillRectAlpha(frame, {bottom.x + screenW - stripe - 28, bottom.y, 28, bottom.h}, color(180, 228, 175), 34);
    }

    drawRect(frame, top, color(102, 180, 219), 4);
    drawRect(frame, bottom, color(116, 186, 126), 4);
    drawText(frame, top.x + 28, top.y + 26, "TOP SCREEN", color(229, 246, 255), 4);
    drawText(frame, bottom.x + 28, bottom.y + 26, "BOTTOM SCREEN", color(232, 255, 230), 4);
    drawText(frame, top.x + 28, top.y + 84, "RENDER PLACEHOLDER", color(174, 218, 239), 2);
    drawText(frame, bottom.x + 28, bottom.y + 84, "INPUT PLACEHOLDER", color(185, 229, 184), 2);

    drawText(frame, 44, 612, "GAME: " + game.title, color(226, 229, 231), 2);
    drawText(frame, 44, 644, "IR: X" + std::to_string(game.internalResolution), color(145, 174, 190), 2);
    if (!status.empty())
        drawText(frame, 440, 644, status, color(248, 211, 120), 2);
}

void drawMenuLayer(UiFrame& frame, int selected, float transition)
{
    if (transition <= 0.01f)
        return;

    const uint8_t dimAlpha = static_cast<uint8_t>(120 * transition);
    fillRectAlpha(frame, {0, 0, frame.width, frame.height}, color(0, 0, 0), dimAlpha);

    const int panelW = 430;
    const int panelX = frame.width - static_cast<int>(panelW * transition);
    fillRect(frame, {panelX, 0, panelW, frame.height}, color(24, 30, 38));
    fillRect(frame, {panelX, 0, 6, frame.height}, color(78, 154, 196));
    drawText(frame, panelX + 36, 44, "NDS MENU", color(236, 247, 255), 4);
    drawText(frame, panelX + 38, 96, "A SELECT  B BACK", color(132, 166, 184), 2);

    for (int i = 0; i < static_cast<int>(std::size(kMenuItems)); ++i)
    {
        const int y = 150 + i * 66;
        const bool active = i == selected;
        fillRect(frame, {panelX + 28, y, panelW - 56, 48}, active ? color(52, 112, 146) : color(31, 39, 49));
        if (active)
            fillRect(frame, {panelX + 28, y, 6, 48}, color(245, 198, 96));
        drawText(frame, panelX + 52, y + 15, kMenuItems[i].label, active ? color(255, 249, 220) : color(205, 218, 225), 2);
    }
}

bool setReturnNro(const std::string& returnNro)
{
    if (returnNro.empty())
    {
        appendLog("GBAStationNDSStub: returnNro empty, cannot exit to main NRO");
        return false;
    }

    if (!envHasNextLoad())
    {
        appendLog("GBAStationNDSStub: envHasNextLoad=false, cannot exit to main NRO");
        return false;
    }

    const std::string args = quoteArg(returnNro);
    const Result rc = envSetNextLoad(returnNro.c_str(), args.c_str());
    appendLog("GBAStationNDSStub: envSetNextLoad exit rc=0x%x path=%s", rc, returnNro.c_str());
    return R_SUCCEEDED(rc);
}

} // namespace

int main(int argc, char* argv[])
{
    appendLog("GBAStationNDSStub: start argc=%d", argc);
    for (int i = 0; i < argc; ++i)
        appendLog("GBAStationNDSStub: argv[%d]=%s", i, argv[i] ? argv[i] : "(null)");

    const char* romPath = "";
    const char* returnNro = "";

    for (int i = 1; i < argc; ++i)
    {
        if (!argv[i])
            continue;

        if (std::strcmp(argv[i], "--return") == 0 && i + 1 < argc)
        {
            returnNro = argv[i + 1];
            ++i;
            continue;
        }

        if (!romPath[0] && !endsWithNoCase(argv[i], ".nro"))
        {
            romPath = argv[i];
            break;
        }
    }

    std::string returnNroPath = returnNro && returnNro[0] ? returnNro : "sdmc:/switch/GBAStation.nro";
    appendLog("GBAStationNDSStub: romPath=%s", romPath && romPath[0] ? romPath : "(empty)");
    appendLog("GBAStationNDSStub: returnNro=%s", returnNroPath.c_str());

    std::optional<nlohmann::json> record;
    if (romPath && romPath[0])
    {
        record = loadNdsGameDbRecord(romPath);
        if (record.has_value())
            logGameDbRecord(*record);
        else
            appendLog("GBAStationNDSStub: gameDb.found=0");
    }

    GameInfo game = buildGameInfo(romPath ? romPath : "", record);
    appendLog("GBAStationNDSStub: ui start title=%s", game.title.c_str());

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    Framebuffer fb {};
    Result rc = framebufferCreate(&fb, nwindowGetDefault(), kScreenWidth, kScreenHeight, PIXEL_FORMAT_RGBA_8888, 2);
    if (R_FAILED(rc))
    {
        appendLog("GBAStationNDSStub: framebufferCreate failed rc=0x%x", rc);
        setReturnNro(returnNroPath);
        return 1;
    }

    rc = framebufferMakeLinear(&fb);
    if (R_FAILED(rc))
    {
        appendLog("GBAStationNDSStub: framebufferMakeLinear failed rc=0x%x", rc);
        framebufferClose(&fb);
        setReturnNro(returnNroPath);
        return 1;
    }

    bool running = true;
    bool pendingReturnToMain = false;
    bool menuVisible = false;
    int selected = 0;
    int tick = 0;
    float menuTransition = 0.0f;
    std::string status = "READY";

    while (appletMainLoop() && running)
    {
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);

        if (down & HidNpadButton_Plus)
            menuVisible = !menuVisible;
        if (down & HidNpadButton_Minus)
            menuVisible = true;

        if (menuVisible)
        {
            if (down & HidNpadButton_AnyUp)
                selected = (selected + static_cast<int>(std::size(kMenuItems)) - 1) % static_cast<int>(std::size(kMenuItems));
            if (down & HidNpadButton_AnyDown)
                selected = (selected + 1) % static_cast<int>(std::size(kMenuItems));
            if (down & HidNpadButton_B)
            {
                menuVisible = false;
                status = "RESUME";
            }
            if (down & HidNpadButton_A)
            {
                const MenuItem& item = kMenuItems[selected];
                appendLog("GBAStationNDSStub: menu action=%s", item.label);
                switch (item.action)
                {
                case MenuAction::Resume:
                    menuVisible = false;
                    status = "RESUME";
                    break;
                case MenuAction::SaveState:
                    status = "SAVE STATE TODO";
                    break;
                case MenuAction::LoadState:
                    status = "LOAD STATE TODO";
                    break;
                case MenuAction::Cheats:
                    status = game.cheatPath.empty() ? "CHEATS TODO" : "CHEATS DB READY";
                    break;
                case MenuAction::Display:
                    status = "DISPLAY TODO";
                    break;
                case MenuAction::Reset:
                    status = "RESET TODO";
                    break;
                case MenuAction::Exit:
                    status = "EXITING";
                    pendingReturnToMain = true;
                    running = false;
                    break;
                }
            }
        }

        const float target = menuVisible ? 1.0f : 0.0f;
        menuTransition += (target - menuTransition) * 0.24f;
        if (!menuVisible && menuTransition < 0.01f)
            menuTransition = 0.0f;
        if (menuVisible && menuTransition > 0.99f)
            menuTransition = 1.0f;

        u32 stride = 0;
        void* frameBuf = framebufferBegin(&fb, &stride);
        if (frameBuf)
        {
            UiFrame frame;
            frame.pixels = static_cast<uint32_t*>(frameBuf);
            frame.stridePixels = static_cast<int>(stride / sizeof(uint32_t));
            drawGameLayer(frame, game, tick, status);
            drawMenuLayer(frame, selected, menuTransition);
        }
        framebufferEnd(&fb);

        ++tick;
        svcSleepThread(16'666'667);
    }

    framebufferClose(&fb);
    if (pendingReturnToMain)
        setReturnNro(returnNroPath);
    appendLog("GBAStationNDSStub: exit");
    return 0;
}
