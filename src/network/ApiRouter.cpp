#include "ApiRouter.h"

#include "core/ThreeDsTitlePaths.hpp"
#include "core/Tools.hpp"
#include "core/rom/PspMeta.hpp"
#include "core/common.h"
#include "core/game_database.hpp"

#include "miniz.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace beiklive::network
{

namespace
{
constexpr std::uint64_t MAX_UPLOAD_SIZE = 4ull * 1024ull * 1024ull * 1024ull;
constexpr std::uint64_t MAX_TEXT_EDIT_SIZE = 2ull * 1024ull * 1024ull;
constexpr std::size_t MAX_ROM_STEM_SIZE = 40;
constexpr const char* JSON_HEADERS = "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n";
constexpr const char* CORS_HEADERS = "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\n";
constexpr const char* ALBUM_ROOT = "sdmc:/emuMMC/SD00/Nintendo/Album";

std::string strFromMg(mg_str s)
{
    return std::string(s.buf ? s.buf : "", s.len);
}

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trimExtDot(std::string ext)
{
    if (!ext.empty() && ext[0] == '.')
        ext.erase(ext.begin());
    return toLower(ext);
}

int platformFromExt(const std::string& ext)
{
    if (ext == "gba") return static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA);
    if (ext == "gbc") return static_cast<int>(beiklive::enums::EmuPlatform::EmuGBC);
    if (ext == "gb") return static_cast<int>(beiklive::enums::EmuPlatform::EmuGB);
    if (ext == "nes" || ext == "fds") return static_cast<int>(beiklive::enums::EmuPlatform::EmuNES);
    if (ext == "sfc" || ext == "smc") return static_cast<int>(beiklive::enums::EmuPlatform::EmuSNES);
    if (ext == "nds") return static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS);
    if (ext == "cia" || ext == "cci" || ext == "3ds")
        return static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS);
    if (ext == "md" || ext == "gen" || ext == "bin" || ext == "smd")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuGenesis);
    if (ext == "zip" || ext == "7z")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuArcade);
    if (ext == "cdi" || ext == "gdi" || ext == "chd" || ext == "cue")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuDreamcast);
    if (ext == "iso" || ext == "cso")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP);
    if (ext == "img" || ext == "ecm" || ext == "mds" || ext == "m3u")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuPS1);
    if (ext == "ccd")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuSaturn);
    if (ext == "gcm" || ext == "rvz" || ext == "wbfs" || ext == "wad" ||
        ext == "ciso" || ext == "tgc" || ext == "gcz" || ext == "wia" ||
        ext == "nfs" || ext == "dol" || ext == "elf")
        return static_cast<int>(beiklive::enums::EmuPlatform::EmuDolphin);
    return 0;
}

std::string platformDir(int platform)
{
    std::string name = beiklive::tools::platformBadgeName(platform);
    return name.empty() ? "OTHER" : name;
}

std::string decodeQuery(mg_http_message* hm, const char* name)
{
    char buf[2048] = {};
    int n = mg_http_get_var(&hm->query, name, buf, sizeof(buf));
    return n > 0 ? std::string(buf, static_cast<size_t>(n)) : "";
}

std::string jsonString(mg_http_message* hm, const char* path, const std::string& fallback = "")
{
    char* value = mg_json_get_str(hm->body, path);
    if (!value)
        return fallback;
    std::string out(value);
    free(value);
    return out;
}

long jsonLong(mg_http_message* hm, const char* path, long fallback = 0)
{
    return mg_json_get_long(hm->body, path, fallback);
}

bool jsonBool(mg_http_message* hm, const char* path, bool fallback = false)
{
    bool value = fallback;
    if (mg_json_get_bool(hm->body, path, &value))
        return value;
    return fallback;
}

std::string uriDecode(const std::string& value)
{
    std::string out(value.size() + 1, '\0');
    int n = mg_url_decode(value.c_str(), value.size(), out.data(), out.size(), 0);
    if (n < 0)
        return value;
    out.resize(static_cast<size_t>(n));
    return out;
}

bool containsNonAscii(const std::string& value)
{
    return std::any_of(value.begin(), value.end(), [](unsigned char c) { return c >= 0x80; });
}

std::vector<std::string> utf8Chars(const std::string& input);

bool containsChineseChar(const std::string& value)
{
    for (const auto& ch : utf8Chars(value))
    {
        if (ch.size() != 3)
            continue;
        const unsigned char b0 = static_cast<unsigned char>(ch[0]);
        const unsigned char b1 = static_cast<unsigned char>(ch[1]);
        const unsigned char b2 = static_cast<unsigned char>(ch[2]);
        const uint32_t cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        if ((cp >= 0x3400 && cp <= 0x4DBF) || (cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0xF900 && cp <= 0xFAFF))
            return true;
    }
    return false;
}

std::string safePathComponent(const std::string& name, bool fallbackFile);
fs::path uniqueFileTarget(const fs::path& requested);
std::string encodedPathParam(const fs::path& path);

bool isSafeChar(unsigned char c)
{
    return std::isalnum(c) || c == '-' || c == '_';
}

std::string loadTextFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string pinyinForUtf8(const std::string& ch)
{
    static nlohmann::json table;
    static bool loaded = false;
    if (!loaded)
    {
        loaded = true;
        try
        {
            std::string data = loadTextFile(beiklive::res_path("pinyin/pingyin.json"));
            if (!data.empty())
                table = nlohmann::json::parse(data);
        }
        catch (...)
        {
            table = nlohmann::json::object();
        }
    }

    if (table.is_object() && table.contains(ch) && table[ch].is_string())
        return table[ch].get<std::string>();
    return "";
}

std::vector<std::string> utf8Chars(const std::string& input)
{
    std::vector<std::string> result;
    for (size_t i = 0; i < input.size();)
    {
        unsigned char c = static_cast<unsigned char>(input[i]);
        size_t len = 1;
        if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        if (i + len > input.size())
            len = 1;
        result.push_back(input.substr(i, len));
        i += len;
    }
    return result;
}

std::string safeStemFromTitle(const std::string& stem)
{
    std::string out;
    bool lastSep = false;
    for (const auto& ch : utf8Chars(stem))
    {
        unsigned char c = static_cast<unsigned char>(ch[0]);
        if (ch.size() == 1 && isSafeChar(c))
        {
            out.push_back(static_cast<char>(std::tolower(c)));
            lastSep = false;
            continue;
        }

        if (ch.size() == 1 && (c == ' ' || c == '.' || c == '(' || c == ')' || c == '[' || c == ']'))
        {
            if (!lastSep && !out.empty())
            {
                out.push_back('_');
                lastSep = true;
            }
            continue;
        }

        std::string part = pinyinForUtf8(ch);
        if (part.empty())
            continue;

        if (!lastSep && !out.empty())
            out.push_back('_');
        for (unsigned char pc : part)
        {
            if (isSafeChar(pc))
                out.push_back(static_cast<char>(std::tolower(pc)));
        }
        out.push_back('_');
        lastSep = true;
    }

    while (!out.empty() && out.back() == '_')
        out.pop_back();
    while (!out.empty() && out.front() == '_')
        out.erase(out.begin());
    return out.empty() ? "game" : out;
}

std::string shortStableHash(const std::string& value)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned char c : value)
    {
        hash ^= c;
        hash *= 1099511628211ull;
    }

    static constexpr char HEX[] = "0123456789abcdef";
    std::string result(8, '0');
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[result.size() - 1 - i] = HEX[hash & 0x0F];
        hash >>= 4;
    }
    return result;
}

