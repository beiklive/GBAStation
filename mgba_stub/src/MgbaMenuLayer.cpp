#include "mgba_stub/MgbaMenuLayer.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <switch.h>

#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"
#include "stb/stb_image.h"
#include "mgba_stub/MgbaShaderCatalog.hpp"
#include "mgba_stub/StubLog.hpp"
#include "mgba_stub/ui/UiComponents.hpp"

namespace beiklive::mgba_stub {

using namespace ui;

namespace {

constexpr float kPanelAnimationMs = 220.0f;
constexpr float kSidebarAnimationMs = 180.0f;
constexpr float kSelectorInitialDelayMs = 320.0f;
constexpr float kNavInitialDelayMs = 280.0f;
constexpr float kSaveCardH = 112.0f;
constexpr float kSaveCardGapY = 16.0f;
constexpr float kSettingRowH = 50.0f;
constexpr float kSettingStepY = 58.0f;
constexpr float kCheatRowH = 50.0f;
constexpr float kCheatStepY = 58.0f;
constexpr float kFilePickerRowH = 84.0f;
constexpr float kFilePickerTopH = 96.0f;
constexpr float kFilePickerFooterH = 96.0f;
constexpr float kFilePickerBodyPad = 18.0f;

constexpr float kFastForwardValues[] = {
    0.1f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f
};
constexpr float kFastForwardMin = kFastForwardValues[0];
constexpr float kFastForwardMax = kFastForwardValues[std::size(kFastForwardValues) - 1];
constexpr float kCustomScaleDefault = 1.0f;
constexpr float kCustomScaleStep = 0.1f;
constexpr float kCustomScaleMin = 1.0f;
constexpr float kCustomScaleMax = 15.0f;
constexpr float kCustomOffsetDefault = 0.0f;
constexpr float kCustomOffsetStep = 1.0f;
constexpr float kCustomOffsetMin = -1024.0f;
constexpr float kCustomOffsetMax = 1024.0f;
constexpr int kCustomLayoutControlCount = 3;
constexpr int kOverlayControlCount = 2;
constexpr int kShaderBaseControlCount = 2;
constexpr float kShaderParamRowH = 50.0f;
constexpr float kShaderParamStepY = 58.0f;
constexpr float kShaderListRowH = 58.0f;
constexpr float kShaderListPanelMaxH = 610.0f;
constexpr float kShaderListPanelVerticalMargin = 80.0f;
constexpr float kShaderListHeaderH = 78.0f;
constexpr float kShaderListFooterH = 54.0f;
constexpr float kShaderListPadTop = 30.0f;
constexpr float kShaderListPadBottom = 14.0f;
constexpr const char* kOverlayRoot = "sdmc:/GBAStation/overlays";
constexpr int kDisplayRowCustomLayout = 4;
constexpr int kDisplayRowOverlay = 5;
constexpr int kDisplayRowShader = 6;
constexpr int kDisplayRowSyncDisplay = 7;
constexpr int kDisplayRowSyncOverlay = 8;
constexpr int kDisplayRowSyncShader = 9;
constexpr int kDisplayRowCount = 10;
constexpr int kCustomLayoutMode = 5;

bool isDirectionUp(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyUp) != 0;
}

bool isDirectionDown(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyDown) != 0;
}

bool isDirectionLeft(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyLeft) != 0;
}

bool isDirectionRight(std::uint64_t buttons)
{
    return (buttons & HidNpadButton_AnyRight) != 0;
}

int navDirectionFromButtons(std::uint64_t buttons)
{
    if (isDirectionUp(buttons))
        return 1;
    if (isDirectionDown(buttons))
        return 2;
    if (isDirectionLeft(buttons))
        return 3;
    if (isDirectionRight(buttons))
        return 4;
    return 0;
}

std::uint64_t buttonsFromNavDirection(int direction)
{
    switch (direction)
    {
    case 1: return HidNpadButton_AnyUp;
    case 2: return HidNpadButton_AnyDown;
    case 3: return HidNpadButton_AnyLeft;
    case 4: return HidNpadButton_AnyRight;
    default: return 0;
    }
}

float focusedScroll(float focusedTop, float focusedH, float contentH)
{
    const float bodyH = contentBodyHeight();
    const float maxScroll = std::max(0.0f, contentH - bodyH);
    float scroll = 0.0f;
    if (focusedTop + focusedH > bodyH)
        scroll = focusedTop + focusedH - bodyH;
    if (focusedTop < scroll)
        scroll = focusedTop;
    return std::clamp(scroll, 0.0f, maxScroll);
}

float centeredFocusedScroll(float focusedTop, float focusedH, float contentH, float bodyH)
{
    const float maxScroll = std::max(0.0f, contentH - bodyH);
    const float focusedCenter = focusedTop + focusedH * 0.5f;
    return std::clamp(focusedCenter - bodyH * 0.5f, 0.0f, maxScroll);
}

bool eMgbaWithNoCase(const std::string& value, const char* suffix)
{
    const size_t suffixLen = std::strlen(suffix);
    if (value.size() < suffixLen)
        return false;

    const size_t offset = value.size() - suffixLen;
    for (size_t i = 0; i < suffixLen; ++i)
    {
        if (std::tolower(static_cast<unsigned char>(value[offset + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i])))
            return false;
    }
    return true;
}

std::string parentPathOrRoot(const std::string& path)
{
    if (!path.empty())
    {
        std::filesystem::path p(path);
        std::error_code ec;
        if (std::filesystem::is_directory(p, ec))
            return p.string();
        const std::string parent = p.parent_path().string();
        if (!parent.empty())
            return parent;
    }
    return kOverlayRoot;
}

std::string formatFileTime(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto ftime = std::filesystem::last_write_time(path, ec);
    if (ec)
        return {};
    const auto sysTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    const std::time_t tt = std::chrono::system_clock::to_time_t(sysTime);
    std::tm* tm = std::localtime(&tt);
    if (!tm)
        return {};
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm);
    return buffer;
}

float filePickerTargetScroll(int focus, int count)
{
    const float bodyH = menuMetrics().screenH - kFilePickerTopH - kFilePickerFooterH - kFilePickerBodyPad * 2.0f;
    const float contentH = static_cast<float>(std::max(0, count)) * kFilePickerRowH;
    const float focusedTop = static_cast<float>(std::max(0, focus)) * kFilePickerRowH;
    return centeredFocusedScroll(focusedTop, kFilePickerRowH, contentH, bodyH);
}

bool loadPngTextureFromFile(const std::string& path, std::uint32_t& texture, int& width, int& height)
{
    texture = 0;
    width = 0;
    height = 0;
    if (path.empty() || !eMgbaWithNoCase(path, ".png"))
        return false;

    auto makePathCandidates = [](const std::string& source) {
        std::vector<std::string> paths;
        paths.push_back(source);
        if (source.rfind("sdmc:", 0) == 0)
            paths.push_back(source.substr(5));
        else if (!source.empty() && source[0] == '/')
            paths.push_back("sdmc:" + source);
        return paths;
    };

    FILE* fp = nullptr;
    std::string openedPath;
    for (const auto& candidate : makePathCandidates(path))
    {
        appendStubLog("GBAStationMgbaStub: png preview open try path=%s", candidate.c_str());
        fp = std::fopen(candidate.c_str(), "rb");
        if (fp)
        {
            openedPath = candidate;
            break;
        }
    }
    if (!fp)
    {
        appendStubLog("GBAStationMgbaStub: png preview open failed path=%s", path.c_str());
        return false;
    }

    if (std::fseek(fp, 0, SEEK_END) != 0)
    {
        std::fclose(fp);
        appendStubLog("GBAStationMgbaStub: png preview seek end failed path=%s", openedPath.c_str());
        return false;
    }
    const long fileSize = std::ftell(fp);
    if (fileSize <= 0 || fileSize > 64 * 1024 * 1024)
    {
        std::fclose(fp);
        appendStubLog("GBAStationMgbaStub: png preview size invalid path=%s bytes=%ld", openedPath.c_str(), fileSize);
        return false;
    }
    std::rewind(fp);
    std::vector<unsigned char> bytes(static_cast<std::size_t>(fileSize));
    const std::size_t readBytes = std::fread(bytes.data(), 1, bytes.size(), fp);
    std::fclose(fp);
    if (readBytes != bytes.size())
    {
        appendStubLog("GBAStationMgbaStub: png preview read failed path=%s read=%u expected=%u",
                      openedPath.c_str(),
                      static_cast<unsigned>(readBytes),
                      static_cast<unsigned>(bytes.size()));
        return false;
    }
    appendStubLog("GBAStationMgbaStub: png preview read ok path=%s bytes=%u",
                  openedPath.c_str(),
                  static_cast<unsigned>(bytes.size()));

    auto readBe32 = [](const unsigned char* p) {
        return (static_cast<std::uint32_t>(p[0]) << 24) |
               (static_cast<std::uint32_t>(p[1]) << 16) |
               (static_cast<std::uint32_t>(p[2]) << 8) |
               static_cast<std::uint32_t>(p[3]);
    };
    if (bytes.size() >= 33 &&
        bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N' && bytes[3] == 'G' &&
        bytes[12] == 'I' && bytes[13] == 'H' && bytes[14] == 'D' && bytes[15] == 'R')
    {
        const std::uint32_t headerW = readBe32(bytes.data() + 16);
        const std::uint32_t headerH = readBe32(bytes.data() + 20);
        const unsigned bitDepth = bytes[24];
        const unsigned colorType = bytes[25];
        appendStubLog("GBAStationMgbaStub: png preview header size=%ux%u bit=%u color=%u path=%s",
                      headerW,
                      headerH,
                      bitDepth,
                      colorType,
                      openedPath.c_str());
    }
    else
    {
        appendStubLog("GBAStationMgbaStub: png preview header invalid path=%s", openedPath.c_str());
        return false;
    }

    int comp = 0;
    appendStubLog("GBAStationMgbaStub: png preview decode file begin path=%s", openedPath.c_str());
    unsigned char* pixels = stbi_load(openedPath.c_str(),
                                      &width,
                                      &height,
                                      &comp,
                                      4);
    if (!pixels || width <= 0 || height <= 0 || width > 4096 || height > 4096)
    {
        if (pixels)
            stbi_image_free(pixels);
        appendStubLog("GBAStationMgbaStub: png preview decode failed path=%s width=%d height=%d reason=%s",
                      openedPath.c_str(),
                      width,
                      height,
                      stbi_failure_reason() ? stbi_failure_reason() : "(null)");
        width = 0;
        height = 0;
        return false;
    }
    appendStubLog("GBAStationMgbaStub: png preview decode ok path=%s size=%dx%d comp=%d",
                  openedPath.c_str(),
                  width,
                  height,
                  comp);

    std::vector<unsigned char> scaled;
    const int decodedW = width;
    const int decodedH = height;
    constexpr int kMaxUploadPixels = kScreenWidth * kScreenHeight;
    if (static_cast<long long>(width) * static_cast<long long>(height) > kMaxUploadPixels)
    {
        const float scale = std::sqrt(static_cast<float>(kMaxUploadPixels) /
                                      static_cast<float>(width * height));
        const int outW = std::max(1, static_cast<int>(std::floor(width * scale)));
        const int outH = std::max(1, static_cast<int>(std::floor(height * scale)));
        scaled.resize(static_cast<std::size_t>(outW) * outH * 4);
        for (int y = 0; y < outH; ++y)
        {
            const int sy = std::min(height - 1, static_cast<int>((static_cast<long long>(y) * height) / outH));
            for (int x = 0; x < outW; ++x)
            {
                const int sx = std::min(width - 1, static_cast<int>((static_cast<long long>(x) * width) / outW));
                const unsigned char* src = pixels + (static_cast<std::size_t>(sy) * width + sx) * 4;
                unsigned char* dst = scaled.data() + (static_cast<std::size_t>(y) * outW + x) * 4;
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst[3] = src[3];
            }
        }
        width = outW;
        height = outH;
        appendStubLog("GBAStationMgbaStub: png preview downscale %dx%d -> %dx%d uploadBytes=%u",
                      decodedW,
                      decodedH,
                      width,
                      height,
                      static_cast<unsigned>(scaled.size()));
    }

    const unsigned char* uploadPixels = scaled.empty() ? pixels : scaled.data();
    const std::size_t uploadBytes = static_cast<std::size_t>(width) * height * 4;
    if (uploadBytes > 7 * 1024 * 1024)
    {
        stbi_image_free(pixels);
        appendStubLog("GBAStationMgbaStub: png preview upload rejected path=%s uploadBytes=%u",
                      openedPath.c_str(),
                      static_cast<unsigned>(uploadBytes));
        width = 0;
        height = 0;
        return false;
    }

    appendStubLog("GBAStationMgbaStub: png preview texture create begin size=%dx%d uploadBytes=%u",
                  width,
                  height,
                  static_cast<unsigned>(uploadBytes));
    texture = Gfx::TextureCreate(static_cast<u32>(width),
                                 static_cast<u32>(height),
                                 DkImageFormat_RGBA8_Unorm);
    appendStubLog("GBAStationMgbaStub: png preview texture create ok tex=%u", texture);
    appendStubLog("GBAStationMgbaStub: png preview texture upload begin tex=%u stride=%u",
                  texture,
                  static_cast<unsigned>(width * 4));
    Gfx::TextureUpload(texture,
                       0,
                       0,
                       static_cast<u32>(width),
                       static_cast<u32>(height),
                       const_cast<unsigned char*>(uploadPixels),
                       static_cast<u32>(width * 4));
    appendStubLog("GBAStationMgbaStub: png preview texture upload queued tex=%u", texture);
    stbi_image_free(pixels);
    return texture != 0;
}

