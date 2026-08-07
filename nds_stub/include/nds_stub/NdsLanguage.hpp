#pragma once

#include <fstream>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace beiklive
{
namespace nds_stub
{

/// Lightweight zh/en/ja translation for the NDS stub menus.
/// Language comes from the launcher's UI.language (config.cfg).
/// Tables are loaded from romfs:/lang/<locale>.json (zh-CN / en-US / ja-JP).
class NdsLanguage
{
public:
    static NdsLanguage& Instance()
    {
        static NdsLanguage instance;
        return instance;
    }

    bool IsEnglish()
    {
        EnsureLoaded();
        return locale_ == "en-US";
    }

    bool IsJapanese()
    {
        EnsureLoaded();
        return locale_ == "ja-JP";
    }

    /// Translate a Chinese UI string; returns the input unchanged for zh or
    /// missing keys. The returned pointer stays valid until the next call.
    const char* Tr(const char* zh)
    {
        EnsureLoaded();
        if (locale_ == "zh-CN" || !zh)
            return zh;
        auto it = table_.find(zh);
        if (it != table_.end() && !it->second.empty())
            return it->second.c_str();
        return zh;
    }

private:
    NdsLanguage() = default;

    void EnsureLoaded()
    {
        if (loaded_)
            return;
        loaded_ = true;

        std::string language = "zh-CN";
        const char* paths[] = {
            "sdmc:/GBAStation/config/config.cfg",
            "/GBAStation/config/config.cfg",
        };
        for (const char* path : paths)
        {
            std::ifstream file(path);
            if (!file)
                continue;
            std::string line;
            while (std::getline(file, line))
            {
                const size_t eq = line.find('=');
                if (eq == std::string::npos)
                    continue;
                const std::string key = line.substr(0, eq);
                if (key != "UI.language")
                    continue;
                std::string value = line.substr(eq + 1);
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                    value = value.substr(1, value.size() - 2);
                // Launcher config.cfg stores strings with an "s|" type prefix.
                if (value.size() >= 2 && value[0] == 's' && value[1] == '|')
                    value = value.substr(2);
                language = value;
                break;
            }
            break;
        }

        if (language == "en-US" || language == "en" || language == "English")
            locale_ = "en-US";
        else if (language == "ja-JP" || language == "ja" || language == "Japanese")
            locale_ = "ja-JP";
        else
            locale_ = "zh-CN";
        if (locale_ == "zh-CN")
            return;

        const std::string langPath = "romfs:/lang/" + locale_ + ".json";
        std::ifstream file(langPath);
        if (!file.is_open())
            return;

        try
        {
            nlohmann::json j = nlohmann::json::parse(file, nullptr, false);
            if (j.is_discarded() || !j.is_object())
                return;
            for (auto it = j.begin(); it != j.end(); ++it)
            {
                if (it.value().is_string())
                    table_[it.key()] = it.value().get<std::string>();
            }
        }
        catch (...)
        {
        }
    }

    bool loaded_ = false;
    std::string locale_ = "zh-CN";
    std::unordered_map<std::string, std::string> table_;
};

} // namespace nds_stub
} // namespace beiklive

/// Global shorthand for NDS stub translations.
#define NDS_L(zh) beiklive::nds_stub::NdsLanguage::Instance().Tr(zh)