std::string safeRomStemFromTitle(const std::string& stem)
{
    std::string safe = safeStemFromTitle(stem);
    if (safe.size() <= MAX_ROM_STEM_SIZE)
        return safe;

    const std::string hash = shortStableHash(stem);
    const size_t prefixSize = MAX_ROM_STEM_SIZE - hash.size() - 1;
    safe.resize(prefixSize);
    while (!safe.empty() && safe.back() == '_')
        safe.pop_back();
    if (safe.empty())
        safe = "game";
    return safe + "_" + hash;
}

std::string safePathComponent(const std::string& name, bool fallbackFile)
{
    fs::path path(name);
    std::string stem = path.stem().string();
    std::string ext = trimExtDot(path.extension().string());
    if (stem.empty())
        stem = path.filename().string();
    std::string safe = safeStemFromTitle(stem);
    if (safe == "game" && !fallbackFile)
        safe = "folder";
    if (!fallbackFile || ext.empty())
        return safe;
    return safe + "." + ext;
}

std::string safeUploadRelativePath(const std::string& relativePath, const std::string& fileName, bool& renamed)
{
    fs::path input(relativePath.empty() ? fileName : relativePath);
    std::vector<std::string> parts;
    for (const auto& part : input)
    {
        std::string raw = part.string();
        if (raw.empty() || raw == "." || raw == "..")
            continue;
        parts.push_back(raw);
    }
    if (parts.empty())
        parts.push_back(fileName.empty() ? "upload.bin" : fileName);

    fs::path safe;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        bool isFile = i + 1 == parts.size();
        std::string clean = containsNonAscii(parts[i]) ? safePathComponent(parts[i], isFile) : parts[i];
        if (clean.empty() || clean == "." || clean == "..")
            clean = isFile ? "upload.bin" : "folder";
        if (clean != parts[i])
            renamed = true;
        safe /= clean;
    }
    return safe.string();
}

std::string uniquePath(const fs::path& dir, const std::string& stem, const std::string& ext)
{
    fs::path path = dir / (stem + "." + ext);
    int i = 1;
    while (fs::exists(path))
        path = dir / (stem + "_" + std::to_string(i++) + "." + ext);
    return path.string();
}

fs::path uniqueFileTarget(const fs::path& requested)
{
    if (!fs::exists(requested))
        return requested;

    fs::path dir = requested.parent_path();
    std::string stem = requested.stem().string();
    std::string ext = requested.extension().string();
    if (stem.empty())
        stem = requested.filename().string();
    int i = 1;
    fs::path path;
    do
    {
        path = dir / (stem + "_" + std::to_string(i++) + ext);
    } while (fs::exists(path));
    return path;
}

std::string gameIdFromEntry(const beiklive::GameEntry& entry)
{
    if (entry.crc32 != 0)
        return std::to_string(entry.crc32);
    return entry.path;
}

std::optional<beiklive::GameEntry> findGame(const std::string& id)
{
    if (!beiklive::GameDB)
        return std::nullopt;
    try
    {
        int crc = std::stoi(id);
        auto byCrc = beiklive::GameDB->findByCrc32(crc);
        if (byCrc)
            return byCrc;
    }
    catch (...)
    {
    }
    return beiklive::GameDB->findByPath(uriDecode(id));
}

bool saveGame(beiklive::GameEntry& entry)
{
    if (!beiklive::GameDB)
        return false;
    beiklive::tools::tryUseNdsInternalIconCover(entry);
    beiklive::GameDB->upsertByPath(entry);
    return beiklive::GameDB->flush();
}

nlohmann::json parseJsonBody(mg_http_message* hm)
{
    if (!hm)
        return nlohmann::json::object();
    auto parsed = nlohmann::json::parse(strFromMg(hm->body), nullptr, false);
    return parsed.is_discarded() ? nlohmann::json::object() : parsed;
}

bool jsonBoolValue(const nlohmann::json& body, const char* key, bool fallback = false)
{
    if (!body.is_object() || !body.contains(key))
        return fallback;

    const auto& value = body[key];
    if (value.is_boolean())
        return value.get<bool>();
    if (value.is_number_integer())
        return value.get<long long>() != 0;
    if (value.is_number_unsigned())
        return value.get<unsigned long long>() != 0;
    if (value.is_number_float())
        return value.get<double>() != 0.0;
    if (value.is_string())
    {
        std::string text = toLower(value.get<std::string>());
        return text == "true" || text == "1" || text == "yes" || text == "on";
    }
    return fallback;
}

nlohmann::json gameJsonWithMeta(const beiklive::GameEntry& entry)
{
    nlohmann::json item;
    beiklive::to_json(item, entry);
    item["id"] = gameIdFromEntry(entry);
    item["platformName"] = beiklive::tools::platformBadgeName(entry.platform);
    return item;
}

bool isRemoteEditableGameField(const std::string& key)
{
    static const std::set<std::string> editable = {
        "title", "playCount", "playTime", "lastPlayed", "favourite",
        "savePath", "screenShotPath", "logoPath", "cheatPath", "overlayPath", "shaderPath",
        "overlayEnabled", "shaderEnabled",
        "displayMode", "integerAspectRatio", "customScale", "customOffsetX", "customOffsetY",
        "ndsTopScale", "ndsTopOffsetX", "ndsTopOffsetY",
        "ndsBottomScale", "ndsBottomOffsetX", "ndsBottomOffsetY", "ndsBottomOpacity",
        "ndsScreenLayout", "ndsScreenOrientation", "ndsIntegerScale", "ndsScreenGap", "ndsInternalResolution",
        "shaderParaPath", "shaderParaNames", "shaderParaValues",
    };
    return editable.count(key) > 0;
}

void replyJson(mg_connection* c, int status, const nlohmann::json& body)
{
    std::string data = body.dump();
    std::string headers = std::string(JSON_HEADERS) + CORS_HEADERS;
    mg_http_reply(c, status, headers.c_str(), "%s", data.c_str());
}

void replyError(mg_connection* c, int status, const std::string& message)
{
    replyJson(c, status, {{"ok", false}, {"error", message}});
}

void ensureDir(const std::string& path)
{
    std::error_code ec;
    fs::create_directories(path, ec);
}

bool isTextEditableExt(const std::string& ext)
{
    static const std::set<std::string> editable = {
        "txt", "log", "cfg", "ini", "json", "xml", "md", "cht", "savtxt", "yaml", "yml", "csv",
    };
    return editable.count(toLower(ext)) > 0;
}

std::string mimeTypeForExt(const std::string& ext)
{
    std::string e = toLower(ext);
    if (e == "png") return "image/png";
    if (e == "jpg" || e == "jpeg") return "image/jpeg";
    if (e == "webp") return "image/webp";
    if (e == "gif") return "image/gif";
    if (e == "bmp") return "image/bmp";
    if (e == "mp4" || e == "m4v") return "video/mp4";
    if (e == "mov") return "video/quicktime";
    if (e == "webm") return "video/webm";
    if (e == "mkv") return "video/x-matroska";
    if (e == "avi") return "video/x-msvideo";
    if (isTextEditableExt(e)) return "text/plain; charset=utf-8";
    if (e == "zip") return "application/zip";
    return "application/octet-stream";
}

bool isAlbumImageExt(const std::string& ext)
{
    std::string e = toLower(ext);
    return e == "png" || e == "jpg" || e == "jpeg" || e == "webp" || e == "bmp" || e == "gif";
}

