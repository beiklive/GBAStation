#include "Translation.hpp"
#include "constexpr.h"
#include "json.hpp"

#include <filesystem>
#include <fstream>

namespace beiklive
{

TranslationManager* Translation = nullptr;

void TranslationManager::Load()
{
    std::string locale = "zh-CN";
    if (SettingManager)
    {
        auto val = SettingManager->Get(beiklive::SettingKey::KEY_UI_LANGUAGE);
        if (val)
        {
            auto str = val->AsString();
            if (str && !str->empty())
                locale = *str;
        }
    }
    Load(locale);
}

void TranslationManager::Load(const std::string& locale)
{
    // Normalize common aliases to canonical locale ids.
    if (locale == "zh-Hans" || locale == "zh" || locale == "zh_CN")
        locale_ = "zh-CN";
    else if (locale == "en" || locale == "en_US")
        locale_ = "en-US";
    else if (locale == "ja" || locale == "ja_JP")
        locale_ = "ja-JP";
    else
        locale_ = locale.empty() ? "zh-CN" : locale;

    table_.clear();
    if (locale_ == "zh-CN")
        return;

    const std::string fileName = locale_ + ".json";
#ifdef __SWITCH__
    const std::string path = "romfs:/lang/" + fileName;
#else
    std::string path = "resources/lang/" + fileName;
#endif
    std::ifstream in(path);
    if (!in.is_open())
    {
        in.clear();
        in.open("lang/" + fileName);
    }
    if (!in.is_open())
        return;

    try
    {
        nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
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

std::string TranslationManager::Tr(std::string_view zh) const
{
    if (locale_ == "zh-CN")
        return std::string(zh);
    auto it = table_.find(std::string(zh));
    if (it != table_.end() && !it->second.empty())
        return it->second;
    return std::string(zh);
}

} // namespace beiklive