float displayRowY(int row)
{
    switch (row)
    {
    case 0: return 0.0f;
    case 1: return kSettingStepY;
    case 2: return kSettingStepY * 2.0f;
    case 3: return kSettingStepY * 3.0f;
    case 4: return kSettingStepY * 4.0f;
    case 5: return kSettingStepY * 5.0f + 43.0f;
    case 6: return kSettingStepY * 6.0f + 43.0f;
    case 7: return kSettingStepY * 7.0f + 86.0f;
    case 8: return kSettingStepY * 8.0f + 86.0f;
    case 9: return kSettingStepY * 9.0f + 86.0f;
    default: return 0.0f;
    }
}

float stepFloatValue(float value, int direction, float step, float minValue, float maxValue)
{
    if (direction == 0 || step <= 0.0f)
        return std::clamp(value, minValue, maxValue);
    return std::clamp(value + static_cast<float>(direction) * step, minValue, maxValue);
}

bool pushMenuOrientationTransform(int orientation)
{
    orientation = std::clamp(orientation, 0, 3);
    if (orientation == 0)
        return false;

    if (orientation == 1)
    {
        Gfx::PushDrawTransform(0.0f, -1.0f, 1280.0f, 1.0f, 0.0f, 0.0f);
        return true;
    }
    if (orientation == 2)
    {
        Gfx::PushDrawTransform(-1.0f, 0.0f, 1280.0f, 0.0f, -1.0f, 720.0f);
        return true;
    }

    Gfx::PushDrawTransform(0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 720.0f);
    return true;
}

} // namespace

MgbaMenuLayer::~MgbaMenuLayer()
{
    releaseStateSlotTextures();
    releaseStatePreviewTexture();
    releaseFilePickerPreview();
}

void MgbaMenuLayer::setStateSlots(const std::array<MgbaStateSlotInfo, 10>& slots)
{
    releaseStateSlotTextures();
    m_slots = slots;
    releaseStatePreviewTexture();
}

void MgbaMenuLayer::setCheatItems(const std::vector<MgbaCheatItem>& cheats)
{
    m_cheats = cheats;
    for (auto& cheat : m_cheats)
    {
        if (cheat.type == MgbaCheatItem::Type::Category)
            cheat.expanded = false;
    }
    invalidateVisibleCheatCache();
    if (static_cast<Item>(m_selected) == Item::Cheats)
        m_contentFocus = std::clamp(m_contentFocus, 0, std::max(0, contentControlCount(Item::Cheats) - 1));
    resetContentScroll();
}

bool MgbaMenuLayer::consumeCheatSettingsDirty()
{
    const bool dirty = m_cheatSettingsDirty;
    m_cheatSettingsDirty = false;
    return dirty;
}

void MgbaMenuLayer::releaseFilePickerPreview()
{
    m_filePickerImagePreviewVisible = false;
    if (m_filePickerPreviewTexture != 0)
    {
        Gfx::PresentQueue.waitIdle();
        Gfx::TextureDelete(m_filePickerPreviewTexture);
        m_filePickerPreviewTexture = 0;
    }
    m_filePickerPreviewWidth = 0;
    m_filePickerPreviewHeight = 0;
    m_filePickerPreviewPath.clear();
    m_filePickerPreviewAttempted = false;
}

void MgbaMenuLayer::releaseStatePreviewTexture() const
{
    if (m_statePreviewTexture != 0)
    {
        Gfx::PresentQueue.waitIdle();
        Gfx::TextureDelete(m_statePreviewTexture);
        m_statePreviewTexture = 0;
    }
    m_statePreviewWidth = 0;
    m_statePreviewHeight = 0;
    m_statePreviewSlot = -1;
    m_statePreviewPath.clear();
    m_statePreviewAttempted = false;
}

void MgbaMenuLayer::releaseStateSlotTextures() const
{
    bool wait = false;
    for (auto& slot : m_slots)
    {
        if (slot.thumbnailTexture != 0)
        {
            wait = true;
            break;
        }
    }
    if (wait)
        Gfx::PresentQueue.waitIdle();

    for (auto& slot : m_slots)
    {
        if (slot.thumbnailTexture != 0)
        {
            Gfx::TextureDelete(slot.thumbnailTexture);
            slot.thumbnailTexture = 0;
        }
        slot.thumbnailWidth = 0;
        slot.thumbnailHeight = 0;
        slot.thumbnailLoadAttempted = false;
    }
}

void MgbaMenuLayer::ensureStatePreviewTexture() const
{
    const Item item = static_cast<Item>(m_selected);
    if (!m_visible ||
        (item != Item::SaveState && item != Item::LoadState) ||
        m_contentFocus < 0 ||
        m_contentFocus >= static_cast<int>(m_slots.size()))
    {
        releaseStatePreviewTexture();
        return;
    }

    const auto& slot = m_slots[m_contentFocus];
    const bool usable = slot.exists && slot.thumbnailAvailable && !slot.thumbnailPath.empty();
    if (!usable)
    {
        if (m_statePreviewTexture == 0 &&
            m_statePreviewAttempted &&
            m_statePreviewSlot == m_contentFocus &&
            m_statePreviewPath == slot.thumbnailPath)
            return;
        releaseStatePreviewTexture();
        m_statePreviewSlot = m_contentFocus;
        m_statePreviewPath = slot.thumbnailPath;
        m_statePreviewAttempted = true;
        return;
    }

    if (m_statePreviewTexture != 0 &&
        m_statePreviewSlot == m_contentFocus &&
        m_statePreviewPath == slot.thumbnailPath)
        return;
    if (m_statePreviewTexture == 0 &&
        m_statePreviewAttempted &&
        m_statePreviewSlot == m_contentFocus &&
        m_statePreviewPath == slot.thumbnailPath)
        return;

    releaseStatePreviewTexture();
    m_statePreviewSlot = m_contentFocus;
    m_statePreviewPath = slot.thumbnailPath;
    m_statePreviewAttempted = true;
    appendStubLog("GBAStationMgbaStub: state preview load begin slot=%d path=%s",
                  m_contentFocus,
                  slot.thumbnailPath.c_str());
    if (!loadPngTextureFromFile(slot.thumbnailPath,
                                m_statePreviewTexture,
                                m_statePreviewWidth,
                                m_statePreviewHeight))
    {
        appendStubLog("GBAStationMgbaStub: state preview load failed slot=%d path=%s",
                      m_contentFocus,
                      slot.thumbnailPath.c_str());
        releaseStatePreviewTexture();
        m_statePreviewSlot = m_contentFocus;
        m_statePreviewPath = slot.thumbnailPath;
        m_statePreviewAttempted = true;
        return;
    }

    appendStubLog("GBAStationMgbaStub: state preview load ok slot=%d size=%dx%d tex=%u",
                  m_contentFocus,
                  m_statePreviewWidth,
                  m_statePreviewHeight,
                  m_statePreviewTexture);
}

void MgbaMenuLayer::ensureStateSlotTextures() const
{
    const Item item = static_cast<Item>(m_selected);
    if (!m_visible || (item != Item::SaveState && item != Item::LoadState))
        return;

    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i)
    {
        auto& slot = m_slots[i];
        if (!slot.exists ||
            slot.thumbnailTexture != 0 ||
            slot.thumbnailLoadAttempted ||
            slot.thumbnailPath.empty() ||
            !std::filesystem::exists(slot.thumbnailPath))
        {
            continue;
        }

        slot.thumbnailLoadAttempted = true;
        if (!loadPngTextureFromFile(slot.thumbnailPath,
                                    slot.thumbnailTexture,
                                    slot.thumbnailWidth,
                                    slot.thumbnailHeight))
        {
            appendStubLog("GBAStationMgbaStub: state grid thumbnail load failed slot=%d path=%s",
                          i,
                          slot.thumbnailPath.c_str());
            slot.thumbnailTexture = 0;
            slot.thumbnailWidth = 0;
            slot.thumbnailHeight = 0;
        }
    }
}

