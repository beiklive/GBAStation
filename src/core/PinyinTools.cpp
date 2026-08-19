#include "core/PinyinTools.hpp"

#include "core/common.h"

#include <nlohmann/json.hpp>

#include <cctype>
#include <fstream>
#include <mutex>
#include <vector>

namespace beiklive
{
namespace pinyin
{

namespace
{

const nlohmann::json& table()
{
    static nlohmann::json s_table = nlohmann::json::object();
    static std::once_flag s_once;
    std::call_once(s_once, []() {
        std::ifstream f(BK_RES("pinyin/pingyin.json"), std::ios::binary);
        if (f.is_open())
        {
            try
            {
                f >> s_table;
            }
            catch (...)
            {
                s_table = nlohmann::json::object();
            }
        }
    });
    return s_table;
}

/* 按 UTF-8 边界拆分文本为字符列表 */
std::vector<std::string> utf8Chars(const std::string& text)
{
    std::vector<std::string> result;
    result.reserve(text.size() / 3 + 1);
    for (size_t i = 0; i < text.size();)
    {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        size_t len = 1;
        if ((c & 0xE0) == 0xC0)
            len = 2;
        else if ((c & 0xF0) == 0xE0)
            len = 3;
        else if ((c & 0xF8) == 0xF0)
            len = 4;
        if (i + len > text.size())
            len = 1;
        result.push_back(text.substr(i, len));
        i += len;
    }
    return result;
}

} // namespace

std::string forChar(const std::string& utf8Char)
{
    const auto& t = table();
    if (!t.is_object())
        return "";
    auto it = t.find(utf8Char);
    if (it == t.end() || !it->is_string())
        return "";
    return it->get<std::string>();
}

bool containsCjk(const std::string& utf8Text)
{
    const auto& t = table();
    if (!t.is_object())
        return false;
    for (const auto& ch : utf8Chars(utf8Text))
    {
        if (ch.size() >= 2 && t.contains(ch))
            return true;
    }
    return false;
}

std::string full(const std::string& utf8Text)
{
    const auto& t = table();
    std::string out;
    for (const auto& ch : utf8Chars(utf8Text))
    {
        if (ch.size() >= 2)
        {
            if (t.is_object())
            {
                auto it = t.find(ch);
                if (it != t.end() && it->is_string())
                {
                    out += it->get<std::string>();
                    continue;
                }
            }
            continue; // 未映射的 CJK/符号忽略
        }
        const unsigned char c = static_cast<unsigned char>(ch[0]);
        if (std::isalnum(c))
            out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

std::string initials(const std::string& utf8Text)
{
    const auto& t = table();
    std::string out;
    for (const auto& ch : utf8Chars(utf8Text))
    {
        if (ch.size() < 2 || !t.is_object())
            continue;
        auto it = t.find(ch);
        if (it == t.end() || !it->is_string())
            continue;
        const std::string py = it->get<std::string>();
        if (!py.empty())
            out.push_back(static_cast<char>(std::tolower(
                static_cast<unsigned char>(py[0]))));
    }
    return out;
}

} // namespace pinyin
} // namespace beiklive