bool isAlbumVideoExt(const std::string& ext)
{
    std::string e = toLower(ext);
    return e == "mp4" || e == "mov" || e == "m4v" || e == "webm" || e == "mkv" || e == "avi";
}

bool isAlbumMediaExt(const std::string& ext)
{
    return isAlbumImageExt(ext) || isAlbumVideoExt(ext);
}

fs::path albumRoot()
{
    return fs::path(ALBUM_ROOT);
}

bool addDirectoryToZip(mz_zip_archive& zip, const fs::path& root, const fs::path& dir)
{
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec)
            return false;
        if (!entry.is_regular_file(ec))
            continue;

        fs::path relative = fs::relative(entry.path(), root, ec);
        if (ec)
            relative = entry.path().filename();
        std::string archiveName = relative.generic_string();
        if (archiveName.empty())
            continue;
        if (!mz_zip_writer_add_file(&zip, archiveName.c_str(), entry.path().string().c_str(), nullptr, 0, MZ_DEFAULT_COMPRESSION))
            return false;
    }
    return true;
}

fs::path zipDirectoryToCache(const fs::path& dir, std::string& error)
{
    fs::path zipDir = fs::path(beiklive::path::cachePath()) / "web_downloads";
    ensureDir(zipDir.string());
    fs::path zipPath = uniqueFileTarget(zipDir / (safePathComponent(dir.filename().string(), false) + ".zip"));

    mz_zip_archive zip = {};
    if (!mz_zip_writer_init_file(&zip, zipPath.string().c_str(), 0))
    {
        error = "create zip failed";
        return {};
    }

    bool ok = addDirectoryToZip(zip, dir, dir);
    if (ok)
        ok = mz_zip_writer_finalize_archive(&zip) != 0;
    mz_zip_writer_end(&zip);
    if (!ok)
    {
        std::error_code ec;
        fs::remove(zipPath, ec);
        error = "write zip failed";
        return {};
    }
    return zipPath;
}

std::string defaultSavePath(const std::string& stem)
{
    fs::path path = fs::path(beiklive::path::savePath()) / "web" / stem;
    ensureDir(path.string());
    return path.string();
}

fs::path fileBrowserRoot()
{
#ifdef __SWITCH__
    return fs::path("sdmc:/");
#else
    std::error_code ec;
    fs::path root = beiklive::path::ROOT.empty() ? fs::current_path(ec) : fs::path(beiklive::path::ROOT);
    fs::path canonical = fs::weakly_canonical(root, ec);
    return ec ? root : canonical;
#endif
}

std::string uploadDirForKind(const std::string& kind, int platform)
{
    if (kind == "cover")
        return (fs::path(beiklive::path::cachePath()) / "covers").string();
    if (kind == "save")
        return beiklive::path::savePath();
    if (kind == "file")
        return fileBrowserRoot().string();
    return (fs::path(beiklive::path::romPath()) / platformDir(platform)).string();
}

std::string saveDirForGame(const beiklive::GameEntry& game)
{
    if (!game.savePath.empty())
        return game.savePath;
    return defaultSavePath(fs::path(game.path).stem().string());
}

std::uintmax_t safeFileSize(const fs::path& path)
{
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    return ec ? 0 : size;
}

std::string safeModTime(const fs::path& path)
{
    return beiklive::tools::getFileModTimeStr(path.string());
}

int parseStateSlot(const std::string& stem, const fs::path& path)
{
    std::string name = path.filename().string();
    std::string prefix = stem + ".ss";
    if (name.rfind(prefix, 0) != 0)
        return -1;

    std::string tail = name.substr(prefix.size());
    if (tail.empty() || !std::all_of(tail.begin(), tail.end(), [](unsigned char c) { return std::isdigit(c); }))
        return -1;

    try
    {
        return std::stoi(tail);
    }
    catch (...)
    {
        return -1;
    }
}

std::vector<fs::path> listImageFiles(const std::string& gameId = "")
{
    std::vector<fs::path> images;
    std::vector<fs::path> roots = {
        fs::path(beiklive::path::ROOT) / beiklive::path::PROGRAM_NAME / "logos",
    };
    ensureDir(roots.front().string());

    if (!gameId.empty())
    {
        auto game = findGame(gameId);
        if (game && !game->savePath.empty())
            roots.push_back(game->savePath);
    }

    std::set<std::string> visitedRoots;
    std::set<std::string> visitedImages;
    for (const auto& root : roots)
    {
        std::error_code ec;
        fs::path canonicalRoot = fs::weakly_canonical(root, ec);
        std::string rootKey = ec ? root.string() : canonicalRoot.string();
        if (!visitedRoots.insert(rootKey).second || !fs::exists(root, ec))
            continue;
        for (const auto& entry : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec || !entry.is_regular_file())
                continue;
            std::string ext = trimExtDot(entry.path().extension().string());
            if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp")
            {
                fs::path canonicalImage = fs::weakly_canonical(entry.path(), ec);
                std::string imageKey = ec ? entry.path().string() : canonicalImage.string();
                if (visitedImages.insert(imageKey).second)
                    images.push_back(entry.path());
            }
        }
    }
    return images;
}

std::string encodedPathParam(const fs::path& path)
{
    std::string raw = path.string();
    std::string out(raw.size() * 3 + 1, '\0');
    size_t n = mg_url_encode(raw.c_str(), raw.size(), out.data(), out.size());
    out.resize(n);
    return out;
}

std::string safeHeaderFilename(std::string name)
{
    for (char& c : name)
    {
        if (c == '"' || c == '\\' || c == '\r' || c == '\n')
            c = '_';
    }
    return name.empty() ? "save.dat" : name;
}

std::string htmlRoot()
{
#ifdef __SWITCH__
    fs::path sd = fs::path("sdmc:/GBAStation/web");
    std::error_code ec;
    if (fs::exists(sd, ec))
        return sd.string();
#endif
    return beiklive::res_path("web");
}

std::string webLogoPath()
{
#ifdef __SWITCH__
    fs::path requested("romfs:/icon/defaullt.png");
    std::error_code ec;
    if (fs::exists(requested, ec))
        return requested.string();

    fs::path fallback("romfs:/icon/default.png");
    if (fs::exists(fallback, ec))
        return fallback.string();
#endif
    return beiklive::res_path("icon/default.png");
}

bool pathInsideDirectory(const fs::path& target, const fs::path& dir, fs::path& canonicalTarget)
{
    std::error_code ec;
    fs::path canonicalDir = fs::weakly_canonical(dir, ec);
    if (ec)
        return false;
    canonicalTarget = fs::weakly_canonical(target, ec);
    if (ec)
        return false;
    std::string dirStr = canonicalDir.string();
    std::string targetStr = canonicalTarget.string();
    return targetStr == dirStr || (targetStr.size() > dirStr.size() &&
           targetStr.rfind(dirStr, 0) == 0 &&
           (dirStr.empty() || dirStr.back() == fs::path::preferred_separator || dirStr.back() == '/' || dirStr.back() == '\\' ||
            targetStr[dirStr.size()] == fs::path::preferred_separator ||
            targetStr[dirStr.size()] == '/' ||
           targetStr[dirStr.size()] == '\\'));
}

fs::path browserPathFromClient(const std::string& raw)
{
    fs::path root = fileBrowserRoot();
    if (raw.empty())
        return root;

    fs::path path(raw);
#ifdef __SWITCH__
    std::string text = path.string();
    if (text == "sdmc:")
        return root;
    if (text.rfind("sdmc:/", 0) != 0 && text != "sdmc:")
        path = root / path;
#else
    if (path.is_relative())
        path = root / path;
#endif
    return path.lexically_normal();
}