void MgbaMenuLayer::reloadFilePickerEntries(const std::string& directory, const std::string& focusPath)
{
    releaseFilePickerPreview();
    m_filePickerEntries.clear();
    m_filePickerDirectory = directory.empty() ? kOverlayRoot : directory;
    m_filePickerFocus = 0;
    m_filePickerScrollY = 0.0f;
    m_filePickerScrollLastTick = 0;
    m_filePickerImagePreviewVisible = false;

    std::error_code ec;
    if (!std::filesystem::exists(m_filePickerDirectory, ec))
        std::filesystem::create_directories(m_filePickerDirectory, ec);

    if (std::filesystem::path(m_filePickerDirectory).has_parent_path())
    {
        MgbaFilePickerEntry up {};
        up.name = "..";
        up.path = std::filesystem::path(m_filePickerDirectory).parent_path().string();
        up.isDirectory = true;
        m_filePickerEntries.push_back(up);
    }

    std::vector<MgbaFilePickerEntry> dirs;
    std::vector<MgbaFilePickerEntry> files;
    for (const auto& entry : std::filesystem::directory_iterator(m_filePickerDirectory, ec))
    {
        if (ec)
            break;
        MgbaFilePickerEntry item {};
        item.path = entry.path().string();
        item.name = entry.path().filename().string();
        item.isDirectory = entry.is_directory(ec);
        item.modifiedTime = formatFileTime(entry.path());
        if (!item.isDirectory)
        {
            if (!eMgbaWithNoCase(item.name, ".png"))
                continue;
            item.size = entry.file_size(ec);
        }
        if (item.name.empty())
            continue;
        (item.isDirectory ? dirs : files).push_back(item);
    }

    auto sortByName = [](const MgbaFilePickerEntry& a, const MgbaFilePickerEntry& b) {
        return a.name < b.name;
    };
    std::sort(dirs.begin(), dirs.end(), sortByName);
    std::sort(files.begin(), files.end(), sortByName);
    m_filePickerEntries.insert(m_filePickerEntries.end(), dirs.begin(), dirs.end());
    m_filePickerEntries.insert(m_filePickerEntries.end(), files.begin(), files.end());

    const std::string normalizedFocus = focusPath.empty() ? std::string{} : std::filesystem::path(focusPath).lexically_normal().string();
    if (!normalizedFocus.empty())
    {
        for (int i = 0; i < static_cast<int>(m_filePickerEntries.size()); ++i)
        {
            if (std::filesystem::path(m_filePickerEntries[i].path).lexically_normal().string() == normalizedFocus)
            {
                m_filePickerFocus = i;
                break;
            }
        }
    }
    appendStubLog("GBAStationMgbaStub: file picker entries dir=%s count=%d focus=%d",
                  m_filePickerDirectory.c_str(),
                  static_cast<int>(m_filePickerEntries.size()),
                  m_filePickerFocus);
}

void MgbaMenuLayer::ensureFilePickerPreview()
{
    if (!m_filePickerVisible ||
        m_filePickerFocus < 0 ||
        m_filePickerFocus >= static_cast<int>(m_filePickerEntries.size()))
        return;

    const auto& entry = m_filePickerEntries[m_filePickerFocus];
    if (entry.isDirectory || !eMgbaWithNoCase(entry.path, ".png"))
    {
        releaseFilePickerPreview();
        return;
    }

    releaseFilePickerPreview();
    m_filePickerPreviewPath = entry.path;
    m_filePickerPreviewAttempted = true;
    appendStubLog("GBAStationMgbaStub: file picker preview load begin path=%s", entry.path.c_str());
    if (loadPngTextureFromFile(entry.path,
                               m_filePickerPreviewTexture,
                               m_filePickerPreviewWidth,
                               m_filePickerPreviewHeight))
    {
        appendStubLog("GBAStationMgbaStub: file picker preview load ok size=%dx%d",
                      m_filePickerPreviewWidth,
                      m_filePickerPreviewHeight);
        m_filePickerImagePreviewVisible = true;
    }
    else
    {
        appendStubLog("GBAStationMgbaStub: file picker preview load failed path=%s", entry.path.c_str());
        m_filePickerImagePreviewVisible = false;
    }
}

void MgbaMenuLayer::beginSelectionAnimation(int oldSelected, int newSelected)
{
    if (oldSelected == newSelected)
        return;

    m_previousSelected = oldSelected;
    m_selected = newSelected;
    m_selectionAnimStartTick = armGetSystemTick();
    m_selectionAnimating = true;
    m_focusScope = FocusScope::Tabs;
    m_contentFocus = 0;
    resetContentScroll();
}

void MgbaMenuLayer::beginPanelAnimation(bool opening)
{
    m_panelOpening = opening;
    m_panelAnimating = true;
    m_panelAnimStartTick = armGetSystemTick();
}

void MgbaMenuLayer::open()
{
    if (m_visible && !m_panelAnimating)
        return;
    releaseStatePreviewTexture();
    m_customLayoutEditorVisible = false;
    m_customLayoutEditorClosing = false;
    m_customLayoutReturnToMenu = false;
    m_overlaySidebarVisible = false;
    m_overlaySidebarClosing = false;
    m_overlaySidebarReturnToMenu = false;
    m_shaderSidebarVisible = false;
    m_shaderSidebarClosing = false;
    m_shaderSidebarReturnToMenu = false;
    m_filePickerVisible = false;
    m_filePickerClosing = false;
    m_filePickerReturnToOverlay = false;
    closeSyncConfirmDialog();
    releaseFilePickerPreview();
    m_visible = true;
    m_focusScope = FocusScope::Tabs;
    m_contentFocus = 0;
    resetContentScroll();
    m_previousSelected = m_selected;
    m_selectionAnimStartTick = armGetSystemTick();
    m_selectionAnimating = true;
    beginPanelAnimation(true);
}

void MgbaMenuLayer::close()
{
    if (!m_visible && !m_panelAnimating && !m_customLayoutEditorVisible &&
        !m_overlaySidebarVisible && !m_shaderSidebarVisible && !m_filePickerVisible)
        return;
    releaseStatePreviewTexture();
    if (m_filePickerVisible && !m_filePickerClosing)
    {
        m_filePickerClosing = true;
        m_filePickerReturnToOverlay = false;
        m_filePickerAnimStartTick = armGetSystemTick();
    }
    if (m_customLayoutEditorVisible && !m_customLayoutEditorClosing)
    {
        m_customLayoutEditorClosing = true;
        m_customLayoutReturnToMenu = false;
        m_customLayoutAnimStartTick = armGetSystemTick();
    }
    if (m_overlaySidebarVisible && !m_overlaySidebarClosing)
    {
        m_overlaySidebarClosing = true;
        m_overlaySidebarReturnToMenu = false;
        m_overlaySidebarAnimStartTick = armGetSystemTick();
    }
    if (m_shaderSidebarVisible && !m_shaderSidebarClosing)
    {
        m_shaderSidebarClosing = true;
        m_shaderSidebarReturnToMenu = false;
        m_shaderSidebarAnimStartTick = armGetSystemTick();
    }
    m_visible = false;
    m_focusScope = FocusScope::Tabs;
    m_contentFocus = 0;
    closeSyncConfirmDialog();
    resetContentScroll();
    beginPanelAnimation(false);
}

void MgbaMenuLayer::toggle()
{
    if (m_customLayoutEditorVisible || m_overlaySidebarVisible || m_shaderSidebarVisible ||
        m_filePickerVisible || m_visible)
        close();
    else
        open();
}

bool MgbaMenuLayer::active() const
{
    if (m_visible || m_customLayoutEditorVisible || m_overlaySidebarVisible ||
        m_shaderSidebarVisible || m_filePickerVisible)
        return true;
    return m_panelAnimating && animationProgress(m_panelAnimStartTick, kPanelAnimationMs) < 1.0f;
}

void MgbaMenuLayer::showToast(const std::string& message)
{
    m_toastMessage = message;
    m_toastStartTick = armGetSystemTick();
}

void MgbaMenuLayer::clearToast()
{
    m_toastMessage.clear();
    m_toastStartTick = 0;
}

std::vector<MgbaMenuSound> MgbaMenuLayer::consumeSounds()
{
    std::vector<MgbaMenuSound> souMgba;
    souMgba.swap(m_pendingSounds);
    return souMgba;
}

void MgbaMenuLayer::queueSound(MgbaMenuSound sound)
{
    m_pendingSounds.push_back(sound);
}

void MgbaMenuLayer::reopenDisplayContent(int focusedRow)
{
    open();
    m_selected = itemIndex(Item::Display);
    m_previousSelected = m_selected;
    m_selectionAnimating = false;
    m_focusScope = FocusScope::Content;
    m_contentFocus = std::clamp(focusedRow, 0, std::max(0, contentControlCount(Item::Display) - 1));
    resetContentScroll();
}

float MgbaMenuLayer::panelProgress() const
{
    if (!m_panelAnimating)
        return m_visible ? 1.0f : 0.0f;

    const float progress = easeOutCubic(animationProgress(m_panelAnimStartTick, kPanelAnimationMs));
    return m_panelOpening ? progress : 1.0f - progress;
}

float MgbaMenuLayer::customLayoutEditorProgress() const
{
    if (!m_customLayoutEditorVisible)
        return 0.0f;

    const float progress = easeOutCubic(animationProgress(m_customLayoutAnimStartTick, kSidebarAnimationMs));
    return m_customLayoutEditorClosing ? 1.0f - progress : progress;
}

void MgbaMenuLayer::resetContentScroll()
{
    m_contentScrollY = 0.0f;
    m_contentScrollLastTick = 0;
}

float MgbaMenuLayer::targetContentScrollY() const
{
    setMenuMetricsOrientation(0);
    const Item item = static_cast<Item>(m_selected);
    switch (item)
    {
    case Item::SaveState:
    case Item::LoadState:
        return 0.0f;
    case Item::Display:
    {
        const int row = std::clamp(m_contentFocus, 0, contentControlCount(Item::Display) - 1);
        const float contentH = displayRowY(kDisplayRowCount - 1) + kSettingRowH;
        return focusedScroll(displayRowY(row), kSettingRowH, contentH);
    }
    case Item::Cheats:
    {
        const int count = contentControlCount(Item::Cheats);
        const int row = std::clamp(m_contentFocus, 0, std::max(0, count - 1));
        const float contentH = static_cast<float>(count) * kCheatStepY;
        return centeredFocusedScroll(static_cast<float>(row) * kCheatStepY,
                                     kCheatRowH,
                                     contentH,
                                     contentBodyHeight());
    }
    default:
        return 0.0f;
    }
}

float MgbaMenuLayer::smoothedContentScrollY() const
{
    const float target = targetContentScrollY();
    const std::uint64_t now = armGetSystemTick();
    if (m_contentScrollLastTick == 0)
    {
        m_contentScrollLastTick = now;
        m_contentScrollY = target;
        return m_contentScrollY;
    }

    const float dtMs = static_cast<float>(armTicksToNs(now - m_contentScrollLastTick)) / 1000000.0f;
    m_contentScrollLastTick = now;
    const float t = 1.0f - std::exp(-dtMs / 72.0f);
    m_contentScrollY += (target - m_contentScrollY) * std::clamp(t, 0.0f, 1.0f);
    if (std::fabs(target - m_contentScrollY) < 0.5f)
        m_contentScrollY = target;
    return m_contentScrollY;
}

int MgbaMenuLayer::shaderControlCount() const
{
    return kShaderBaseControlCount + static_cast<int>(m_display.shaderParams.size());
}

