#include "game/render/GLSLPParser.hpp"

#include <borealis.hpp>
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
    return ShaderPassDesc::ScaleType::Source;
}

/// 将 wrap_mode 字符串转换为 WrapMode 枚举。
static ShaderPassDesc::WrapMode parseWrapMode(const std::string& val)
{
    std::string v = toLower(trimValue(val));
    if (v == "clamp_to_border") return ShaderPassDesc::WrapMode::ClampToBorder;
    if (v == "repeat")          return ShaderPassDesc::WrapMode::Repeat;
    if (v == "mirrored_repeat") return ShaderPassDesc::WrapMode::MirroredRepeat;
    if (v != "clamp_to_edge" && !v.empty()) {
        brls::Logger::warning("GLSLPParser: 未识别的 wrap_mode 值 \"{}\"，使用默认 clamp_to_edge", v);
    }
    return ShaderPassDesc::WrapMode::ClampToEdge;
}

// ============================================================
// GLSLPParser::parse
// ============================================================

bool GLSLPParser::parse(const std::string& glslpPath,
                        std::vector<ShaderPassDesc>& outPasses,
                        std::vector<GLSLPTextureDesc>* outTextures,
                        std::vector<GLSLPParamOverride>* outParams,
                        GLSLPPresetMeta*             outMeta)
{
    outPasses.clear();
    if (outMeta) { outMeta->feedbackPass = -1; outMeta->historySize = 0; }

    // 获取 .glslp 文件所在目录，用于解析相对路径
    std::filesystem::path baseDir =
        std::filesystem::path(glslpPath).parent_path();

    // ---- 检查 #reference 指令（简单预设引用链） ----
    {
        std::ifstream refFile(glslpPath);
        if (refFile.is_open()) {
            std::string refLine;
            while (std::getline(refFile, refLine)) {
                if (!refLine.empty() && refLine.back() == '\r') refLine.pop_back();
                size_t ns = refLine.find_first_not_of(" \t");
                if (ns == std::string::npos) continue;
                std::string trimRef = refLine.substr(ns);
                if (trimRef.empty() || trimRef[0] == '#') {
                    if (trimRef.size() > 10 && trimRef.substr(1, 9) == "reference") {
                        std::string after = trimRef.substr(10);
                        size_t q1 = after.find('"');
                        size_t q2 = (q1 != std::string::npos) ? after.find('"', q1 + 1) : std::string::npos;
                        if (q1 != std::string::npos && q2 != std::string::npos) {
                            std::string refPath = after.substr(q1 + 1, q2 - q1 - 1);
                            std::filesystem::path resolvedRef;
                            if (std::filesystem::path(refPath).is_absolute()) {
                                resolvedRef = refPath;
                            } else {
                                resolvedRef = baseDir / refPath;
                            }
                            resolvedRef = resolvedRef.lexically_normal();

                            brls::Logger::debug("GLSLPParser: 跟随 #reference -> {}", resolvedRef.string());

                            std::vector<ShaderPassDesc>    refPasses;
                            std::vector<GLSLPTextureDesc>  refTextures;
                            std::vector<GLSLPParamOverride> refParams;

                            bool ok = parse(resolvedRef.string(), refPasses,
                                            outTextures ? &refTextures : nullptr,
                                            outParams    ? &refParams    : nullptr,
                                            outMeta);
                            if (!ok) {
                                brls::Logger::error("GLSLPParser: #reference 解析失败: {}", resolvedRef.string());
                                return false;
                            }

                            outPasses = std::move(refPasses);
                            if (outTextures) *outTextures = std::move(refTextures);
                            if (outParams)   *outParams   = std::move(refParams);

                            // 当前文件的 key=value 对用于覆盖引用链中的值（参数/纹理）
                            goto apply_overrides;
                        }
                    }
                    continue;
                }
                break;
            }
        }
    }

apply_overrides:
    // ---- 重新打开当前文件读取 key=value 对 ----
    std::unordered_map<std::string, std::string> kv;
    {
        std::ifstream f2(glslpPath);
        if (!f2.is_open() && outPasses.empty()) return false;
        if (f2.is_open())
        {
            std::string line;
            while (std::getline(f2, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                auto hash = line.find('#');
                if (hash != std::string::npos) line = line.substr(0, hash);
                auto eq = line.find('=');
                if (eq == std::string::npos) continue;
                std::string key = trimValue(line.substr(0, eq));
                std::string val = trimValue(line.substr(eq + 1));
                if (!key.empty())
                    kv[toLower(key)] = val;
            }
        }
    }

    // ---- 读取 shaders 总数 ----
    auto it = kv.find("shaders");
    bool hasShaders = (it != kv.end());
    int  numShaders  = 0;

    if (hasShaders) {
        try { numShaders = std::stoi(it->second); } catch (...) { return false; }
        if (numShaders <= 0) return false;

        // 如果已有来自 #reference 的 passes，当前文件的 shaders 声明会替换它们
        if (!outPasses.empty()) {
            brls::Logger::debug("GLSLPParser: 当前文件覆盖 #reference 的 {} 个通道", outPasses.size());
            outPasses.clear();
        }
    } else {
        // 无 shaders 键：若 passes 为空（无 #reference），则失败
        if (outPasses.empty()) return false;
    }

    // ---- 按序解析每个通道（仅在当前文件声明了 shaders 时）----
    if (hasShaders) {
        for (int i = 0; i < numShaders; ++i) {
            std::string idx = std::to_string(i);
            ShaderPassDesc pass;

            auto sit = kv.find("shader" + idx);
            if (sit == kv.end()) continue;

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

            // wrap_mode
            {
                auto wm = kv.find("wrap_mode" + idx);
                if (wm != kv.end()) {
                    pass.wrapMode = parseWrapMode(wm->second);
                }
            }

            // scale_type（同时设置 X 和 Y）
            {
                auto st = kv.find("scale_type" + idx);
                if (st != kv.end()) {
                    pass.scaleTypeX = pass.scaleTypeY = parseScaleType(st->second);
                    pass.hasExplicitScale = true;
                }
            }
            // scale_type_x / scale_type_y（单独覆盖）
            {
                auto stx = kv.find("scale_type_x" + idx);
                if (stx != kv.end()) {
                    pass.scaleTypeX = parseScaleType(stx->second);
                    pass.hasExplicitScale = true;
                }
                auto sty = kv.find("scale_type_y" + idx);
                if (sty != kv.end()) {
                    pass.scaleTypeY = parseScaleType(sty->second);
                    pass.hasExplicitScale = true;
                }
            }

            // scale（同时设置 X 和 Y）
            {
                auto sc = kv.find("scale" + idx);
                if (sc != kv.end()) {
                    try {
                        float v = std::stof(sc->second);
                        pass.scaleX = pass.scaleY = v;
                        pass.hasExplicitScale = true;
                    } catch (...) {}
                }
            }
            // scale_x / scale_y（单独覆盖）
            {
                auto sx = kv.find("scale_x" + idx);
                if (sx != kv.end()) {
                    try {
                        pass.scaleX = std::stof(sx->second);
                        pass.hasExplicitScale = true;
                    } catch (...) {}
                }
                auto sy = kv.find("scale_y" + idx);
                if (sy != kv.end()) {
                    try {
                        pass.scaleY = std::stof(sy->second);
                        pass.hasExplicitScale = true;
                    } catch (...) {}
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

            // alias
            {
                auto al = kv.find("alias" + idx);
                if (al != kv.end()) {
                    pass.alias = al->second;
                }
            }

            // frame_count_mod
            {
                auto fcm = kv.find("frame_count_mod" + idx);
                if (fcm != kv.end()) {
                    try { pass.frameCountMod = std::stoi(fcm->second); } catch (...) {}
                }
            }

            // mipmap_input
            {
                auto mi = kv.find("mipmap_input" + idx);
                if (mi != kv.end()) {
                    std::string v = toLower(mi->second);
                    pass.mipmapInput = (v == "true" || v == "1");
                }
            }

            // feedback (per-pass)
            {
                auto fb = kv.find("feedback" + idx);
                if (fb != kv.end()) {
                    std::string v = toLower(fb->second);
                    pass.feedback = (v == "true" || v == "1");
                }
            }

            outPasses.push_back(std::move(pass));
        }
    }

    // ---- 最后一个通道的缩放默认值处理 ----
    // 按 RetroArch 规范：最后通道无显式缩放时默认为 viewport×1.0
    if (!outPasses.empty() && !outPasses.back().hasExplicitScale) {
        auto& last = outPasses.back();
        last.scaleTypeX = ShaderPassDesc::ScaleType::Viewport;
        last.scaleTypeY = ShaderPassDesc::ScaleType::Viewport;
        last.scaleX = 1.0f;
        last.scaleY = 1.0f;
        brls::Logger::debug("GLSLPParser: 共 {} 个通道，最后通道无显式缩放，默认设置为 viewport×1.0",
                             outPasses.size());
    }

    // ---- 解析外部纹理声明 ----
    if (outTextures) {
        auto tit = kv.find("textures");
        if (tit != kv.end()) {
            // 对已有纹理去重（#reference 可能已加载同名纹理）
            std::unordered_map<std::string, size_t> texIndex;
            for (size_t i = 0; i < outTextures->size(); ++i)
                texIndex[toLower((*outTextures)[i].name)] = i;

            std::istringstream ss(tit->second);
            std::string texName;
            while (std::getline(ss, texName, ';')) {
                texName = trimValue(texName);
                if (texName.empty()) continue;

                GLSLPTextureDesc td;
                td.name = texName;

                auto pathIt = kv.find(toLower(texName));
                if (pathIt != kv.end() && !pathIt->second.empty()) {
                    std::filesystem::path texRel(pathIt->second);
                    if (texRel.is_absolute()) {
                        td.path = texRel.string();
                    } else {
                        td.path = (baseDir / texRel).lexically_normal().string();
                    }
                }

                auto linIt = kv.find(toLower(texName) + "_linear");
                if (linIt != kv.end()) {
                    std::string v = toLower(linIt->second);
                    td.filterLinear = (v == "true" || v == "1" || v == "yes");
                }

                auto wrapIt = kv.find(toLower(texName) + "_wrap_mode");
                if (wrapIt != kv.end()) {
                    td.wrapMode = parseWrapMode(wrapIt->second);
                }

                if (!td.path.empty()) {
                    auto dup = texIndex.find(toLower(texName));
                    if (dup != texIndex.end()) {
                        (*outTextures)[dup->second] = std::move(td);
                    } else {
                        outTextures->push_back(std::move(td));
                    }
                }
            }
        }
    }

    // ---- 解析参数默认值覆盖 ----
    if (outParams) {
        auto pit = kv.find("parameters");
        if (pit != kv.end()) {
            // 对已有参数去重覆盖
            std::unordered_map<std::string, size_t> paramIndex;
            for (size_t i = 0; i < outParams->size(); ++i)
                paramIndex[toLower((*outParams)[i].name)] = i;

            std::istringstream ss(pit->second);
            std::string paramName;
            while (std::getline(ss, paramName, ';')) {
                paramName = trimValue(paramName);
                if (paramName.empty()) continue;

                auto valIt = kv.find(toLower(paramName));
                if (valIt != kv.end() && !valIt->second.empty()) {
                    float val = 0.0f;
                    try { val = std::stof(valIt->second); } catch (...) { continue; }

                    auto dup = paramIndex.find(toLower(paramName));
                    if (dup != paramIndex.end()) {
                        (*outParams)[dup->second].value = val;
                    } else {
                        outParams->push_back({ paramName, val });
                    }
                }
            }
        }
    }

    // ---- 解析预设级元数据 ----
    if (outMeta) {
        auto fp = kv.find("feedback_pass");
        if (fp != kv.end()) {
            try { outMeta->feedbackPass = std::stoi(fp->second); } catch (...) {}
        }
        auto hs = kv.find("history_size");
        if (hs != kv.end()) {
            try { outMeta->historySize = std::stoi(hs->second); } catch (...) {}
        }
    }

    return !outPasses.empty();
}

// ============================================================
// GLSLPParser::parseParamMeta
// 从 .glsl 着色器源文件中解析 #pragma parameter 指令
// ============================================================

void GLSLPParser::parseParamMeta(const std::string& shaderPath,
                                  std::vector<ShaderParamInfo>& outMeta)
{
    outMeta.clear();
    std::ifstream f(shaderPath);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos) continue;
        std::string trimmed = line.substr(s);

        if (trimmed.size() < 17 || trimmed.substr(0, 17) != "#pragma parameter") continue;
        std::string rest = trimmed.substr(17);

        // 解析 NAME
        std::istringstream ss(rest);
        std::string name;
        ss >> name;
        if (name.empty()) continue;

        // 解析引号包裹的显示名称
        std::string desc;
        {
            size_t q1 = rest.find('"');
            size_t q2 = (q1 != std::string::npos) ? rest.find('"', q1 + 1) : std::string::npos;
            if (q1 != std::string::npos && q2 != std::string::npos)
                desc = rest.substr(q1 + 1, q2 - q1 - 1);
            else
                desc = name;
        }

        // 在显示名称后解析四个浮点数：DEFAULT MIN MAX STEP
        std::string afterQuote;
        {
            size_t q2 = rest.rfind('"');
            if (q2 != std::string::npos && q2 + 1 < rest.size())
                afterQuote = rest.substr(q2 + 1);
            else
                afterQuote = rest;
        }
        std::istringstream nums(afterQuote);
        float defVal = 0.f, minVal = 0.f, maxVal = 1.f, stepVal = 0.f;
        nums >> defVal >> minVal >> maxVal >> stepVal;

        // 去重：若同名参数已存在则覆盖
        bool found = false;
        for (auto& p : outMeta) {
            if (p.name == name) {
                p = { name, desc, defVal, minVal, maxVal, stepVal, defVal };
                found = true;
                break;
            }
        }
        if (!found)
            outMeta.push_back({ name, desc, defVal, minVal, maxVal, stepVal, defVal });
    }
}

} // namespace beiklive
