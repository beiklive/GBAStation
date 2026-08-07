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
    locale_ = (locale == "en-US" || locale == "en" || locale == "zh-Hans" || locale == "zh-CN") ? locale : "zh-CN";
    if (locale_ == "en-US" || locale_ == "en")
        locale_ = "en-US";
    else
        locale_ = "zh-CN";
    table_.clear();
    if (locale_ != "en-US")
        return;

#ifdef __SWITCH__
    const char* path = "romfs:/lang/en-US.json";
#else
    const char* path = "resources/lang/en-US.json";
#endif
    std::ifstream in(path);
    if (!in.is_open())
    {
        in.clear();
        in.open("lang/en-US.json");
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
    if (locale_ != "en-US")
        return std::string(zh);
    auto it = table_.find(std::string(zh));
    if (it != table_.end())
        return it->second;
    return std::string(zh);
}

} // namespace beiklive