float MgbaMenuLayer::shaderParamTargetScroll() const
{
    if (m_display.shaderParams.empty() || m_shaderSidebarFocus < kShaderBaseControlCount)
        return 0.0f;

    setMenuMetricsOrientation(0);
    const bool portrait = menuMetrics().screenH > menuMetrics().screenW;
    const float sectionY = portrait ? 158.0f : 122.0f;
    const float rowY = sectionY + 44.0f;
    const float rowGap = 67.0f;
    const float paramSectionY = rowY + rowGap * 2.0f + 28.0f;
    const float paramListY = paramSectionY + 42.0f;
    const float bodyH = std::max(1.0f, menuMetrics().screenH - paramListY - 28.0f);
    const int paramIndex = std::clamp(m_shaderSidebarFocus - kShaderBaseControlCount,
                                      0,
                                      static_cast<int>(m_display.shaderParams.size()) - 1);
    const float contentH = static_cast<float>(m_display.shaderParams.size()) * kShaderParamStepY;
    const float focusedTop = static_cast<float>(paramIndex) * kShaderParamStepY;
    const float maxScroll = std::max(0.0f, contentH - bodyH);
    float scroll = m_shaderParamScrollY;
    if (focusedTop + kShaderParamRowH > scroll + bodyH)
        scroll = focusedTop + kShaderParamRowH - bodyH;
    if (focusedTop < scroll)
        scroll = focusedTop;
    return std::clamp(scroll, 0.0f, maxScroll);
}

float MgbaMenuLayer::smoothedShaderParamScroll() const
{
    const float target = shaderParamTargetScroll();
    const std::uint64_t now = armGetSystemTick();
    if (m_shaderParamScrollLastTick == 0)
    {
        m_shaderParamScrollLastTick = now;
        m_shaderParamScrollY = target;
        return m_shaderParamScrollY;
    }

    const float dtMs = static_cast<float>(armTicksToNs(now - m_shaderParamScrollLastTick)) / 1000000.0f;
    m_shaderParamScrollLastTick = now;
    const float t = 1.0f - std::exp(-dtMs / 72.0f);
    m_shaderParamScrollY += (target - m_shaderParamScrollY) * std::clamp(t, 0.0f, 1.0f);
    if (std::fabs(target - m_shaderParamScrollY) < 0.5f)
        m_shaderParamScrollY = target;
    return m_shaderParamScrollY;
}

void MgbaMenuLayer::resetShaderParamScroll()
{
    m_shaderParamScrollY = 0.0f;
    m_shaderParamScrollLastTick = 0;
}

int MgbaMenuLayer::currentShaderTypeIndex() const
{
    const auto entries = MgbaShaderListEntries(m_shaderListPath);
    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        if (entries[i].kind == MgbaShaderListEntry::Kind::Shader &&
            m_display.mgbaShaderType == entries[i].shaderType)
        {
            return i;
        }
    }
    return 0;
}

float MgbaMenuLayer::shaderListTargetScroll() const
{
    setMenuMetricsOrientation(0);
    const float panelH = std::min(menuMetrics().screenH - kShaderListPanelVerticalMargin,
                                  kShaderListPanelMaxH);
    const float bodyH = std::max(1.0f,
                                 panelH - kShaderListHeaderH - kShaderListFooterH -
                                     kShaderListPadTop - kShaderListPadBottom);
    const auto entries = MgbaShaderListEntries(m_shaderListPath);
    const int entryCount = std::max(1, static_cast<int>(entries.size()));
    const float contentH = static_cast<float>(entryCount) * kShaderListRowH;
    const float focusedTop = static_cast<float>(std::clamp(m_shaderListFocus,
                                                           0,
                                                           entryCount - 1)) * kShaderListRowH;
    return centeredFocusedScroll(focusedTop, kShaderListRowH, contentH, bodyH);
}

float MgbaMenuLayer::smoothedShaderListScroll() const
{
    const float target = shaderListTargetScroll();
    const std::uint64_t now = armGetSystemTick();
    if (m_shaderListScrollLastTick == 0)
    {
        m_shaderListScrollLastTick = now;
        m_shaderListScrollY = target;
        return m_shaderListScrollY;
    }

    const float dtMs = static_cast<float>(armTicksToNs(now - m_shaderListScrollLastTick)) / 1000000.0f;
    m_shaderListScrollLastTick = now;
    const float t = 1.0f - std::exp(-dtMs / 72.0f);
    m_shaderListScrollY += (target - m_shaderListScrollY) * std::clamp(t, 0.0f, 1.0f);
    if (std::fabs(target - m_shaderListScrollY) < 0.5f)
        m_shaderListScrollY = target;
    return m_shaderListScrollY;
}

void MgbaMenuLayer::resetShaderListScroll()
{
    m_shaderListScrollY = 0.0f;
    m_shaderListScrollLastTick = 0;
}

void MgbaMenuLayer::beginShaderList()
{
    m_shaderListVisible = true;
    m_shaderListPath = MgbaShaderListPathForType(m_display.mgbaShaderType);
    m_shaderListFocus = currentShaderTypeIndex();
    resetShaderListScroll();
    m_navDirection = 0;
}

void MgbaMenuLayer::closeShaderList()
{
    m_shaderListVisible = false;
    m_shaderListPath.clear();
    resetShaderListScroll();
    m_navDirection = 0;
}

void MgbaMenuLayer::setFastForwardMultiplier(float multiplier)
{
    m_display.fastForwardMultiplier = std::clamp(multiplier, kFastForwardMin, kFastForwardMax);
}

void MgbaMenuLayer::setCustomLayoutSettings(const MgbaCustomLayoutSettings& settings)
{
    m_display.customLayout = settings;
    m_display.customLayout.topScale = std::clamp(m_display.customLayout.topScale, kCustomScaleMin, kCustomScaleMax);
    m_display.customLayout.topOffsetX = std::clamp(m_display.customLayout.topOffsetX, kCustomOffsetMin, kCustomOffsetMax);
    m_display.customLayout.topOffsetY = std::clamp(m_display.customLayout.topOffsetY, kCustomOffsetMin, kCustomOffsetMax);
    m_display.customLayout.bottomScale = m_display.customLayout.topScale;
    m_display.customLayout.bottomOffsetX = m_display.customLayout.topOffsetX;
    m_display.customLayout.bottomOffsetY = m_display.customLayout.topOffsetY;
}

void MgbaMenuLayer::setDisplaySettings(const MgbaDisplaySettings& settings)
{
    m_display = settings;
    m_display.fastForwardMultiplier = std::clamp(m_display.fastForwardMultiplier, kFastForwardMin, kFastForwardMax);
    m_display.integerScaleMultiplier = std::clamp(m_display.integerScaleMultiplier, 0, 8);
    m_display.layout = std::clamp(m_display.layout, 0, 5);
    m_display.orientation = std::clamp(m_display.orientation, 0, 3);
    m_display.mgbaShaderType = normalizeMgbaShaderType(m_display.mgbaShaderType);
    if (m_shaderListVisible)
        m_shaderListPath = MgbaShaderListPathForType(m_display.mgbaShaderType);
    m_shaderListFocus = currentShaderTypeIndex();
    for (auto& param : m_display.shaderParams)
    {
        param.minValue = std::min(param.minValue, param.maxValue);
        param.step = std::max(0.0001f, param.step);
        param.value = std::clamp(param.value, param.minValue, param.maxValue);
    }
    m_shaderSidebarFocus = std::clamp(m_shaderSidebarFocus, 0, std::max(0, shaderControlCount() - 1));
    setCustomLayoutSettings(m_display.customLayout);
}

bool MgbaMenuLayer::itemHasContent(Item item) const
{
    return item == Item::SaveState || item == Item::LoadState ||
           item == Item::Cheats || item == Item::Display;
}

int MgbaMenuLayer::contentControlCount(Item item) const
{
    switch (item)
    {
    case Item::SaveState:
    case Item::LoadState:
        return 10;
    case Item::Display:
        return kDisplayRowCount;
    case Item::Cheats:
        return static_cast<int>(visibleCheatIndices().size());
    default:
        return 0;
    }
}

void MgbaMenuLayer::invalidateVisibleCheatCache()
{
    m_visibleCheatCacheDirty = true;
}

void MgbaMenuLayer::rebuildVisibleCheatCache() const
{
    if (!m_visibleCheatCacheDirty)
        return;

    m_visibleCheatCache.clear();
    m_visibleCheatCache.reserve(m_cheats.size());
    for (int i = 0; i < static_cast<int>(m_cheats.size()); ++i)
    {
        int parent = m_cheats[i].parent;
        bool visible = true;
        int guard = 0;
        while (parent >= 0 && parent < static_cast<int>(m_cheats.size()))
        {
            if (parent == i || ++guard > static_cast<int>(m_cheats.size()))
            {
                visible = false;
                break;
            }
            if (!m_cheats[parent].expanded)
            {
                visible = false;
                break;
            }
            parent = m_cheats[parent].parent;
        }
        if (visible)
            m_visibleCheatCache.push_back(i);
    }
    m_visibleCheatCacheDirty = false;
}

const std::vector<int>& MgbaMenuLayer::visibleCheatIndices() const
{
    rebuildVisibleCheatCache();
    return m_visibleCheatCache;
}

int MgbaMenuLayer::visibleCheatIndex(int visibleRow) const
{
    const auto& visible = visibleCheatIndices();
    if (visible.empty())
        return -1;
    visibleRow = std::clamp(visibleRow, 0, static_cast<int>(visible.size()) - 1);
    return visible[visibleRow];
}

int MgbaMenuLayer::nextFocusableDisplayRow(int from, int direction) const
{
    int row = from;
    for (int i = 0; i < contentControlCount(Item::Display); ++i)
    {
        row = (row + direction + contentControlCount(Item::Display)) % contentControlCount(Item::Display);
        if (row == kDisplayRowCustomLayout && m_display.layout != kCustomLayoutMode)
            continue;
        return row;
    }
    return from;
}

bool MgbaMenuLayer::activateDisplayControl()
{
    switch (m_contentFocus)
    {
    case 3:
        if (m_display.layout != 4)
            return false;
        return cycleCurrentSetting(1);
    case kDisplayRowCustomLayout:
        if (m_display.layout == kCustomLayoutMode)
            beginCustomLayoutEditor();
        return false;
    case kDisplayRowOverlay:
        beginOverlaySidebar();
        return false;
    case kDisplayRowShader:
        beginShaderSidebar();
        return false;
    case kDisplayRowSyncDisplay:
    case kDisplayRowSyncOverlay:
    case kDisplayRowSyncShader:
        return false;
    default:
        return false;
    }
}

bool MgbaMenuLayer::activateCheatControl()
{
    const int index = visibleCheatIndex(m_contentFocus);
    if (index < 0 || index >= static_cast<int>(m_cheats.size()))
        return false;

    auto& item = m_cheats[index];
    if (item.type == MgbaCheatItem::Type::Category)
    {
        item.expanded = !item.expanded;
        invalidateVisibleCheatCache();
        const int count = contentControlCount(Item::Cheats);
        m_contentFocus = std::clamp(m_contentFocus, 0, std::max(0, count - 1));
        resetContentScroll();
        return false;
    }

    item.enabled = !item.enabled;
    m_cheatSettingsDirty = true;
    return true;
}

void MgbaMenuLayer::beginCustomLayoutEditor()
{
    m_visible = false;
    m_panelAnimating = false;
    m_customLayoutEditorVisible = true;
    m_customLayoutEditorClosing = false;
    m_customLayoutReturnToMenu = false;
    m_customLayoutAnimStartTick = armGetSystemTick();
    m_customLayoutFocus = 0;
    resetContentScroll();
}

void MgbaMenuLayer::beginOverlaySidebar()
{
    m_visible = false;
    m_panelAnimating = false;
    m_overlaySidebarVisible = true;
    m_overlaySidebarClosing = false;
    m_overlaySidebarReturnToMenu = false;
    m_overlaySidebarAnimStartTick = armGetSystemTick();
    m_overlaySidebarFocus = 0;
    m_selectorDirection = 0;
    resetContentScroll();
}

