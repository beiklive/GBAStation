#include "Video/renderer/GLSLPParser.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <unordered_map>

namespace beiklive {

// ============================================================
// 内部工具函数
// ============================================================

/// 去除字符串首尾空白及可选引号字符。
static std::string trimValue(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n\"");
    if (b == std::string::npos) return {};
    size_t e = s.find_last_not_of(" \t\r\n\"");
    return s.substr(b, e - b + 1);
}

/// 将键名字符串转换为小写。
static std::string toLower(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/// 将 scale_type 字符串转换为 ScaleType 枚举。
static ShaderPassDesc::ScaleType parseScaleType(const std::string& val)
{
    std::string v = toLower(trimValue(val));
    if (v == "viewport") return ShaderPassDesc::ScaleType::Viewport;
    if (v == "absolute") return ShaderPassDesc::ScaleType::Absolute;
    return ShaderPassDesc::ScaleType::Source; // 默认 source
}

// ============================================================
// GLSLPParser::parse
// ============================================================

bool GLSLPParser::parse(const std::string& glslpPath,
                        std::vector<ShaderPassDesc>& outPasses,
                        std::vector<GLSLPTextureDesc>* outTextures)
{
    outPasses.clear();

    std::ifstream f(glslpPath);
    if (!f.is_open()) return false;

    // 获取 .glslp 文件所在目录，用于解析相对路径
    std::filesystem::path baseDir =
        std::filesystem::path(glslpPath).parent_path();

    // ---- 第一遍：读取所有 key=value 对 ----
    std::unordered_map<std::string, std::string> kv;
    {
        std::string line;
        while (std::getline(f, line)) {
            // 去掉行尾 \r
            if (!line.empty() && line.back() == '\r') line.pop_back();
            // 去掉注释
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            // 解析 key = value
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = trimValue(line.substr(0, eq));
            std::string val = trimValue(line.substr(eq + 1));
            if (!key.empty())
                kv[toLower(key)] = val;
        }
    }

    // ---- 读取 shaders 总数 ----
    auto it = kv.find("shaders");
    if (it == kv.end()) return false;
    int numShaders = 0;
    try { numShaders = std::stoi(it->second); } catch (...) { return false; }
    if (numShaders <= 0) return false;

    // ---- 按序解析每个通道 ----
    for (int i = 0; i < numShaders; ++i) {
        std::string idx = std::to_string(i);
        ShaderPassDesc pass;

        // shader 路径（必需）
        auto sit = kv.find("shader" + idx);
        if (sit == kv.end()) continue;

        // 解析为绝对路径
        std::filesystem::path shaderRel(sit->second);
        if (shaderRel.is_absolute()) {
            pass.shaderPath = shaderRel.string();
        } else {
            pass.shaderPath = (baseDir / shaderRel).lexically_normal().string();
        }

        // filter_linear
        {
            auto flt = kv.find("filter_linear" + idx);
            if (flt != kv.end()) {
                std::string v = toLower(flt->second);
                pass.filterLinear = (v == "true" || v == "1" || v == "yes");
            }
        }

        // scale_type（同时设置 X 和 Y）
        {
            auto st = kv.find("scale_type" + idx);
            if (st != kv.end()) {
                pass.scaleTypeX = pass.scaleTypeY = parseScaleType(st->second);
            }
        }
        // scale_type_x / scale_type_y（单独覆盖）
        {
            auto stx = kv.find("scale_type_x" + idx);
            if (stx != kv.end()) pass.scaleTypeX = parseScaleType(stx->second);
            auto sty = kv.find("scale_type_y" + idx);
            if (sty != kv.end()) pass.scaleTypeY = parseScaleType(sty->second);
        }

        // scale（同时设置 X 和 Y）
        {
            auto sc = kv.find("scale" + idx);
            if (sc != kv.end()) {
                try {
                    float v = std::stof(sc->second);
                    pass.scaleX = pass.scaleY = v;
                } catch (...) {}
            }
        }
        // scale_x / scale_y（单独覆盖）
        {
            auto sx = kv.find("scale_x" + idx);
            if (sx != kv.end()) {
                try { pass.scaleX = std::stof(sx->second); } catch (...) {}
            }
            auto sy = kv.find("scale_y" + idx);
            if (sy != kv.end()) {
                try { pass.scaleY = std::stof(sy->second); } catch (...) {}
            }
        }

        // float_framebuffer
        {
            auto ff = kv.find("float_framebuffer" + idx);
            if (ff != kv.end()) {
                std::string v = toLower(ff->second);
                pass.floatFramebuffer = (v == "true" || v == "1");
            }
        }
        // srgb_framebuffer
        {
            auto sf = kv.find("srgb_framebuffer" + idx);
            if (sf != kv.end()) {
                std::string v = toLower(sf->second);
                pass.srgbFramebuffer = (v == "true" || v == "1");
            }
        }

        // alias（通道别名，供后续通道以 <alias>Texture 形式引用本通道输出）
        {
            auto al = kv.find("alias" + idx);
            if (al != kv.end()) {
                pass.alias = al->second;
            }
        }

        outPasses.push_back(std::move(pass));
    }

    // ---- 解析外部纹理声明（textures = NAME1;NAME2;...）----
    if (outTextures) {
        outTextures->clear();
        auto tit = kv.find("textures");
        if (tit != kv.end()) {
            std::istringstream ss(tit->second);
            std::string texName;
            while (std::getline(ss, texName, ';')) {
                texName = trimValue(texName);
                if (texName.empty()) continue;

                GLSLPTextureDesc td;
                td.name = texName;

                // 路径（键名为纹理名称的原始形式，已在 kv 中以小写存储）
                auto pathIt = kv.find(toLower(texName));
                if (pathIt != kv.end() && !pathIt->second.empty()) {
                    std::filesystem::path texRel(pathIt->second);
                    if (texRel.is_absolute()) {
                        td.path = texRel.string();
                    } else {
                        td.path = (baseDir / texRel).lexically_normal().string();
                    }
                }

                // 线性过滤标志（键名 = <texname>_linear，小写查找）
                auto linIt = kv.find(toLower(texName) + "_linear");
                if (linIt != kv.end()) {
                    std::string v = toLower(linIt->second);
                    td.filterLinear = (v == "true" || v == "1" || v == "yes");
                }

                if (!td.path.empty()) {
                    outTextures->push_back(std::move(td));
                }
            }
        }
    }

    return !outPasses.empty();
}

} // namespace beiklive