bool pathInsideFileBrowser(const fs::path& target);

fs::path browserParentPath(const fs::path& path)
{
#ifdef __SWITCH__
    std::string text = path.lexically_normal().string();
    if (text == "sdmc:" || text == "sdmc:/")
        return {};
    while (text.size() > 6 && text.back() == '/')
        text.pop_back();
    if (text.rfind("sdmc:/", 0) != 0)
        return {};
    size_t slash = text.find_last_of('/');
    if (slash == std::string::npos || slash <= 5)
        return fs::path("sdmc:/");
    return fs::path(text.substr(0, slash));
#else
    fs::path parent = path.parent_path();
    if (!parent.empty() && pathInsideFileBrowser(parent) && parent != path)
        return parent;
    return {};
#endif
}

bool pathInsideFileBrowser(const fs::path& target)
{
    fs::path root = fileBrowserRoot();
#ifdef __SWITCH__
    std::string rootStr = root.string();
    std::string targetStr = target.lexically_normal().string();
    return targetStr == rootStr || targetStr == "sdmc:" || targetStr.rfind("sdmc:/", 0) == 0;
#else
    fs::path canonicalTarget;
    return pathInsideDirectory(target, root, canonicalTarget);
#endif
}

bool pathInsideAlbum(const fs::path& target)
{
    fs::path root = albumRoot();
#ifdef __SWITCH__
    std::string rootStr = root.lexically_normal().string();
    std::string targetStr = target.lexically_normal().string();
    return targetStr == rootStr || (targetStr.size() > rootStr.size() &&
           targetStr.rfind(rootStr, 0) == 0 &&
           (rootStr.empty() || rootStr.back() == '/' || targetStr[rootStr.size()] == '/'));
#else
    fs::path canonicalTarget;
    return pathInsideDirectory(target, root, canonicalTarget);
#endif
}

nlohmann::json albumEntryJson(const fs::path& path)
{
    std::string ext = trimExtDot(path.extension().string());
    return {
        {"name", path.filename().string()},
        {"path", path.string()},
        {"type", isAlbumVideoExt(ext) ? "video" : "image"},
        {"size", safeFileSize(path)},
        {"modified", safeModTime(path)},
        {"ext", ext},
        {"url", "/api/album/view?path=" + encodedPathParam(path)},
    };
}

bool writablePathInsideFileBrowser(const fs::path& target)
{
    fs::path parent = target.parent_path();
    if (parent.empty())
        parent = fileBrowserRoot();
    return pathInsideFileBrowser(parent);
}

nlohmann::json fileEntryJson(const fs::directory_entry& entry)
{
    std::error_code ec;
    fs::path path = entry.path();
    bool isDir = entry.is_directory(ec);
    return {
        {"name", path.filename().string()},
        {"path", path.string()},
        {"isDir", isDir},
        {"size", isDir ? 0 : safeFileSize(path)},
        {"modified", safeModTime(path)},
        {"ext", isDir ? "" : trimExtDot(path.extension().string())},
    };
}

} // namespace

ApiRouter::ApiRouter(std::atomic<bool>& stopRequested)
    : stopRequested_(stopRequested)
{
}

void ApiRouter::Handle(mg_connection* c, mg_http_message* hm)
{
    std::string method = strFromMg(hm->method);
    std::string uri = strFromMg(hm->uri);

    if (method == "OPTIONS")
    {
        mg_http_reply(c, 204, CORS_HEADERS, "");
        return;
    }

    try
    {
        if (uri.rfind("/api/", 0) == 0)
            handleApi(c, hm, method, uri);
        else
            serveStatic(c, hm);
    }
    catch (const std::exception& e)
    {
        replyError(c, 500, e.what());
    }
    catch (...)
    {
        replyError(c, 500, "unknown error");
    }
}

void ApiRouter::handleApi(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri)
{
    if (uri == "/api/games" && method == "GET")
        return handleGames(c);
    if (uri == "/api/upload/start" && method == "POST")
        return handleUploadStart(c, hm);
    if (uri == "/api/upload/chunk" && method == "POST")
        return handleUploadChunk(c, hm);
    if (uri == "/api/upload/finish" && method == "POST")
        return handleUploadFinish(c, hm);
    if (uri == "/api/upload/cancel" && method == "POST")
        return handleUploadCancel(c, hm);
    if (uri == "/api/images" && method == "GET")
        return handleImages(c, hm);
    if (uri == "/api/image" && method == "GET")
        return handleImageFile(c, hm);
    if (uri == "/api/logo" && method == "GET")
        return handleLogoFile(c, hm);
    if (uri.rfind("/api/album", 0) == 0)
        return handleAlbum(c, hm, method, uri);
    if (uri.rfind("/api/files", 0) == 0)
        return handleFiles(c, hm, method, uri);
    if (uri.rfind("/api/system", 0) == 0)
        return handleSystem(c, hm, method, uri);
    if (uri.rfind("/api/game/", 0) == 0)
        return handleGameById(c, hm, method, uri);
    replyError(c, 404, "api not found");
}

void ApiRouter::handleGames(mg_connection* c)
{
    nlohmann::json games = nlohmann::json::array();
    if (beiklive::GameDB)
    {
        for (const auto& entry : beiklive::GameDB->getAll())
        {
            games.push_back(gameJsonWithMeta(entry));
        }
    }
    replyJson(c, 200, {{"ok", true}, {"games", games}});
}

void ApiRouter::handleGameById(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri)
{
    std::string rest = uri.substr(std::string("/api/game/").size());
    auto slash = rest.find('/');
    std::string id = slash == std::string::npos ? rest : rest.substr(0, slash);
    std::string action = slash == std::string::npos ? "" : rest.substr(slash + 1);

    if (action == "cover" && method == "GET")
        return handleCoverFile(c, hm, id);
    if (action == "saves" && method == "GET")
        return handleSaveList(c, hm, id);
    if (action == "save/delete" && method == "DELETE")
        return handleSaveDelete(c, hm, id);
    if (action == "save/export" && method == "GET")
        return handleSaveExport(c, hm, id);
    if (action == "save/start" && method == "POST")
        return handleSaveStart(c, hm, id);
    if (action == "cover/start" && method == "POST")
        return handleCoverStart(c, hm, id);
    if (action == "cover/select" && method == "POST")
        return handleCoverSelect(c, hm, id);

    auto game = findGame(id);
    if (!game)
        return replyError(c, 404, "game not found");

    if (method == "DELETE")
    {
        nlohmann::json body = parseJsonBody(hm);
        bool deleteFile = jsonBoolValue(body, "deleteFile");
        std::string path = game->path;
        bool removed = beiklive::GameDB && beiklive::GameDB->removeByPath(path);
        if (removed && deleteFile)
        {
            std::error_code ec;
            fs::remove(path, ec);
        }
        bool saved = removed && beiklive::GameDB->flush();
        return replyJson(c, 200, {{"ok", removed}, {"saved", saved}});
    }

    if (method == "PUT")
    {
        nlohmann::json patch = parseJsonBody(hm);
        nlohmann::json updated;
        beiklive::to_json(updated, *game);
        if (patch.is_object())
        {
            for (auto it = patch.begin(); it != patch.end(); ++it)
            {
                if (isRemoteEditableGameField(it.key()))
                    updated[it.key()] = it.value();
            }
        }
        try
        {
            beiklive::from_json(updated, *game);
        }
        catch (...)
        {
            return replyError(c, 400, "invalid game config");
        }
        bool saved = saveGame(*game);
        return replyJson(c, 200, {{"ok", saved}, {"saved", saved}, {"game", gameJsonWithMeta(*game)}});
    }

    replyError(c, 405, "method not allowed");
}