void MgbaMenuLayer::beginShaderSidebar()
{
    m_visible = false;
    m_panelAnimating = false;
    m_shaderSidebarVisible = true;
    m_shaderSidebarClosing = false;
    m_shaderSidebarReturnToMenu = false;
    m_shaderSidebarAnimStartTick = armGetSystemTick();
    m_shaderSidebarFocus = 0;
    m_shaderListVisible = false;
    m_selectorDirection = 0;
    resetShaderParamScroll();
    resetShaderListScroll();
    resetContentScroll();
}

void MgbaMenuLayer::beginFilePicker()
{
    m_overlaySidebarVisible = false;
    m_overlaySidebarClosing = false;
    m_filePickerVisible = true;
    m_filePickerClosing = false;
    m_filePickerReturnToOverlay = false;
    m_filePickerAnimStartTick = armGetSystemTick();
    reloadFilePickerEntries(parentPathOrRoot(m_display.overlayPath), m_display.overlayPath);
}

void MgbaMenuLayer::closeOverlaySidebar(bool returnToMenu)
{
    if (!m_overlaySidebarVisible || m_overlaySidebarClosing)
        return;
    m_overlaySidebarClosing = true;
    m_overlaySidebarReturnToMenu = returnToMenu;
    m_overlaySidebarAnimStartTick = armGetSystemTick();
    m_selectorDirection = 0;
}

void MgbaMenuLayer::closeShaderSidebar(bool returnToMenu)
{
    if (!m_shaderSidebarVisible || m_shaderSidebarClosing)
        return;
    m_shaderListVisible = false;
    m_shaderSidebarClosing = true;
    m_shaderSidebarReturnToMenu = returnToMenu;
    m_shaderSidebarAnimStartTick = armGetSystemTick();
    m_selectorDirection = 0;
}

void MgbaMenuLayer::closeFilePicker(bool returnToOverlay)
{
    if (!m_filePickerVisible || m_filePickerClosing)
        return;
    m_filePickerClosing = true;
    m_filePickerReturnToOverlay = returnToOverlay;
    m_filePickerAnimStartTick = armGetSystemTick();
}

bool MgbaMenuLayer::cycleCustomLayoutSetting(int direction)
{
    if (direction == 0)
        return false;

    switch (m_customLayoutFocus)
    {
    case 0:
        m_display.customLayout.topScale = stepFloatValue(m_display.customLayout.topScale,
                                                         direction,
                                                         kCustomScaleStep,
                                                         kCustomScaleMin,
                                                         kCustomScaleMax);
        m_display.customLayout.bottomScale = m_display.customLayout.topScale;
        return true;
    case 1:
        m_display.customLayout.topOffsetX = stepFloatValue(m_display.customLayout.topOffsetX,
                                                           direction,
                                                           kCustomOffsetStep,
                                                           kCustomOffsetMin,
                                                           kCustomOffsetMax);
        m_display.customLayout.bottomOffsetX = m_display.customLayout.topOffsetX;
        return true;
    case 2:
        m_display.customLayout.topOffsetY = stepFloatValue(m_display.customLayout.topOffsetY,
                                                           direction,
                                                           kCustomOffsetStep,
                                                           kCustomOffsetMin,
                                                           kCustomOffsetMax);
        m_display.customLayout.bottomOffsetY = m_display.customLayout.topOffsetY;
        return true;
    default:
        return false;
    }
}

bool MgbaMenuLayer::resetCustomLayoutSetting()
{
    auto resetFloat = [](float& value, float defaultValue) {
        if (std::fabs(value - defaultValue) < 0.001f)
            return false;
        value = defaultValue;
        return true;
    };

    switch (m_customLayoutFocus)
    {
    case 0:
    {
        const bool changed = resetFloat(m_display.customLayout.topScale, kCustomScaleDefault);
        m_display.customLayout.bottomScale = m_display.customLayout.topScale;
        return changed;
    }
    case 1:
    {
        const bool changed = resetFloat(m_display.customLayout.topOffsetX, kCustomOffsetDefault);
        m_display.customLayout.bottomOffsetX = m_display.customLayout.topOffsetX;
        return changed;
    }
    case 2:
    {
        const bool changed = resetFloat(m_display.customLayout.topOffsetY, kCustomOffsetDefault);
        m_display.customLayout.bottomOffsetY = m_display.customLayout.topOffsetY;
        return changed;
    }
    default: return false;
    }
}

bool MgbaMenuLayer::cycleOverlaySetting(int direction)
{
    (void)direction;
    if (m_overlaySidebarFocus == 0)
    {
        m_display.overlayEnabled = !m_display.overlayEnabled;
        return true;
    }
    return false;
}

bool MgbaMenuLayer::cycleShaderSetting(int direction)
{
    if (m_shaderSidebarFocus == 0)
    {
        m_display.shaderEnabled = !m_display.shaderEnabled;
        return true;
    }
    if (m_shaderSidebarFocus >= kShaderBaseControlCount)
    {
        const int paramIndex = m_shaderSidebarFocus - kShaderBaseControlCount;
        if (paramIndex < 0 || paramIndex >= static_cast<int>(m_display.shaderParams.size()))
            return false;

        auto& param = m_display.shaderParams[paramIndex];
        const float next = direction == 0
            ? param.defaultValue
            : stepFloatValue(param.value, direction, param.step, param.minValue, param.maxValue);
        if (std::fabs(next - param.value) < 0.0001f)
            return false;
        param.value = next;
        return true;
    }

    return false;
}

bool MgbaMenuLayer::cycleCurrentSetting(int direction)
{
    if (static_cast<Item>(m_selected) != Item::Display || direction == 0)
        return false;

    auto cycleIndex = [direction](int value, int count) {
        return (value + direction + count) % count;
    };

    switch (m_contentFocus)
    {
    case 0:
    {
        int idx = 2;
        for (int i = 0; i < static_cast<int>(std::size(kFastForwardValues)); ++i)
        {
            if (std::fabs(kFastForwardValues[i] - m_display.fastForwardMultiplier) < 0.01f)
            {
                idx = i;
                break;
            }
        }
        idx = cycleIndex(idx, static_cast<int>(std::size(kFastForwardValues)));
        m_display.fastForwardMultiplier = kFastForwardValues[idx];
        return true;
    }
    case 1:
        m_display.linearFiltering = !m_display.linearFiltering;
        return true;
    case 2:
        m_display.layout = cycleIndex(m_display.layout, 6);
        if (m_contentFocus == kDisplayRowCustomLayout && m_display.layout != kCustomLayoutMode)
            m_contentFocus = nextFocusableDisplayRow(m_contentFocus, direction);
        return true;
    case 3:
    {
        if (m_display.layout != 4)
            return false;
        int idx = std::clamp(m_display.integerScaleMultiplier, 0, 8);
        idx = cycleIndex(idx, 9);
        m_display.integerScale = true;
        m_display.integerScaleMultiplier = idx;
        return true;
    }
    default:
        return false;
    }
}

bool MgbaMenuLayer::updateHeldSelector(std::uint64_t buttonsHeld)
{
    if (m_focusScope != FocusScope::Content ||
        static_cast<Item>(m_selected) != Item::Display ||
        (buttonsHeld & (HidNpadButton_L | HidNpadButton_R)) == 0)
    {
        m_selectorDirection = 0;
        return false;
    }

    const int direction = (buttonsHeld & HidNpadButton_R) ? 1 : -1;
    const std::uint64_t now = armGetSystemTick();
    if (m_selectorDirection != direction)
    {
        m_selectorDirection = direction;
        m_selectorRepeatStartTick = now;
        m_selectorLastStepTick = now;
        return false;
    }

    const float heldMs = static_cast<float>(armTicksToNs(now - m_selectorRepeatStartTick)) / 1000000.0f;
    if (heldMs < kSelectorInitialDelayMs)
        return false;

    const float intervalMs = std::max(52.0f, 180.0f - (heldMs - kSelectorInitialDelayMs) * 0.25f);
    const float sinceLastMs = static_cast<float>(armTicksToNs(now - m_selectorLastStepTick)) / 1000000.0f;
    if (sinceLastMs < intervalMs)
        return false;

    m_selectorLastStepTick = now;
    return cycleCurrentSetting(direction);
}

bool MgbaMenuLayer::updateHeldCustomSelector(std::uint64_t buttonsHeld)
{
    if (!m_customLayoutEditorVisible ||
        (buttonsHeld & (HidNpadButton_L | HidNpadButton_R)) == 0)
    {
        m_selectorDirection = 0;
        return false;
    }

    const int direction = (buttonsHeld & HidNpadButton_R) ? 1 : -1;
    const std::uint64_t now = armGetSystemTick();
    if (m_selectorDirection != direction)
    {
        m_selectorDirection = direction;
        m_selectorRepeatStartTick = now;
        m_selectorLastStepTick = now;
        return false;
    }

    const float heldMs = static_cast<float>(armTicksToNs(now - m_selectorRepeatStartTick)) / 1000000.0f;
    if (heldMs < kSelectorInitialDelayMs)
        return false;

    const float intervalMs = std::max(52.0f, 180.0f - (heldMs - kSelectorInitialDelayMs) * 0.25f);
    const float sinceLastMs = static_cast<float>(armTicksToNs(now - m_selectorLastStepTick)) / 1000000.0f;
    if (sinceLastMs < intervalMs)
        return false;

    m_selectorLastStepTick = now;
    return cycleCustomLayoutSetting(direction);
}

bool MgbaMenuLayer::updateHeldShaderSelector(std::uint64_t buttonsHeld)
{
    if (!m_shaderSidebarVisible ||
        m_shaderSidebarFocus < kShaderBaseControlCount ||
        (buttonsHeld & (HidNpadButton_L | HidNpadButton_R)) == 0)
    {
        m_selectorDirection = 0;
        return false;
    }

    const int direction = (buttonsHeld & HidNpadButton_R) ? 1 : -1;
    const std::uint64_t now = armGetSystemTick();
    if (m_selectorDirection != direction)
    {
        m_selectorDirection = direction;
        m_selectorRepeatStartTick = now;
        m_selectorLastStepTick = now;
        return false;
    }

    const float heldMs = static_cast<float>(armTicksToNs(now - m_selectorRepeatStartTick)) / 1000000.0f;
    if (heldMs < kSelectorInitialDelayMs)
        return false;

    const float intervalMs = std::max(52.0f, 180.0f - (heldMs - kSelectorInitialDelayMs) * 0.25f);
    const float sinceLastMs = static_cast<float>(armTicksToNs(now - m_selectorLastStepTick)) / 1000000.0f;
    if (sinceLastMs < intervalMs)
        return false;

    m_selectorLastStepTick = now;
    return cycleShaderSetting(direction);
}

