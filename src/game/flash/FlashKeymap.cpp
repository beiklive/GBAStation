#include "FlashKeymap.hpp"
#include "core/enums.h"
#include "core/constexpr.h"
#include "core/json.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace beiklive::flash {

std::string FlashKeymap::m_activeBasename;
std::string FlashKeymap::m_activeSwfPath;
std::unordered_map<std::string, std::string> FlashKeymap::m_activeBindings;

const std::vector<std::pair<std::string, std::string>> FlashKeymap::k_fallback = {
    {"A",            "Space"},
    {"B",            "Z"},
    {"X",            "X"},
    {"Y",            "Shift"},
    {"R",            "Enter"},
    {"Plus",         "P"},
    {"L",            "Escape"},
    {"Left",         "Left"},
    {"Right",        "Right"},
    {"Up",           "Up"},
    {"Down",         "Down"},
    {"StickLUp",     "Up"},
    {"StickLDown",   "Down"},
    {"StickLLeft",   "Left"},
    {"StickLRight",  "Right"},
    {"ZL",           "MouseLeft"},
};

const std::vector<std::string> FlashKeymap::k_reserved = {
    "Minus", "ZR",
};

static std::string s_rootPath()
{
#ifdef __SWITCH__
    static const std::string root = "sdmc:/flashnx";
    return root;
#else
    return ".";
#endif
}

std::string FlashKeymap::sidecarPath(const std::string& swfPath)
{
    std::string basename = fs::path(swfPath).stem().string();
    return s_rootPath() + "/" + basename + ".keymap.json";
}

std::string FlashKeymap::defaultKeymapPath()
{
    return s_rootPath() + "/keymap_default.json";
}

static std::string readTextFile(const std::string& path)
{
    std::error_code ec;
    if (!fs::exists(path, ec))
        return "";
    std::ifstream f(path);
    if (!f)
        return "";
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return content;
}

static void writeTextFile(const std::string& path, const std::string& content)
{
    std::error_code ec;
    fs::create_directories(fs::path(path).parent_path(), ec);
    std::ofstream f(path, std::ios::trunc);
    if (f)
        f << content;
}

static void stripReserved(std::unordered_map<std::string, std::string>& bindings)
{
    for (const auto& btn : FlashKeymap::k_reserved)
        bindings.erase(btn);
}

void FlashKeymap::writeDefaultToSd(const std::string& path)
{
    json j;
    j["version"] = 1;
    json bindings = json::object();
    for (const auto& [btn, key] : k_fallback)
        bindings[btn] = key;
    j["bindings"] = bindings;

    std::string content = j.dump(2);
    writeTextFile(path, content);
}

bool FlashKeymap::initForSwf(const std::string& swfPath)
{
    std::string basename = fs::path(swfPath).stem().string();

    if (m_activeSwfPath == swfPath)
        return true;

    std::string perGamePath = sidecarPath(swfPath);
    std::string defaultPath = defaultKeymapPath();

    m_activeBindings.clear();
    bool loaded = false;

    std::string jsonStr = readTextFile(perGamePath);
    if (!jsonStr.empty()) {
        try {
            json j = json::parse(jsonStr);
            if (j.contains("bindings") && j["bindings"].is_object()) {
                for (auto& [key, val] : j["bindings"].items())
                    m_activeBindings[key] = val.get<std::string>();
                stripReserved(m_activeBindings);
                loaded = true;
            }
        } catch (...) {}
    }

    if (!loaded) {
        jsonStr = readTextFile(defaultPath);
        if (!jsonStr.empty()) {
            try {
                json j = json::parse(jsonStr);
                if (j.contains("bindings") && j["bindings"].is_object()) {
                    for (auto& [key, val] : j["bindings"].items())
                        m_activeBindings[key] = val.get<std::string>();
                    stripReserved(m_activeBindings);
                    loaded = true;
                }
            } catch (...) {}
        }
    }

    if (!loaded) {
        for (const auto& [btn, key] : k_fallback)
            m_activeBindings[btn] = key;
        writeDefaultToSd(defaultPath);
    }

    m_activeBasename = basename;
    m_activeSwfPath = swfPath;
    return true;
}

std::string FlashKeymap::lookup(const std::string& switchButton)
{
    auto it = m_activeBindings.find(switchButton);
    if (it != m_activeBindings.end())
        return it->second;
    return "";
}

bool FlashKeymap::setBinding(const std::string& switchButton, const std::string& flashKey)
{
    if (flashKey == "(none)" || flashKey.empty())
        m_activeBindings.erase(switchButton);
    else
        m_activeBindings[switchButton] = flashKey;
    return saveSidecar();
}

bool FlashKeymap::clearBinding(const std::string& button)
{
    m_activeBindings.erase(button);
    return saveSidecar();
}

bool FlashKeymap::saveSidecar()
{
    if (m_activeSwfPath.empty())
        return false;

    json j;
    j["version"] = 1;
    json bindings = json::object();
    for (const auto& [btn, key] : m_activeBindings)
        bindings[btn] = key;
    j["bindings"] = bindings;

    std::string path = sidecarPath(m_activeSwfPath);
    writeTextFile(path, j.dump(2));
    return true;
}

const std::unordered_map<std::string, std::string>& FlashKeymap::getActiveBindings()
{
    return m_activeBindings;
}

const std::vector<std::pair<std::string, std::string>>& FlashKeymap::getFallbackBindings()
{
    return k_fallback;
}

void FlashKeymap::reset()
{
    m_activeBindings.clear();
    m_activeBasename.clear();
    m_activeSwfPath.clear();
}

} // namespace beiklive::flash