std::string ApiRouter::makeToken()
{
    std::lock_guard<std::mutex> lock(uploadMutex_);
    return std::to_string(nextToken_++);
}

void ApiRouter::handleUploadStart(mg_connection* c, mg_http_message* hm)
{
    std::string kind = jsonString(hm, "$.kind", "rom");
    std::string originalName = jsonString(hm, "$.name");
    bool importNameMapping = jsonBool(hm, "$.importNameMapping", false);
    std::uint64_t totalSize = static_cast<std::uint64_t>(std::max<long>(0, jsonLong(hm, "$.size")));

    if (originalName.empty())
        return replyError(c, 400, "missing filename");
    if (totalSize > MAX_UPLOAD_SIZE)
        return replyError(c, 400, "file too large");

    fs::path original(originalName);
    std::string ext = trimExtDot(original.extension().string());
    int platform = platformFromExt(ext);
    if (kind == "rom" && platform == 0)
        return replyError(c, 400, "unsupported rom extension");
    if (kind == "cover" && !(ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "webp"))
        return replyError(c, 400, "unsupported image extension");

    std::string stem = original.stem().string();
    std::string safeStem = safeStemFromTitle(stem);
    std::string storageStem = kind == "rom" ? safeRomStemFromTitle(stem) : safeStem;
    bool stemHasChinese = containsChineseChar(stem);
    std::string target;
    std::string finalPath;
    bool renamed = containsNonAscii(originalName);
    if (kind == "file")
    {
        std::string targetDirRaw = jsonString(hm, "$.path");
        std::string relativePath = jsonString(hm, "$.relativePath", originalName);
        fs::path targetDir = browserPathFromClient(targetDirRaw);
        if (!pathInsideFileBrowser(targetDir))
            return replyError(c, 403, "path outside file browser root");
        ensureDir(targetDir.string());

        bool relativeRenamed = false;
        fs::path safeRelative = safeUploadRelativePath(relativePath, originalName, relativeRenamed);
        fs::path requested = (targetDir / safeRelative).lexically_normal();
        if (!writablePathInsideFileBrowser(requested))
            return replyError(c, 403, "target outside file browser root");
        ensureDir(requested.parent_path().string());
        fs::path finalTarget = uniqueFileTarget(requested);
        fs::path tempTarget = finalTarget;
        tempTarget += ".uploading";
        target = tempTarget.string();
        finalPath = finalTarget.string();
        renamed = renamed || relativeRenamed || finalTarget.filename() != requested.filename();
    }
    else
    {
        std::string targetDir = uploadDirForKind(kind, platform);
        ensureDir(targetDir);
        target = uniquePath(targetDir, storageStem, ext);
        finalPath = target;
    }

    UploadSession session;
    session.token = makeToken();
    session.kind = kind;
    session.originalName = originalName;
    session.originalStem = stem;
    session.title = containsNonAscii(stem) ? stem : safeStem;
    session.targetPath = target;
    session.finalPath = finalPath;
    session.platform = platform;
    session.totalSize = totalSize;
    session.importNameMapping = importNameMapping;
    session.renamedFromChinese = kind == "rom" && stemHasChinese && fs::path(target).stem().string() != stem;

    {
        std::lock_guard<std::mutex> lock(uploadMutex_);
        uploads_[session.token] = session;
    }

    replyJson(c, 200, {
        {"ok", true},
        {"token", session.token},
        {"targetName", fs::path(finalPath.empty() ? target : finalPath).filename().string()},
        {"targetPath", finalPath.empty() ? target : finalPath},
        {"renamed", renamed},
        {"title", session.title},
        {"platform", platform},
    });
}

bool ApiRouter::writeChunk(const UploadSession& session, mg_http_message* hm, std::uint64_t offset, std::string& error)
{
    if (offset + hm->body.len > session.totalSize || session.totalSize > MAX_UPLOAD_SIZE)
    {
        error = "upload size mismatch";
        return false;
    }

    FILE* fp = std::fopen(session.targetPath.c_str(), offset == 0 ? "wb" : "r+b");
    if (!fp)
    {
        error = "open target failed";
        return false;
    }
    if (std::fseek(fp, static_cast<long>(offset), SEEK_SET) != 0)
    {
        std::fclose(fp);
        error = "seek target failed";
        return false;
    }
    size_t written = std::fwrite(hm->body.buf, 1, hm->body.len, fp);
    std::fclose(fp);
    if (written != hm->body.len)
    {
        error = "write target failed";
        return false;
    }
    return true;
}

void ApiRouter::handleUploadChunk(mg_connection* c, mg_http_message* hm)
{
    std::string token = decodeQuery(hm, "token");
    std::uint64_t offset = 0;
    try { offset = static_cast<std::uint64_t>(std::stoull(decodeQuery(hm, "offset"))); } catch (...) {}

    UploadSession session;
    {
        std::lock_guard<std::mutex> lock(uploadMutex_);
        auto it = uploads_.find(token);
        if (it == uploads_.end())
            return replyError(c, 404, "upload not found");
        session = it->second;
    }

    std::string error;
    if (!writeChunk(session, hm, offset, error))
        return replyError(c, 400, error);
    replyJson(c, 200, {{"ok", true}, {"offset", offset + hm->body.len}});
}

void ApiRouter::handleUploadCancel(mg_connection* c, mg_http_message* hm)
{
    std::string token = jsonString(hm, "$.token");
    if (token.empty())
        token = decodeQuery(hm, "token");

    UploadSession session;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(uploadMutex_);
        auto it = uploads_.find(token);
        if (it != uploads_.end())
        {
            session = it->second;
            uploads_.erase(it);
            found = true;
        }
    }

    if (found && !session.targetPath.empty())
    {
        std::error_code ec;
        fs::remove(session.targetPath, ec);
    }

    replyJson(c, 200, {{"ok", true}, {"cancelled", found}});
}