std::uint64_t MgbaMenuLayer::updateHeldNavigation(std::uint64_t buttonsDown, std::uint64_t buttonsHeld)
{
    const int heldDirection = navDirectionFromButtons(buttonsHeld);
    if (heldDirection == 0)
    {
        m_navDirection = 0;
        return 0;
    }

    const std::uint64_t now = armGetSystemTick();
    const int downDirection = navDirectionFromButtons(buttonsDown);
    if (downDirection != 0 || m_navDirection != heldDirection)
    {
        m_navDirection = heldDirection;
        m_navRepeatStartTick = now;
        m_navLastStepTick = now;
        return 0;
    }

    const float heldMs = static_cast<float>(armTicksToNs(now - m_navRepeatStartTick)) / 1000000.0f;
    if (heldMs < kNavInitialDelayMs)
        return 0;

    const float intervalMs = std::max(48.0f, 128.0f - (heldMs - kNavInitialDelayMs) * 0.12f);
    const float sinceLastMs = static_cast<float>(armTicksToNs(now - m_navLastStepTick)) / 1000000.0f;
    if (sinceLastMs < intervalMs)
        return 0;

    m_navLastStepTick = now;
    return buttonsFromNavDirection(heldDirection);
}

void MgbaMenuLayer::openDeleteDialog()
{
    if (m_focusScope != FocusScope::Content)
        return;
    const Item item = static_cast<Item>(m_selected);
    if (item != Item::SaveState && item != Item::LoadState)
        return;
    if (m_contentFocus < 0 || m_contentFocus >= static_cast<int>(m_slots.size()) || !m_slots[m_contentFocus].exists)
        return;

    m_deleteSlot = m_contentFocus;
    m_deleteDialogVisible = true;
}

void MgbaMenuLayer::closeDeleteDialog()
{
    m_deleteDialogVisible = false;
    m_deleteSlot = -1;
}

void MgbaMenuLayer::openSyncConfirmDialog(MgbaMenuAction action)
{
    if (action != MgbaMenuAction::SyncDisplaySettings &&
        action != MgbaMenuAction::SyncOverlaySettings &&
        action != MgbaMenuAction::SyncShaderSettings)
        return;
    m_syncConfirmVisible = true;
    m_syncConfirmAction = action;
    closeSyncResultDialog();
}

void MgbaMenuLayer::closeSyncConfirmDialog()
{
    m_syncConfirmVisible = false;
    m_syncConfirmAction = MgbaMenuAction::None;
}

void MgbaMenuLayer::showSyncResult(MgbaMenuAction action, int count)
{
    if (action != MgbaMenuAction::SyncDisplaySettings &&
        action != MgbaMenuAction::SyncOverlaySettings &&
        action != MgbaMenuAction::SyncShaderSettings)
        return;
    closeSyncConfirmDialog();
    m_syncResultVisible = true;
    m_syncResultAction = action;
    m_syncResultCount = count;
}

void MgbaMenuLayer::closeSyncResultDialog()
{
    m_syncResultVisible = false;
    m_syncResultAction = MgbaMenuAction::None;
    m_syncResultCount = 0;
}

