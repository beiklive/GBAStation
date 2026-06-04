#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

namespace beiklive::flash {

class FlashKeymap {
public:
    static bool initForSwf(const std::string& swfPath);

    static std::string lookup(const std::string& switchButton);

    static bool setBinding(const std::string& switchButton, const std::string& flashKey);
    static bool clearBinding(const std::string& button);

    static bool saveSidecar();

    static const std::unordered_map<std::string, std::string>& getActiveBindings();
    static const std::vector<std::pair<std::string, std::string>>& getFallbackBindings();

    static void reset();

private:
    static std::string m_activeBasename;
    static std::string m_activeSwfPath;
    static std::unordered_map<std::string, std::string> m_activeBindings;

    static const std::vector<std::pair<std::string, std::string>> k_fallback;
    static const std::vector<std::string> k_reserved;

    static std::string sidecarPath(const std::string& swfPath);
    static std::string defaultKeymapPath();
    static void writeDefaultToSd(const std::string& path);
};

} // namespace beiklive::flash