void ApiRouter::handleUploadFinish(mg_connection* c, mg_http_message* hm)
{
    std::string token = jsonString(hm, "$.token");
    UploadSession session;
    {
        std::lock_guard<std::mutex> lock(uploadMutex_);
        auto it = uploads_.find(token);
        if (it == uploads_.end())
            return replyError(c, 404, "upload not found");
        session = it->second;
        uploads_.erase(it);
    }

    if (session.kind == "rom")
    {
        int detectedPlatform = beiklive::tools::detectGamePlatform(session.targetPath);
        int platform = detectedPlatform >= 0 ? detectedPlatform : session.platform;
        if (platform != session.platform)
        {
            fs::path targetDir = uploadDirForKind("rom", platform);
            ensureDir(targetDir.string());
            fs::path movedPath = uniqueFileTarget(targetDir / fs::path(session.targetPath).filename());
            std::error_code ec;
            fs::rename(session.targetPath, movedPath, ec);
            if (!ec)
                session.targetPath = movedPath.string();
        }

        beiklive::GameEntry entry;
        entry.path = session.targetPath;
        entry.title = session.title;
        entry.platform = platform;
        if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS))
            entry.threeDsTitleId = beiklive::three_ds::readNcsdTitleId(session.targetPath);
        entry.logoPath = beiklive::tools::getDefaultLogoPath(
            static_cast<beiklive::enums::EmuPlatform>(platform),
            session.targetPath);
        entry.savePath = beiklive::tools::defaultGameSavePath(platform, session.targetPath);
        if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuPSP)) {
            // PSP ROM 入库时提取真实游戏标题与 ICON0 封面（保存到该 ROM 的存档目录）。
            const std::string realTitle = beiklive::psp_meta::ExtractTitle(session.targetPath);
            if (!realTitle.empty())
                entry.title = realTitle;
            if (entry.logoPath.empty() ||
                entry.logoPath == beiklive::tools::getDefaultLogoPath(
                    static_cast<beiklive::enums::EmuPlatform>(platform), session.targetPath))
            {
                const std::string icon = beiklive::psp_meta::ExtractIcon0(
                    session.targetPath, entry.savePath);
                if (!icon.empty())
                    entry.logoPath = icon;
            }
        }
        if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)) {
            entry.ndsScreenLayout = "priority_top";
            entry.ndsScreenOrientation = "0";
            entry.ndsIntegerScale = true;
            entry.ndsScreenGap = 0;
            entry.ndsBottomOpacity = 1.0f;
        }
        ensureDir(entry.savePath);
        bool saved = saveGame(entry);
        if (saved && session.importNameMapping && session.renamedFromChinese &&
            !session.originalStem.empty() && beiklive::NameMappingManager)
        {
            std::string mappedKey = fs::path(session.targetPath).stem().string();
            if (!mappedKey.empty() && mappedKey != session.originalStem)
            {
                SET_MAPPING_KEY_STR(mappedKey, session.originalStem);
                beiklive::NameMappingManager->Save();
            }
        }
        return replyJson(c, 200, {{"ok", saved}, {"saved", saved}, {"gameId", gameIdFromEntry(entry)}, {"platform", platform}});
    }

    if (session.kind == "save")
    {
        auto game = findGame(session.gameId);
        if (!game)
            return replyError(c, 404, "game not found");
        game->savePath = fs::path(session.targetPath).parent_path().string();
        bool saved = saveGame(*game);
        return replyJson(c, 200, {{"ok", saved}, {"saved", saved}, {"path", session.targetPath}});
    }

    if (session.kind == "cover")
    {
        auto game = findGame(session.gameId);
        if (!game)
            return replyError(c, 404, "game not found");
        game->logoPath = session.targetPath;
        bool saved = saveGame(*game);
        return replyJson(c, 200, {{"ok", saved}, {"saved", saved}, {"path", session.targetPath}});
    }

    if (session.kind == "file")
    {
        fs::path finalPath = session.finalPath.empty() ? session.targetPath : session.finalPath;
        std::error_code ec;
        if (session.totalSize == 0 && !fs::exists(session.targetPath, ec))
        {
            std::ofstream empty(session.targetPath, std::ios::binary);
        }
        if (fs::exists(finalPath, ec))
            finalPath = uniqueFileTarget(finalPath);
        fs::rename(session.targetPath, finalPath, ec);
        if (ec)
        {
            fs::copy_file(session.targetPath, finalPath, fs::copy_options::overwrite_existing, ec);
            if (!ec)
                fs::remove(session.targetPath, ec);
        }
        if (ec)
            return replyError(c, 500, "finalize upload failed");
        return replyJson(c, 200, {{"ok", true}, {"path", finalPath.string()}, {"name", finalPath.filename().string()}});
    }

    replyJson(c, 200, {{"ok", true}, {"path", session.targetPath}});
}

void ApiRouter::handleSaveStart(mg_connection* c, mg_http_message* hm, const std::string& gameId)
{
    auto game = findGame(gameId);
    if (!game)
        return replyError(c, 404, "game not found");

    fs::path rom(game->path);
    std::string targetDir = saveDirForGame(*game);
    ensureDir(targetDir);
    std::string type = jsonString(hm, "$.type", "battery");
    int slot = static_cast<int>(jsonLong(hm, "$.slot", 0));

    UploadSession session;
    session.token = makeToken();
    session.kind = "save";
    session.gameId = gameId;
    session.originalName = jsonString(hm, "$.name", type == "state" ? rom.stem().string() + ".ss" + std::to_string(slot)
                                                                     : rom.stem().string() + ".sav");
    session.totalSize = static_cast<std::uint64_t>(std::max<long>(0, jsonLong(hm, "$.size")));
    session.targetPath = (fs::path(targetDir) /
                          (type == "state" ? rom.stem().string() + ".ss" + std::to_string(slot)
                                           : rom.stem().string() + ".sav"))
                             .string();

    {
        std::lock_guard<std::mutex> lock(uploadMutex_);
        uploads_[session.token] = session;
    }
    replyJson(c, 200, {{"ok", true}, {"token", session.token}, {"targetPath", session.targetPath}});
}

void ApiRouter::handleSaveList(mg_connection* c, mg_http_message*, const std::string& gameId)
{
    auto game = findGame(gameId);
    if (!game)
        return replyError(c, 404, "game not found");

    fs::path rom(game->path);
    std::string stem = rom.stem().string();
    fs::path saveDir = saveDirForGame(*game);
    nlohmann::json battery = nlohmann::json::array();
    nlohmann::json states = nlohmann::json::array();

    fs::path batteryPath = saveDir / (stem + ".sav");
    if (fs::exists(batteryPath))
    {
        battery.push_back({
            {"type", "battery"},
            {"name", batteryPath.filename().string()},
            {"path", batteryPath.string()},
            {"size", safeFileSize(batteryPath)},
            {"modified", safeModTime(batteryPath)},
        });
    }

    std::error_code ec;
    if (fs::exists(saveDir, ec))
    {
        for (const auto& entry : fs::directory_iterator(saveDir, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec || !entry.is_regular_file())
                continue;

            fs::path path = entry.path();
            if (trimExtDot(path.extension().string()) == "png")
                continue;

            int slot = parseStateSlot(stem, path);
            if (slot < 0)
                continue;

            fs::path thumb = path.string() + ".png";
            nlohmann::json item = {
                {"type", "state"},
                {"slot", slot},
                {"name", path.filename().string()},
                {"path", path.string()},
                {"size", safeFileSize(path)},
                {"modified", safeModTime(path)},
                {"thumbPath", fs::exists(thumb) ? thumb.string() : ""},
                {"thumbUrl", fs::exists(thumb) ? "/api/image?path=" + encodedPathParam(thumb) : ""},
            };
            states.push_back(item);
        }
    }

    std::sort(states.begin(), states.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
        return a.value("slot", 0) < b.value("slot", 0);
    });

    replyJson(c, 200, {
        {"ok", true},
        {"saveDir", saveDir.string()},
        {"battery", battery},
        {"states", states},
    });
}

void ApiRouter::handleSaveDelete(mg_connection* c, mg_http_message* hm, const std::string& gameId)
{
    auto game = findGame(gameId);
    if (!game)
        return replyError(c, 404, "game not found");

    std::string path = jsonString(hm, "$.path");
    if (path.empty())
        return replyError(c, 400, "missing path");

    fs::path canonicalTarget;
    if (!pathInsideDirectory(path, saveDirForGame(*game), canonicalTarget))
        return replyError(c, 403, "path outside save dir");

    std::error_code ec;
    bool removed = fs::remove(canonicalTarget, ec);
    fs::remove(fs::path(canonicalTarget.string() + ".png"), ec);
    bool saved = beiklive::GameDB ? beiklive::GameDB->flush() : false;
    replyJson(c, 200, {{"ok", removed}, {"saved", saved}});
}