MgbaMenuResult MgbaMenuLayer::update(std::uint64_t buttonsDown, std::uint64_t buttonsHeld)
{
    if (!active())
        return {};

    if (m_panelAnimating && animationProgress(m_panelAnimStartTick, kPanelAnimationMs) >= 1.0f)
        m_panelAnimating = false;

    if (m_customLayoutEditorVisible && m_customLayoutEditorClosing &&
        animationProgress(m_customLayoutAnimStartTick, kSidebarAnimationMs) >= 1.0f)
    {
        m_customLayoutEditorVisible = false;
        m_customLayoutEditorClosing = false;
        const bool returnToMenu = m_customLayoutReturnToMenu;
        m_customLayoutReturnToMenu = false;
        m_selectorDirection = 0;
        if (returnToMenu)
            reopenDisplayContent(kDisplayRowCustomLayout);
    }

    if (m_overlaySidebarVisible && m_overlaySidebarClosing &&
        animationProgress(m_overlaySidebarAnimStartTick, kSidebarAnimationMs) >= 1.0f)
    {
        m_overlaySidebarVisible = false;
        m_overlaySidebarClosing = false;
        const bool returnToMenu = m_overlaySidebarReturnToMenu;
        m_overlaySidebarReturnToMenu = false;
        m_selectorDirection = 0;
        if (returnToMenu)
            reopenDisplayContent(kDisplayRowOverlay);
    }

    if (m_shaderSidebarVisible && m_shaderSidebarClosing &&
        animationProgress(m_shaderSidebarAnimStartTick, kSidebarAnimationMs) >= 1.0f)
    {
        m_shaderSidebarVisible = false;
        m_shaderSidebarClosing = false;
        const bool returnToMenu = m_shaderSidebarReturnToMenu;
        m_shaderSidebarReturnToMenu = false;
        m_selectorDirection = 0;
        if (returnToMenu)
            reopenDisplayContent(kDisplayRowShader);
    }

    if (m_filePickerVisible && m_filePickerClosing &&
        animationProgress(m_filePickerAnimStartTick, kPanelAnimationMs) >= 1.0f)
    {
        m_filePickerVisible = false;
        m_filePickerClosing = false;
        const bool returnToOverlay = m_filePickerReturnToOverlay;
        m_filePickerReturnToOverlay = false;
        releaseFilePickerPreview();
        if (returnToOverlay)
        {
            m_overlaySidebarVisible = true;
            m_overlaySidebarClosing = false;
            m_overlaySidebarReturnToMenu = false;
            m_overlaySidebarAnimStartTick = armGetSystemTick();
        }
    }

    if (m_filePickerVisible)
    {
        if (m_filePickerClosing)
            return {};

        if (m_filePickerImagePreviewVisible)
        {
            if (buttonsDown & (HidNpadButton_A | HidNpadButton_B))
            {
                queueSound((buttonsDown & HidNpadButton_B) ? MgbaMenuSound::Back : MgbaMenuSound::Click);
                releaseFilePickerPreview();
            }
            return {};
        }

        const std::uint64_t navButtons = buttonsDown | updateHeldNavigation(buttonsDown, buttonsHeld);
        if (buttonsDown & HidNpadButton_B)
        {
            queueSound(MgbaMenuSound::Back);
            closeFilePicker(true);
            return {};
        }
        const int oldFocus = m_filePickerFocus;
        if (isDirectionUp(navButtons) && !m_filePickerEntries.empty())
            m_filePickerFocus = std::max(0, m_filePickerFocus - 1);
        if (isDirectionDown(navButtons) && !m_filePickerEntries.empty())
            m_filePickerFocus = std::min(static_cast<int>(m_filePickerEntries.size()) - 1, m_filePickerFocus + 1);
        if (m_filePickerFocus != oldFocus)
            queueSound(MgbaMenuSound::Focus);

        if ((buttonsDown & HidNpadButton_A) &&
            m_filePickerFocus >= 0 &&
            m_filePickerFocus < static_cast<int>(m_filePickerEntries.size()))
        {
            const auto entry = m_filePickerEntries[m_filePickerFocus];
            if (entry.isDirectory)
            {
                queueSound(MgbaMenuSound::Click);
                reloadFilePickerEntries(entry.path);
                return {};
            }
            if (eMgbaWithNoCase(entry.path, ".png"))
            {
                queueSound(MgbaMenuSound::Click);
                m_display.overlayPath = entry.path;
                closeFilePicker(true);
                return {MgbaMenuAction::OverlayPathSelected, -1, entry.path};
            }
            queueSound(MgbaMenuSound::Error);
        }

        if ((buttonsDown & HidNpadButton_X) &&
            m_filePickerFocus >= 0 &&
            m_filePickerFocus < static_cast<int>(m_filePickerEntries.size()))
        {
            const auto& entry = m_filePickerEntries[m_filePickerFocus];
            if (!entry.isDirectory && eMgbaWithNoCase(entry.path, ".png"))
            {
                queueSound(MgbaMenuSound::Click);
                ensureFilePickerPreview();
            }
            else
            {
                queueSound(MgbaMenuSound::Error);
            }
        }
        return {};
    }

    if (m_customLayoutEditorVisible)
    {
        if (m_customLayoutEditorClosing)
            return {};

        const std::uint64_t navButtons = buttonsDown | updateHeldNavigation(buttonsDown, buttonsHeld);

        if (buttonsDown & HidNpadButton_B)
        {
            queueSound(MgbaMenuSound::Back);
            m_customLayoutEditorClosing = true;
            m_customLayoutReturnToMenu = true;
            m_customLayoutAnimStartTick = armGetSystemTick();
            m_selectorDirection = 0;
            return {MgbaMenuAction::CustomLayoutCommitted, -1};
        }
        const int oldFocus = m_customLayoutFocus;
        if (isDirectionUp(navButtons))
            m_customLayoutFocus = (m_customLayoutFocus + kCustomLayoutControlCount - 1) % kCustomLayoutControlCount;
        if (isDirectionDown(navButtons))
            m_customLayoutFocus = (m_customLayoutFocus + 1) % kCustomLayoutControlCount;
        if (m_customLayoutFocus != oldFocus)
            queueSound(MgbaMenuSound::Focus);

        if (buttonsDown & HidNpadButton_L)
        {
            if (cycleCustomLayoutSetting(-1))
            {
                queueSound(MgbaMenuSound::Slider);
                return {MgbaMenuAction::CustomLayoutChanged, -1};
            }
            return {};
        }
        if (buttonsDown & HidNpadButton_R)
        {
            if (cycleCustomLayoutSetting(1))
            {
                queueSound(MgbaMenuSound::Slider);
                return {MgbaMenuAction::CustomLayoutChanged, -1};
            }
            return {};
        }
        if (updateHeldCustomSelector(buttonsHeld))
        {
            queueSound(MgbaMenuSound::Slider);
            return {MgbaMenuAction::CustomLayoutChanged, -1};
        }
        if (isDirectionLeft(buttonsDown) || isDirectionRight(buttonsDown))
        {
            if (cycleCustomLayoutSetting(isDirectionRight(buttonsDown) ? 1 : -1))
            {
                queueSound(MgbaMenuSound::Slider);
                return {MgbaMenuAction::CustomLayoutChanged, -1};
            }
            return {};
        }
        if ((buttonsDown & HidNpadButton_A) && resetCustomLayoutSetting())
        {
            queueSound(MgbaMenuSound::Click);
            return {MgbaMenuAction::CustomLayoutChanged, -1};
        }
        if (buttonsDown & HidNpadButton_A)
            queueSound(MgbaMenuSound::Error);

        return {};
    }

    if (m_overlaySidebarVisible)
    {
        if (m_overlaySidebarClosing)
            return {};
        const std::uint64_t navButtons = buttonsDown | updateHeldNavigation(buttonsDown, buttonsHeld);
        if (buttonsDown & HidNpadButton_B)
        {
            queueSound(MgbaMenuSound::Back);
            closeOverlaySidebar(true);
            return {MgbaMenuAction::OverlaySettingsCommitted, -1};
        }
        const int oldFocus = m_overlaySidebarFocus;
        if (isDirectionUp(navButtons))
            m_overlaySidebarFocus = (m_overlaySidebarFocus + kOverlayControlCount - 1) % kOverlayControlCount;
        if (isDirectionDown(navButtons))
            m_overlaySidebarFocus = (m_overlaySidebarFocus + 1) % kOverlayControlCount;
        if (m_overlaySidebarFocus != oldFocus)
            queueSound(MgbaMenuSound::Focus);
        if (buttonsDown & HidNpadButton_A)
        {
            if (m_overlaySidebarFocus == 0)
            {
                if (cycleOverlaySetting(0))
                {
                    queueSound(MgbaMenuSound::Click);
                    return {MgbaMenuAction::OverlaySettingsChanged, -1};
                }
                return {};
            }
            if (m_overlaySidebarFocus == 1)
            {
                queueSound(MgbaMenuSound::Click);
                beginFilePicker();
                return {};
            }
        }
        return {};
    }

    if (m_shaderSidebarVisible)
    {
        if (m_shaderSidebarClosing)
            return {};
        if (m_shaderListVisible)
        {
            const std::uint64_t navButtons = buttonsDown | updateHeldNavigation(buttonsDown, buttonsHeld);
            auto shaderEntries = MgbaShaderListEntries(m_shaderListPath);
            const int shaderCount = std::max(1, static_cast<int>(shaderEntries.size()));
            if (buttonsDown & HidNpadButton_B)
            {
                queueSound(MgbaMenuSound::Back);
                if (m_shaderListPath.empty())
                {
                    closeShaderList();
                }
                else
                {
                    const std::string previous = m_shaderListPath.back();
                    m_shaderListPath.pop_back();
                    shaderEntries = MgbaShaderListEntries(m_shaderListPath);
                    m_shaderListFocus = 0;
                    for (int i = 0; i < static_cast<int>(shaderEntries.size()); ++i)
                    {
                        if (shaderEntries[i].kind == MgbaShaderListEntry::Kind::Directory &&
                            shaderEntries[i].label == previous)
                        {
                            m_shaderListFocus = i;
                            break;
                        }
                    }
                    resetShaderListScroll();
                }
                return {};
            }
            const int oldFocus = m_shaderListFocus;
            if (isDirectionUp(navButtons))
                m_shaderListFocus = (m_shaderListFocus + shaderCount - 1) % shaderCount;
            if (isDirectionDown(navButtons))
                m_shaderListFocus = (m_shaderListFocus + 1) % shaderCount;
            if (m_shaderListFocus != oldFocus)
                queueSound(MgbaMenuSound::Focus);
            if (buttonsDown & HidNpadButton_A)
            {
                if (shaderEntries.empty())
                {
                    queueSound(MgbaMenuSound::Error);
                    return {};
                }
                const auto& entry = shaderEntries[std::clamp(m_shaderListFocus, 0, static_cast<int>(shaderEntries.size()) - 1)];
                if (entry.kind == MgbaShaderListEntry::Kind::Directory)
                {
                    queueSound(MgbaMenuSound::Click);
                    m_shaderListPath = entry.path;
                    m_shaderListFocus = currentShaderTypeIndex();
                    resetShaderListScroll();
                    return {};
                }
                const std::string nextType = entry.shaderType;
                closeShaderList();
                if (nextType == m_display.mgbaShaderType)
                {
                    queueSound(MgbaMenuSound::Click);
                    return {};
                }
                queueSound(MgbaMenuSound::Click);
                m_display.mgbaShaderType = nextType;
                resetShaderParamScroll();
                return {MgbaMenuAction::ShaderSettingsChanged, -1};
            }
            return {};
        }
        const std::uint64_t navButtons = buttonsDown | updateHeldNavigation(buttonsDown, buttonsHeld);
        if (buttonsDown & HidNpadButton_B)
        {
            queueSound(MgbaMenuSound::Back);
            closeShaderSidebar(true);
            return {MgbaMenuAction::ShaderSettingsCommitted, -1};
        }
        const int shaderCount = std::max(1, shaderControlCount());
        const int oldFocus = m_shaderSidebarFocus;
        if (isDirectionUp(navButtons))
            m_shaderSidebarFocus = (m_shaderSidebarFocus + shaderCount - 1) % shaderCount;
        if (isDirectionDown(navButtons))
            m_shaderSidebarFocus = (m_shaderSidebarFocus + 1) % shaderCount;
        if (m_shaderSidebarFocus != oldFocus)
            queueSound(MgbaMenuSound::Focus);
        if (buttonsDown & HidNpadButton_A)
        {
            if (m_shaderSidebarFocus == 1)
            {
                queueSound(MgbaMenuSound::Click);
                beginShaderList();
                return {};
            }
            if (cycleShaderSetting(0))
            {
                queueSound(MgbaMenuSound::Click);
                return {MgbaMenuAction::ShaderSettingsChanged, -1};
            }
            queueSound(MgbaMenuSound::Error);
            return {};
        }
        if (buttonsDown & HidNpadButton_L)
        {
            if (cycleShaderSetting(-1))
            {
                queueSound(MgbaMenuSound::Slider);
                return {MgbaMenuAction::ShaderSettingsChanged, -1};
            }
            return {};
        }
        if (buttonsDown & HidNpadButton_R)
        {
            if (cycleShaderSetting(1))
            {
                queueSound(MgbaMenuSound::Slider);
                return {MgbaMenuAction::ShaderSettingsChanged, -1};
            }
            return {};
        }
        if (updateHeldShaderSelector(buttonsHeld))
        {
            queueSound(MgbaMenuSound::Slider);
            return {MgbaMenuAction::ShaderSettingsChanged, -1};
        }
        if (isDirectionLeft(buttonsDown) || isDirectionRight(buttonsDown))
        {
            if (cycleShaderSetting(isDirectionRight(buttonsDown) ? 1 : -1))
            {
                queueSound(MgbaMenuSound::Slider);
                return {MgbaMenuAction::ShaderSettingsChanged, -1};
            }
            return {};
        }
        return {};
    }

    if (!m_visible)
        return {};

    if (m_syncResultVisible)
    {
        if ((buttonsDown & HidNpadButton_A) || (buttonsDown & HidNpadButton_B))
        {
            queueSound((buttonsDown & HidNpadButton_B) ? MgbaMenuSound::Back : MgbaMenuSound::Click);
            closeSyncResultDialog();
        }
        return {};
    }

    if (m_syncConfirmVisible)
    {
        if (buttonsDown & HidNpadButton_B)
        {
            queueSound(MgbaMenuSound::Back);
            closeSyncConfirmDialog();
            return {};
        }
        if (buttonsDown & HidNpadButton_A)
        {
            queueSound(MgbaMenuSound::Click);
            const MgbaMenuAction action = m_syncConfirmAction;
            closeSyncConfirmDialog();
            return {action, -1};
        }
        return {};
    }

    if (m_deleteDialogVisible)
    {
        if (buttonsDown & HidNpadButton_B)
        {
            queueSound(MgbaMenuSound::Back);
            closeDeleteDialog();
            return {};
        }
        if (buttonsDown & HidNpadButton_A)
        {
            queueSound(MgbaMenuSound::Click);
            const int slot = m_deleteSlot;
            closeDeleteDialog();
            return {MgbaMenuAction::DeleteState, slot};
        }
        return {};
    }

    if (buttonsDown & HidNpadButton_B)
    {
        queueSound(MgbaMenuSound::Back);
        if (m_focusScope == FocusScope::Content)
        {
            m_focusScope = FocusScope::Tabs;
            resetContentScroll();
            return {};
        }

        close();
        return {};
    }

    const int itemCount = itemIndex(Item::Count);
    const std::uint64_t navButtons = buttonsDown | updateHeldNavigation(buttonsDown, buttonsHeld);
    if (m_focusScope == FocusScope::Tabs && isDirectionUp(navButtons))
    {
        queueSound(MgbaMenuSound::Focus);
        beginSelectionAnimation(m_selected, (m_selected + itemCount - 1) % itemCount);
        return {};
    }
    if (m_focusScope == FocusScope::Tabs && isDirectionDown(navButtons))
    {
        queueSound(MgbaMenuSound::Focus);
        beginSelectionAnimation(m_selected, (m_selected + 1) % itemCount);
        return {};
    }

    const Item currentItem = static_cast<Item>(m_selected);
    if (m_focusScope == FocusScope::Content)
    {
        if (currentItem == Item::SaveState || currentItem == Item::LoadState)
        {
            const int oldFocus = m_contentFocus;
            const int count = contentControlCount(currentItem);
            if (isDirectionUp(navButtons))
                m_contentFocus = std::max(0, m_contentFocus - 2);
            else if (isDirectionDown(navButtons))
                m_contentFocus = std::min(count - 1, m_contentFocus + 2);
            else if (isDirectionLeft(navButtons) && (m_contentFocus % 2) == 1)
                m_contentFocus = std::max(0, m_contentFocus - 1);
            else if (isDirectionRight(navButtons) && (m_contentFocus % 2) == 0)
                m_contentFocus = std::min(count - 1, m_contentFocus + 1);
            if (m_contentFocus != oldFocus)
            {
                queueSound(MgbaMenuSound::Focus);
                releaseStatePreviewTexture();
            }

            if (buttonsDown & HidNpadButton_A)
            {
                if (currentItem == Item::LoadState && !m_slots[m_contentFocus].loadable)
                {
                    queueSound(MgbaMenuSound::Error);
                    return {};
                }
                queueSound(MgbaMenuSound::Click);
                return {currentItem == Item::SaveState ? MgbaMenuAction::SaveState : MgbaMenuAction::LoadState,
                        m_contentFocus};
            }
            if (buttonsDown & HidNpadButton_X)
            {
                openDeleteDialog();
                queueSound(m_deleteDialogVisible ? MgbaMenuSound::Click : MgbaMenuSound::Error);
            }
            return {};
        }

        if (currentItem == Item::Display)
        {
            const int oldFocus = m_contentFocus;
            if (isDirectionUp(navButtons))
                m_contentFocus = nextFocusableDisplayRow(m_contentFocus, -1);
            if (isDirectionDown(navButtons))
                m_contentFocus = nextFocusableDisplayRow(m_contentFocus, 1);
            if (m_contentFocus != oldFocus)
                queueSound(MgbaMenuSound::Focus);

            if (buttonsDown & HidNpadButton_L)
            {
                if (cycleCurrentSetting(-1))
                {
                    queueSound(MgbaMenuSound::Slider);
                    return {MgbaMenuAction::DisplaySettingsChanged, -1};
                }
                return {};
            }
            if (buttonsDown & HidNpadButton_R)
            {
                if (cycleCurrentSetting(1))
                {
                    queueSound(MgbaMenuSound::Slider);
                    return {MgbaMenuAction::DisplaySettingsChanged, -1};
                }
                return {};
            }
            if (updateHeldSelector(buttonsHeld))
            {
                queueSound(MgbaMenuSound::Slider);
                return {MgbaMenuAction::DisplaySettingsChanged, -1};
            }
            if (isDirectionLeft(buttonsDown) || isDirectionRight(buttonsDown))
            {
                if (cycleCurrentSetting(isDirectionRight(buttonsDown) ? 1 : -1))
                {
                    queueSound(MgbaMenuSound::Slider);
                    return {MgbaMenuAction::DisplaySettingsChanged, -1};
                }
                return {};
            }

            if (buttonsDown & HidNpadButton_A)
            {
                if (m_contentFocus == kDisplayRowSyncDisplay)
                {
                    queueSound(MgbaMenuSound::Click);
                    openSyncConfirmDialog(MgbaMenuAction::SyncDisplaySettings);
                    return {};
                }
                if (m_contentFocus == kDisplayRowSyncOverlay)
                {
                    queueSound(MgbaMenuSound::Click);
                    openSyncConfirmDialog(MgbaMenuAction::SyncOverlaySettings);
                    return {};
                }
                if (m_contentFocus == kDisplayRowSyncShader)
                {
                    queueSound(MgbaMenuSound::Click);
                    openSyncConfirmDialog(MgbaMenuAction::SyncShaderSettings);
                    return {};
                }
            }
            const bool opensDisplaySubPage =
                (m_contentFocus == kDisplayRowCustomLayout && m_display.layout == kCustomLayoutMode) ||
                m_contentFocus == kDisplayRowOverlay ||
                m_contentFocus == kDisplayRowShader;
            if ((buttonsDown & HidNpadButton_A) && activateDisplayControl())
            {
                queueSound(MgbaMenuSound::Click);
                return {MgbaMenuAction::DisplaySettingsChanged, -1};
            }
            if ((buttonsDown & HidNpadButton_A) && opensDisplaySubPage)
                queueSound(MgbaMenuSound::Click);
            else if ((buttonsDown & HidNpadButton_A) && m_contentFocus == kDisplayRowCustomLayout)
                queueSound(MgbaMenuSound::Error);
            return {};
        }

        if (currentItem == Item::Cheats)
        {
            const int count = contentControlCount(Item::Cheats);
            if (count <= 0)
                return {};

            const int oldFocus = m_contentFocus;
            if (isDirectionUp(navButtons))
                m_contentFocus = std::max(0, m_contentFocus - 1);
            if (isDirectionDown(navButtons))
                m_contentFocus = std::min(count - 1, m_contentFocus + 1);
            if (m_contentFocus != oldFocus)
                queueSound(MgbaMenuSound::Focus);

            if (buttonsDown & HidNpadButton_A)
            {
                queueSound(MgbaMenuSound::Click);
                return activateCheatControl() ? MgbaMenuResult{MgbaMenuAction::CheatSettingsChanged, -1}
                                              : MgbaMenuResult{};
            }
            if (isDirectionLeft(buttonsDown) || isDirectionRight(buttonsDown))
            {
                const int index = visibleCheatIndex(m_contentFocus);
                if (index >= 0 && index < static_cast<int>(m_cheats.size()) &&
                    m_cheats[index].type == MgbaCheatItem::Type::Category)
                {
                    queueSound(MgbaMenuSound::Click);
                    m_cheats[index].expanded = isDirectionRight(buttonsDown);
                    invalidateVisibleCheatCache();
                    const int nextCount = contentControlCount(Item::Cheats);
                    m_contentFocus = std::clamp(m_contentFocus, 0, std::max(0, nextCount - 1));
                    resetContentScroll();
                }
            }
            return {};
        }

        return {};
    }

    if (buttonsDown & HidNpadButton_A)
    {
        switch (currentItem)
        {
        case Item::Resume:
            queueSound(MgbaMenuSound::Click);
            close();
            return {};
        case Item::SaveState:
        case Item::LoadState:
        case Item::Cheats:
        case Item::Display:
            if (itemHasContent(currentItem))
            {
                queueSound(MgbaMenuSound::Click);
                m_focusScope = FocusScope::Content;
                m_contentFocus = 0;
                resetContentScroll();
            }
            return {};
        case Item::Reset:
            queueSound(MgbaMenuSound::Click);
            return {MgbaMenuAction::ResetGame, -1};
        case Item::Exit:
            queueSound(MgbaMenuSound::Click);
            return {MgbaMenuAction::ExitGame, -1};
        default:
            return {};
        }
    }

    return {};
}

