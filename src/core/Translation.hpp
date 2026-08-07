#pragma once

#include "common.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace beiklive
{

/// 轻量翻译管理器（不依赖 borealis 的 gettext 式 i18n）。
///
/// 加载 resources/lang/<locale>.json（扁平映射：key = 中文原文，
/// value = 译文），提供 tr(中文) 查询。语言由 UI.language 配置决定
/// （"zh-CN" / "en-US"），缺失时回退中文原文。
class TranslationManager
{
public:
    TranslationManager() = default;

    /// 从 SettingManager 的 UI.language 读取语言并加载对应 JSON。
    void Load();

    /// 加载指定语言文件（zh-CN / en-US）；失败时清空并回退原文。
    void Load(const std::string& locale);

    /// 查询中文原文对应的译文；无译文或语言为中文时返回原文。
    std::string Tr(std::string_view zh) const;

    /// 当前语言标识（"zh-CN" / "en-US"），默认 zh-CN。
    const std::string& Locale() const { return locale_; }

    bool IsEnglish() const { return locale_ == "en-US"; }

private:
    std::string locale_ = "zh-CN";
    std::unordered_map<std::string, std::string> table_;
};

extern TranslationManager* Translation;

} // namespace beiklive

/// 翻译宏：传入中文原文，返回译文（英文环境）或原文（中文环境）。
/// 返回 std::string，便于字符串拼接；传给 const char* 参数处需 .c_str()。
#define L(zh) (beiklive::Translation ? beiklive::Translation->Tr(zh) : std::string(zh))