void ApiRouter::handleSaveExport(mg_connection* c, mg_http_message* hm, const std::string& gameId)
{
    auto game = findGame(gameId);
    if (!game)
        return replyError(c, 404, "game not found");

    std::string path = decodeQuery(hm, "path");
    if (path.empty())
        return replyError(c, 400, "missing path");

    fs::path canonicalTarget;
    if (!pathInsideDirectory(path, saveDirForGame(*game), canonicalTarget))
        return replyError(c, 403, "path outside save dir");
    if (!fs::exists(canonicalTarget) || !fs::is_regular_file(canonicalTarget))
        return replyError(c, 404, "save file not found");

    std::string disposition = "Content-Disposition: attachment; filename=\"" +
        safeHeaderFilename(canonicalTarget.filename().string()) + "\"\r\n";
    std::string headers = std::string(CORS_HEADERS) + disposition;
    mg_http_serve_opts opts = {};
    opts.extra_headers = headers.c_str();
    mg_http_serve_file(c, hm, canonicalTarget.string().c_str(), &opts);
}

void ApiRouter::handleCoverStart(mg_connection* c, mg_http_message* hm, const std::string& gameId)
{
    auto game = findGame(gameId);
    if (!game)
        return replyError(c, 404, "game not found");

    fs::path original(jsonString(hm, "$.name", "cover.png"));
    std::string ext = trimExtDot(original.extension().string());
    if (ext != "png" && ext != "jpg" && ext != "jpeg" && ext != "webp")
        return replyError(c, 400, "unsupported image extension");

    std::string stem = fs::path(game->path).stem().string() + "_cover";
    fs::path targetDir = fs::path(beiklive::path::cachePath()) / "covers";
    ensureDir(targetDir.string());

    UploadSession session;
    session.token = makeToken();
    session.kind = "cover";
    session.gameId = gameId;
    session.originalName = original.string();
    session.totalSize = static_cast<std::uint64_t>(std::max<long>(0, jsonLong(hm, "$.size")));
    session.targetPath = uniquePath(targetDir, safeStemFromTitle(stem), ext);

    {
        std::lock_guard<std::mutex> lock(uploadMutex_);
        uploads_[session.token] = session;
    }
    replyJson(c, 200, {{"ok", true}, {"token", session.token}, {"targetPath", session.targetPath}});
}

void ApiRouter::handleCoverSelect(mg_connection* c, mg_http_message* hm, const std::string& gameId)
{
    auto game = findGame(gameId);
    if (!game)
        return replyError(c, 404, "game not found");
    std::string path = jsonString(hm, "$.path");
    if (path.empty() || !fs::exists(path))
        return replyError(c, 400, "image not found");
    game->logoPath = path;
    bool saved = saveGame(*game);
    replyJson(c, 200, {{"ok", saved}, {"saved", saved}, {"logoPath", path}});
}

void ApiRouter::handleImages(mg_connection* c, mg_http_message* hm)
{
    std::string gameId = decodeQuery(hm, "gameId");
    nlohmann::json items = nlohmann::json::array();
    for (const auto& path : listImageFiles(gameId))
    {
        items.push_back({
            {"name", path.filename().string()},
            {"path", path.string()},
            {"url", "/api/image?path=" + encodedPathParam(path)},
        });
    }
    replyJson(c, 200, {{"ok", true}, {"images", items}});
}

void ApiRouter::handleImageFile(mg_connection* c, mg_http_message* hm)
{
    std::string path = decodeQuery(hm, "path");
    if (path.empty() || !fs::exists(path))
        return replyError(c, 404, "image not found");
    mg_http_serve_opts opts = {};
    opts.extra_headers = CORS_HEADERS;
    mg_http_serve_file(c, hm, path.c_str(), &opts);
}

void ApiRouter::handleAlbum(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri)
{
    if (uri == "/api/album/list" && method == "GET")
    {
        fs::path root = albumRoot();
        nlohmann::json items = nlohmann::json::array();
        std::error_code ec;
        if (fs::exists(root, ec) && fs::is_directory(root, ec))
        {
            fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
            fs::recursive_directory_iterator end;
            for (; it != end; it.increment(ec))
            {
                if (ec) { ec.clear(); continue; }
                std::error_code entryEc;
                if (!it->is_regular_file(entryEc) || entryEc)
                    continue;
                fs::path path = it->path();
                std::string ext = trimExtDot(path.extension().string());
                if (isAlbumMediaExt(ext))
                    items.push_back(albumEntryJson(path));
            }
        }

        std::sort(items.begin(), items.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
            std::string am = a.value("modified", "");
            std::string bm = b.value("modified", "");
            if (am != bm) return am > bm;
            return a.value("name", "").compare(b.value("name", "")) < 0;
        });

        return replyJson(c, 200, {
            {"ok", true},
            {"root", root.string()},
            {"items", items},
        });
    }

    if (uri == "/api/album/view" && method == "GET")
    {
        fs::path path(decodeQuery(hm, "path"));
        if (!pathInsideAlbum(path))
            return replyError(c, 403, "path outside album root");
        if (!fs::exists(path) || !fs::is_regular_file(path))
            return replyError(c, 404, "media not found");
        std::string ext = trimExtDot(path.extension().string());
        if (!isAlbumMediaExt(ext))
            return replyError(c, 400, "unsupported media type");
        std::string headers = std::string(CORS_HEADERS) + "Content-Type: " + mimeTypeForExt(ext) + "\r\n";
        mg_http_serve_opts opts = {};
        opts.extra_headers = headers.c_str();
        return mg_http_serve_file(c, hm, path.string().c_str(), &opts);
    }

    replyError(c, 404, "album api not found");
}

void ApiRouter::handleLogoFile(mg_connection* c, mg_http_message* hm)
{
    std::string path = webLogoPath();
    if (path.empty() || !fs::exists(path))
        return replyError(c, 404, "logo not found");
    mg_http_serve_opts opts = {};
    opts.extra_headers = CORS_HEADERS;
    mg_http_serve_file(c, hm, path.c_str(), &opts);
}

void ApiRouter::handleCoverFile(mg_connection* c, mg_http_message* hm, const std::string& gameId)
{
    auto game = findGame(gameId);
    if (!game)
        return replyError(c, 404, "game not found");
    if (beiklive::tools::tryUseNdsInternalIconCover(*game))
        saveGame(*game);
    std::string path = game->logoPath;
    if (path.empty() || !fs::exists(path))
        path = beiklive::tools::getDefaultLogoPath(
            static_cast<beiklive::enums::EmuPlatform>(game->platform),
            game->path);
    if (!fs::exists(path))
        return replyError(c, 404, "cover not found");
    mg_http_serve_opts opts = {};
    opts.extra_headers = CORS_HEADERS;
    mg_http_serve_file(c, hm, path.c_str(), &opts);
}

void ApiRouter::handleSystem(mg_connection* c, mg_http_message*, const std::string& method, const std::string& uri)
{
    if (uri == "/api/system/stop" && method == "POST")
    {
        stopRequested_.store(true);
        return replyJson(c, 200, {{"ok", true}});
    }
    replyError(c, 404, "system api not found");
}