void MgbaMenuLayer::draw() const
{
    auto drawToastIfNeeded = [&]() {
        if (m_toastMessage.empty() || m_toastStartTick == 0)
            return;

        constexpr float kToastInMs = 180.0f;
        constexpr float kToastHoldMs = 2000.0f;
        constexpr float kToastOutMs = 180.0f;
        const std::uint64_t now = armGetSystemTick();
        const float elapsedMs = static_cast<float>(armTicksToNs(now - m_toastStartTick)) / 1000000.0f;
        const float totalMs = kToastInMs + kToastHoldMs + kToastOutMs;
        if (elapsedMs >= totalMs)
        {
            m_toastMessage.clear();
            m_toastStartTick = 0;
            return;
        }

        float progress = 1.0f;
        if (elapsedMs < kToastInMs)
            progress = elapsedMs / kToastInMs;
        else if (elapsedMs > kToastInMs + kToastHoldMs)
            progress = 1.0f - (elapsedMs - kToastInMs - kToastHoldMs) / kToastOutMs;

        setMenuMetricsOrientation(0);
        const bool transformed = pushMenuOrientationTransform(0);
        drawToast(m_toastMessage, progress, 1.0f);
        if (transformed)
            Gfx::PopDrawTransform();
        setMenuMetricsOrientation(0);
    };

    if (!active())
    {
        drawToastIfNeeded();
        return;
    }

    if (m_filePickerVisible)
    {
        setMenuMetricsOrientation(0);
        const float progress = m_filePickerClosing
            ? 1.0f - easeOutCubic(animationProgress(m_filePickerAnimStartTick, kPanelAnimationMs))
            : easeOutCubic(animationProgress(m_filePickerAnimStartTick, kPanelAnimationMs));
        if (progress > 0.0f)
        {
            const bool transformed = pushMenuOrientationTransform(0);
            const float target = filePickerTargetScroll(m_filePickerFocus,
                                                        static_cast<int>(m_filePickerEntries.size()));
            const std::uint64_t now = armGetSystemTick();
            if (m_filePickerScrollLastTick == 0)
            {
                m_filePickerScrollLastTick = now;
                m_filePickerScrollY = target;
            }
            else
            {
                const float dtMs = static_cast<float>(armTicksToNs(now - m_filePickerScrollLastTick)) / 1000000.0f;
                m_filePickerScrollLastTick = now;
                const float t = 1.0f - std::exp(-dtMs / 68.0f);
                m_filePickerScrollY += (target - m_filePickerScrollY) * std::clamp(t, 0.0f, 1.0f);
                if (std::fabs(target - m_filePickerScrollY) < 0.5f)
                    m_filePickerScrollY = target;
            }
            drawFilePicker(m_filePickerDirectory,
                           m_filePickerEntries,
                           m_filePickerFocus,
                           m_filePickerScrollY,
                           m_filePickerPreviewTexture,
                           m_filePickerPreviewWidth,
                           m_filePickerPreviewHeight,
                           m_filePickerPreviewPath,
                           m_filePickerImagePreviewVisible,
                           progress,
                           progress);
            if (transformed)
                Gfx::PopDrawTransform();
        }
        setMenuMetricsOrientation(0);
        drawToastIfNeeded();
        return;
    }

    if (m_customLayoutEditorVisible)
    {
        setMenuMetricsOrientation(0);
        const float progress = customLayoutEditorProgress();
        if (progress > 0.0f)
        {
            const bool transformed = pushMenuOrientationTransform(0);
            drawCustomLayoutSidebar(m_display.customLayout, m_customLayoutFocus, progress, progress);
            if (transformed)
                Gfx::PopDrawTransform();
        }
        setMenuMetricsOrientation(0);
        drawToastIfNeeded();
        return;
    }

    if (m_overlaySidebarVisible)
    {
        setMenuMetricsOrientation(0);
        const float progress = m_overlaySidebarClosing
            ? 1.0f - easeOutCubic(animationProgress(m_overlaySidebarAnimStartTick, kSidebarAnimationMs))
            : easeOutCubic(animationProgress(m_overlaySidebarAnimStartTick, kSidebarAnimationMs));
        if (progress > 0.0f)
        {
            const bool transformed = pushMenuOrientationTransform(0);
            drawOverlaySidebar(m_display, m_overlaySidebarFocus, progress, progress);
            if (transformed)
                Gfx::PopDrawTransform();
        }
        setMenuMetricsOrientation(0);
        drawToastIfNeeded();
        return;
    }

    if (m_shaderSidebarVisible)
    {
        setMenuMetricsOrientation(0);
        const float progress = m_shaderSidebarClosing
            ? 1.0f - easeOutCubic(animationProgress(m_shaderSidebarAnimStartTick, kSidebarAnimationMs))
            : easeOutCubic(animationProgress(m_shaderSidebarAnimStartTick, kSidebarAnimationMs));
        if (progress > 0.0f)
        {
            const bool transformed = pushMenuOrientationTransform(0);
            drawShaderSidebar(m_display, m_shaderSidebarFocus, smoothedShaderParamScroll(), progress, progress);
            if (m_shaderListVisible)
            {
                drawShaderListOverlay(MgbaShaderListEntries(m_shaderListPath),
                                      m_shaderListPath,
                                      m_display.mgbaShaderType,
                                      m_shaderListFocus,
                                      smoothedShaderListScroll(),
                                      progress);
            }
            if (transformed)
                Gfx::PopDrawTransform();
        }
        setMenuMetricsOrientation(0);
        drawToastIfNeeded();
        return;
    }

    setMenuMetricsOrientation(0);
    const float panel = panelProgress();
    if (panel <= 0.0f)
    {
        setMenuMetricsOrientation(0);
        return;
    }

    const float slideY = (1.0f - panel) * kScreenH;
    const float selectionProgress = m_selectionAnimating
        ? animationProgress(m_selectionAnimStartTick, 180.0f)
        : 1.0f;
    const float pageProgress = m_selectionAnimating
        ? animationProgress(m_selectionAnimStartTick, 180.0f)
        : 1.0f;

    const bool contentFocused = m_focusScope == FocusScope::Content;
    const Item currentItem = static_cast<Item>(m_selected);
    const bool canDelete = contentFocused &&
        (currentItem == Item::SaveState || currentItem == Item::LoadState) &&
        m_contentFocus >= 0 && m_contentFocus < static_cast<int>(m_slots.size()) &&
        m_slots[m_contentFocus].exists;
    const bool transformed = pushMenuOrientationTransform(0);
    ensureStateSlotTextures();
    drawOverlay(panel);
    drawHeader(slideY);
    drawLeftMenu(m_selected, m_previousSelected, selectionProgress, !contentFocused, slideY);
    drawLine({kSeparatorX, menuMetrics().separatorY + slideY},
             {1.0f, menuMetrics().separatorH},
             {1.0f, 1.0f, 1.0f, 0.08f});
    drawTabFrame(static_cast<Item>(m_selected),
                 static_cast<Item>(m_previousSelected),
                 pageProgress,
                 m_display,
                 m_slots,
                 m_cheats,
                 visibleCheatIndices(),
                 m_contentFocus,
                 contentFocused,
                 smoothedContentScrollY(),
                 m_statePreviewTexture,
                 m_statePreviewWidth,
                 m_statePreviewHeight,
                 m_statePreviewAttempted,
                 slideY);
    drawFooter(contentFocused, canDelete, slideY);
    if (m_deleteDialogVisible)
        drawDeleteDialog(m_deleteSlot, panel);
    if (m_syncConfirmVisible)
        drawSyncConfirmDialog(m_syncConfirmAction, panel);
    if (m_syncResultVisible)
        drawSyncResultDialog(m_syncResultAction, m_syncResultCount, panel);
    if (transformed)
        Gfx::PopDrawTransform();
    setMenuMetricsOrientation(0);
    drawToastIfNeeded();
}

} // namespace beiklive::mgba_stub