void ApiRouter::handleFiles(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri)
{
    if (uri == "/api/files/list" && method == "GET")
    {
        fs::path path = browserPathFromClient(decodeQuery(hm, "path"));
        if (!pathInsideFileBrowser(path))
            return replyError(c, 403, "path outside file browser root");
        if (!fs::exists(path) || !fs::is_directory(path))
            return replyError(c, 404, "directory not found");

        nlohmann::json entries = nlohmann::json::array();
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec)
                break;
            entries.push_back(fileEntryJson(entry));
        }
        std::sort(entries.begin(), entries.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
            bool ad = a.value("isDir", false);
            bool bd = b.value("isDir", false);
            if (ad != bd)
                return ad > bd;
            return a.value("name", "").compare(b.value("name", "")) < 0;
        });

        fs::path parent = browserParentPath(path);
        return replyJson(c, 200, {
            {"ok", true},
            {"root", fileBrowserRoot().string()},
            {"path", path.string()},
            {"parent", parent.empty() ? "" : parent.string()},
            {"entries", entries},
        });
    }

    if (uri == "/api/files/upload/start" && method == "POST")
        return handleUploadStart(c, hm);

    if (uri == "/api/files/download" && method == "GET")
    {
        fs::path path = browserPathFromClient(decodeQuery(hm, "path"));
        if (!pathInsideFileBrowser(path))
            return replyError(c, 403, "path outside file browser root");
        if (!fs::exists(path))
            return replyError(c, 404, "file not found");
        fs::path servedPath = path;
        if (fs::is_directory(path))
        {
            std::string error;
            servedPath = zipDirectoryToCache(path, error);
            if (servedPath.empty())
                return replyError(c, 500, error);
        }
        else if (!fs::is_regular_file(path))
        {
            return replyError(c, 404, "file not found");
        }
        std::string disposition = "Content-Disposition: attachment; filename=\"" +
            safeHeaderFilename(servedPath.filename().string()) + "\"\r\n";
        std::string headers = std::string(CORS_HEADERS) + disposition;
        mg_http_serve_opts opts = {};
        opts.extra_headers = headers.c_str();
        return mg_http_serve_file(c, hm, servedPath.string().c_str(), &opts);
    }

    if (uri == "/api/files/view" && method == "GET")
    {
        fs::path path = browserPathFromClient(decodeQuery(hm, "path"));
        if (!pathInsideFileBrowser(path))
            return replyError(c, 403, "path outside file browser root");
        if (!fs::exists(path) || !fs::is_regular_file(path))
            return replyError(c, 404, "file not found");
        std::string headers = std::string(CORS_HEADERS) + "Content-Type: " + mimeTypeForExt(trimExtDot(path.extension().string())) + "\r\n";
        mg_http_serve_opts opts = {};
        opts.extra_headers = headers.c_str();
        return mg_http_serve_file(c, hm, path.string().c_str(), &opts);
    }

    if (uri == "/api/files/text" && method == "GET")
    {
        fs::path path = browserPathFromClient(decodeQuery(hm, "path"));
        if (!pathInsideFileBrowser(path))
            return replyError(c, 403, "path outside file browser root");
        if (!fs::exists(path) || !fs::is_regular_file(path))
            return replyError(c, 404, "file not found");
        std::string ext = trimExtDot(path.extension().string());
        if (!isTextEditableExt(ext))
            return replyError(c, 400, "unsupported text extension");
        if (safeFileSize(path) > MAX_TEXT_EDIT_SIZE)
            return replyError(c, 400, "text file too large");
        return replyJson(c, 200, {
            {"ok", true},
            {"path", path.string()},
            {"name", path.filename().string()},
            {"content", loadTextFile(path.string())},
        });
    }

    if (uri == "/api/files/text" && method == "PUT")
    {
        fs::path path = browserPathFromClient(jsonString(hm, "$.path"));
        if (!pathInsideFileBrowser(path))
            return replyError(c, 403, "path outside file browser root");
        if (!fs::exists(path) || !fs::is_regular_file(path))
            return replyError(c, 404, "file not found");
        std::string ext = trimExtDot(path.extension().string());
        if (!isTextEditableExt(ext))
            return replyError(c, 400, "unsupported text extension");
        std::string content = jsonString(hm, "$.content");
        if (content.size() > MAX_TEXT_EDIT_SIZE)
            return replyError(c, 400, "text file too large");
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out)
            return replyError(c, 500, "open text file failed");
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!out)
            return replyError(c, 500, "write text file failed");
        return replyJson(c, 200, {{"ok", true}, {"path", path.string()}, {"size", content.size()}});
    }

    if (uri == "/api/files/mkdir" && method == "POST")
    {
        fs::path dir = browserPathFromClient(jsonString(hm, "$.path"));
        std::string name = jsonString(hm, "$.name");
        if (name.empty())
            return replyError(c, 400, "missing folder name");
        bool renamed = false;
        fs::path target = (dir / safeUploadRelativePath(name, name, renamed)).lexically_normal();
        if (!writablePathInsideFileBrowser(target))
            return replyError(c, 403, "path outside file browser root");
        std::error_code ec;
        bool created = fs::create_directories(target, ec);
        if (ec)
            return replyError(c, 500, "create directory failed");
        return replyJson(c, 200, {{"ok", true}, {"created", created}, {"path", target.string()}, {"renamed", renamed}});
    }

    if (uri == "/api/files/rename" && method == "POST")
    {
        fs::path path = browserPathFromClient(jsonString(hm, "$.path"));
        std::string name = jsonString(hm, "$.name");
        if (name.empty())
            return replyError(c, 400, "missing name");
        if (!pathInsideFileBrowser(path) || !fs::exists(path))
            return replyError(c, 404, "file not found");
        bool renamed = false;
        fs::path target = (path.parent_path() / safeUploadRelativePath(name, name, renamed)).lexically_normal();
        if (!writablePathInsideFileBrowser(target))
            return replyError(c, 403, "path outside file browser root");
        std::error_code ec;
        fs::rename(path, target, ec);
        if (ec)
            return replyError(c, 500, "rename failed");
        return replyJson(c, 200, {{"ok", true}, {"path", target.string()}, {"renamed", renamed}});
    }

    if (uri == "/api/files/move" && method == "POST")
    {
        fs::path path = browserPathFromClient(jsonString(hm, "$.path"));
        fs::path destDir = browserPathFromClient(jsonString(hm, "$.destDir"));
        if (!pathInsideFileBrowser(path) || !fs::exists(path))
            return replyError(c, 404, "file not found");
        if (!pathInsideFileBrowser(destDir) || !fs::exists(destDir) || !fs::is_directory(destDir))
            return replyError(c, 404, "target directory not found");
        fs::path target = uniqueFileTarget(destDir / path.filename());
        std::error_code ec;
        fs::rename(path, target, ec);
        if (ec)
            return replyError(c, 500, "move failed");
        return replyJson(c, 200, {{"ok", true}, {"path", target.string()}});
    }

    if (uri == "/api/files/delete" && method == "DELETE")
    {
        fs::path path = browserPathFromClient(jsonString(hm, "$.path"));
        if (!pathInsideFileBrowser(path) || !fs::exists(path))
            return replyError(c, 404, "file not found");
        if (path.lexically_normal() == fileBrowserRoot().lexically_normal())
            return replyError(c, 400, "cannot delete root");
        std::error_code ec;
        std::uintmax_t removed = fs::is_directory(path, ec) ? fs::remove_all(path, ec) : (fs::remove(path, ec) ? 1 : 0);
        if (ec)
            return replyError(c, 500, "delete failed");
        return replyJson(c, 200, {{"ok", true}, {"removed", removed}});
    }

    replyError(c, 404, "files api not found");
}

void ApiRouter::serveStatic(mg_connection* c, mg_http_message* hm)
{
    mg_http_serve_opts opts = {};
    std::string root = htmlRoot();
    opts.root_dir = root.c_str();
    opts.extra_headers = CORS_HEADERS;
    mg_http_serve_dir(c, hm, &opts);
}

} // namespace beiklive::network
