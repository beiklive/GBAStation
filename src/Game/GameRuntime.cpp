#include "Game/GameRuntime.hpp"

#ifdef __SWITCH__

// ---- NanoVG 后端检测（必须在 nanovg_gl.h 之前引入）----
#ifdef BOREALIS_USE_OPENGL
#  ifdef USE_GLES3
#    define NANOVG_GLES3
#  elif defined(USE_GLES2)
#    define NANOVG_GLES2
#  elif defined(USE_GL2)
#    define NANOVG_GL2
#  else
#    define NANOVG_GL3
#  endif
#  include <borealis/extern/nanovg/nanovg_gl.h>
#endif

#include <cstring>

// ---- 模拟器常量 ----
#define AUTO_INPUT    0x4E585031
#define SAMPLES       0x200          // 每次音频回调 512 个采样
#define N_BUFFERS     4
#define ANALOG_DEADZONE 0x4000

// 音频缓冲区大小：>= SAMPLES*4 字节 且 >= 4096 字节
#if (SAMPLES * 4) < 0x1000
#  define BUFFER_SIZE 0x1000
#else
#  define BUFFER_SIZE (SAMPLES * 4)
#endif

// ============================================================
// mGBA 输入平台信息（导出符号）
// ============================================================
MGBA_EXPORT const struct mInputPlatformInfo GBAInputInfo = {
    .platformName = "gba",
    .keyId = (const char*[]) {
        "A", "B", "Select", "Start",
        "Right", "Left", "Up", "Down",
        "R", "L"
    },
    .nKeys = GBA_KEY_MAX,
    .hat = {
        .up    = GBA_KEY_UP,
        .right = GBA_KEY_RIGHT,
        .down  = GBA_KEY_DOWN,
        .left  = GBA_KEY_LEFT
    }
};

// ============================================================
// 文件作用域音频状态（供 C 回调访问）
// ============================================================
static struct mAVStream    stream;
static struct mRotationSource rotation = {0};
static int    audioBufferActive = 0;
static int    enqueuedBuffers   = 0;
static float  gyroZ = 0;
static struct mStereoSample audioBuffer[N_BUFFERS][BUFFER_SIZE / 4]
    __attribute__((__aligned__(0x1000)));

#ifdef __SWITCH__
static AudioOutBuffer audoutBuffer[N_BUFFERS];
static bool frameLimiter = true;

static int _audioWait(u64 timeout) {
    AudioOutBuffer* releasedBuffers = nullptr;
    u32 nReleased = 0;
    Result rc = timeout
        ? audoutWaitPlayFinish(&releasedBuffers, &nReleased, timeout)
        : audoutGetReleasedAudioOutBuffer(&releasedBuffers, &nReleased);
    if (R_FAILED(rc)) return 0;
    enqueuedBuffers -= nReleased;
    return nReleased;
}
#endif // __SWITCH__

static void _postAudioBuffer(struct mAVStream* /*s*/, blip_t* left, blip_t* right) {
#ifdef __SWITCH__
    _audioWait(0);
    // 只有在所有 N_BUFFERS 个槽都被占满时才阻塞等待，
    // 而不是在 N_BUFFERS-1 时就阻塞，充分利用缓冲空间以免频繁卡顿。
    while (enqueuedBuffers >= N_BUFFERS - 1) {
        if (!frameLimiter) { blip_clear(left); blip_clear(right); return; }
        _audioWait(10000000);
    }
    if (enqueuedBuffers >= N_BUFFERS) {
		blip_clear(left);
		blip_clear(right);
		return;
	}


    struct mStereoSample* samples = audioBuffer[audioBufferActive];
    blip_read_samples(left,  &samples[0].left,  SAMPLES, true);
    blip_read_samples(right, &samples[0].right, SAMPLES, true);
    audoutAppendAudioOutBuffer(&audoutBuffer[audioBufferActive]);
    ++audioBufferActive;
    audioBufferActive %= N_BUFFERS;
    ++enqueuedBuffers;
#else
    blip_clear(left);
    blip_clear(right);
#endif
}

// ============================================================
// 亮度传感器回调
// ============================================================
static void _updateLux(struct GBALuminanceSource* /*lux*/) {}

static uint8_t _readLux(struct GBALuminanceSource* lux) {
    if (!lux) return static_cast<uint8_t>(0xFF - 0x16);
    auto* runnerLux = reinterpret_cast<decltype(&gameRunner->luminanceSource)>(lux);
    int value = 0x16;
    if (runnerLux->luxLevel > 0)
        value += GBA_LUX_LEVELS[runnerLux->luxLevel - 1];
    return static_cast<uint8_t>(0xFF - value);
}
// ============================================================
// ROM 加载进度回调
// ============================================================
static void _updateLoading(size_t read, size_t size, void* /*ctx*/) {
    static size_t lastPercent = static_cast<size_t>(-1);
    if (!size) return;
    size_t pct = (read * 100) / size;
    if (pct != lastPercent && (pct % 10 == 0 || pct == 100)) {
        brls::Logger::debug("[GameRuntime] Loading ROM: {}%", pct);
        lastPercent = pct;
    }
}
// ============================================================
// 按键映射辅助函数
// ============================================================
static void _mapKey(struct mInputMap* map, uint32_t binding, int nativeKey, int key) {
    mInputBindKey(map, binding, __builtin_ctz(nativeKey), key);
}

// ============================================================
// 核心初始化回调
// ============================================================
static void _setup() {
    if (!gameRunner || !gameRunner->core) {
        brls::Logger::error("[GameRuntime] _setup: core is null");
        return;
    }
#ifdef __SWITCH__
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_A,     GBA_KEY_A);
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_B,     GBA_KEY_B);
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_Plus,  GBA_KEY_START);
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_Minus, GBA_KEY_SELECT);
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_Up,    GBA_KEY_UP);
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_Down,  GBA_KEY_DOWN);
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_Left,  GBA_KEY_LEFT);
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_Right, GBA_KEY_RIGHT);
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_L,     GBA_KEY_L);
    _mapKey(&gameRunner->core->inputMap, AUTO_INPUT, HidNpadButton_R,     GBA_KEY_R);
#endif
    gameRunner->core->setAVStream(gameRunner->core, &stream);
    gameRunner->core->setAudioBufferSize(gameRunner->core, SAMPLES);



}

static void _gameLoaded() {
    if (!gameRunner || !gameRunner->core) {
        brls::Logger::error("[GameRuntime] _gameLoaded: core is null");
        return;
    }
#ifdef __SWITCH__
    uint32_t samplerate = audoutGetSampleRate();
#else
    uint32_t samplerate = 44100;
#endif
    // 使用 60.0 与 main.c 保持一致，匹配 Borealis 60 Hz vsync 速率。
    // 若使用 GBA 真实帧率（≈59.7275 Hz），blip 每帧会多产出约 2-3 个采样，
    // 导致音频队列持续积压，_postAudioBuffer 在渲染线程上阻塞等待，
    // 造成 vsync 丢失和游戏变慢（音频拖慢）。
    double ratio = GBAAudioCalculateRatio(1, 60.0, 1);
    blip_set_rates(gameRunner->core->getAudioChannel(gameRunner->core, 0),
                   gameRunner->core->frequency(gameRunner->core), samplerate * ratio);
    blip_set_rates(gameRunner->core->getAudioChannel(gameRunner->core, 1),
                   gameRunner->core->frequency(gameRunner->core), samplerate * ratio);
    brls::Logger::debug("[GameRuntime] Audio configured: rate={}, ratio={}", samplerate, ratio);
}

// ============================================================
// 静态成员
// ============================================================
namespace beiklive {
color_t* GameRuntime::frameBuffer = nullptr;

// ============================================================
// 构造函数 / 析构函数
// ============================================================
GameRuntime::GameRuntime(const std::string& gamePath)
    : m_gamePath(gamePath)
{
    brls::Logger::info("[GameRuntime] Constructing, path={}", m_gamePath);

    // 亮度传感器
    gameRunner->luminanceSource.d.readLuminance = _readLux;
    gameRunner->luminanceSource.d.sample        = _updateLux;
    gameRunner->luminanceSource.luxLevel        = 0;

    // 音视频流
    stream.videoDimensionsChanged = nullptr;
    stream.postVideoFrame         = nullptr;
    stream.postAudioFrame         = nullptr;
    stream.postAudioBuffer        = _postAudioBuffer;

#ifdef __SWITCH__
    audoutInitialize();
    memset(audioBuffer, 0, sizeof(audioBuffer));
    audioBufferActive = 0;
    enqueuedBuffers   = 0;
    for (size_t i = 0; i < N_BUFFERS; ++i) {
        audoutBuffer[i].next        = nullptr;
        audoutBuffer[i].buffer      = audioBuffer[i];
        audoutBuffer[i].buffer_size = BUFFER_SIZE;
        audoutBuffer[i].data_size   = SAMPLES * 4;
        audoutBuffer[i].data_offset = 0;
    }
    audoutStartAudioOut();

    hidInitializeTouchScreen();
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&m_pad);
#endif

    glInit();
}

GameRuntime::~GameRuntime()
{
    brls::Logger::info("[GameRuntime] Destroying, path={}", m_gamePath);

    if (gameRunner->core)
        gameRunner->core->unloadROM(gameRunner->core);
    glDeinit();

#ifdef __SWITCH__
    audoutStopAudioOut();
    audoutExit();
#endif
}

// ============================================================
// loadGame：加载游戏
// ============================================================
bool GameRuntime::loadGame()
{
    brls::Logger::info("[GameRuntime] loadGame: {}", m_gamePath);

    gameRunner->core = mCoreFind(m_gamePath.c_str());
    if (!gameRunner->core) {
        brls::Logger::error("[GameRuntime] No core for: {}", m_gamePath);
        m_wantsExit = true;
        return false;
    }

    gameRunner->core->init(gameRunner->core);
    mCoreInitConfig(gameRunner->core, gameRunner->port);
    mInputMapInit(&gameRunner->core->inputMap, &GBAInputInfo);

    struct VFile* rom = mDirectorySetOpenPath(
        &gameRunner->core->dirs, m_gamePath.c_str(), gameRunner->core->isROM);

    bool found = mCorePreloadVFCB(gameRunner->core, rom, _updateLoading, gameRunner);
    if (!found) {
        if (rom) rom->close(rom);
        gameRunner->core->deinit(gameRunner->core);
        brls::Logger::error("[GameRuntime] Failed to load ROM: {}", m_gamePath);
        m_wantsExit = true;
        return false;
    }

    m_softBuffer = new color_t[256 * 256]();
    gameRunner->core->setVideoBuffer(gameRunner->core, m_softBuffer, 256);

    if (gameRunner->core->platform(gameRunner->core) == mPLATFORM_GBA) {
        gameRunner->core->setPeripheral(gameRunner->core, mPERIPH_GBA_LUMINANCE,
                                        &gameRunner->luminanceSource.d);
        gameRunner->gameFile.type = mPLATFORM_GBA;
    } else {
        gameRunner->gameFile.type = mPLATFORM_GB;
    }

    mCoreLoadForeignConfig(gameRunner->core, &gameRunner->config);
    mCoreAutoloadSave(gameRunner->core);
    mCoreAutoloadCheats(gameRunner->core);

    _setup();
    gameRunner->core->reset(gameRunner->core);
    _gameLoaded();

    reloadDisplayConfig();
    if (gameRunner && gameRunner->settingConfig)
        m_display.save(*gameRunner->settingConfig);

    m_gameLoaded = true;
    brls::Logger::info("[GameRuntime] loadGame success");
    return true;
}

// ============================================================
// OpenGL 初始化
// ============================================================
void GameRuntime::glInit()
{
    brls::Logger::debug("[GameRuntime] glInit");

    // 源纹理：256×256 RGBA，存储软件渲染器输出的原始像素数据
    glGenTextures(1, &m_srcTex);
    glBindTexture(GL_TEXTURE_2D, m_srcTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 使用适合当前版本的 GLSL 构建内置着色器链（第 0 通道）
    if (!m_chain.initBuiltin()) {
        brls::Logger::error("[GameRuntime] Shader chain init failed");
    }
}

// ============================================================
// OpenGL 资源释放
// ============================================================
void GameRuntime::glDeinit()
{
    brls::Logger::debug("[GameRuntime] glDeinit");

    _invalidateNvgImage();
    m_nvgCtx = nullptr;
    m_chain.deinit();

    if (m_srcTex) { glDeleteTextures(1, &m_srcTex); m_srcTex = 0; }

    delete[] m_softBuffer;
    m_softBuffer = nullptr;
}

// ============================================================
// 公开接口：添加 / 清除用户自定义着色器通道
// ============================================================
bool GameRuntime::addShaderPass(const std::string& vert, const std::string& frag)
{
    _invalidateNvgImage();   // FBO 重建后最终纹理将发生变化
    bool ok = m_chain.addPass(vert, frag);
    if (ok) brls::Logger::info("[GameRuntime] Shader pass added");
    return ok;
}

void GameRuntime::clearShaderPasses()
{
    _invalidateNvgImage();   // 最终纹理将回退到第 0 通道输出
    m_chain.clearPasses();
}

// ============================================================
// 公开接口：重新加载显示配置
// ============================================================
void GameRuntime::reloadDisplayConfig()
{
    if (gameRunner && gameRunner->settingConfig)
        m_display.load(*gameRunner->settingConfig);
}

// ============================================================
// 辅助函数：使包装着色器链输出的 NVG 图像失效
// ============================================================
void GameRuntime::_invalidateNvgImage()
{
    if (m_nvgImage > 0 && m_nvgCtx) {
        nvgDeleteImage(m_nvgCtx, m_nvgImage);
    }
    m_nvgImage = -1;
    m_nvgTex   = 0;
}

// ============================================================
// 桩实现（保留以备虚表或未来使用）
// ============================================================
void GameRuntime::mInputMapInit(struct mInputMap* map, const struct mInputPlatformInfo* info) {
    map->maps    = 0;
    map->numMaps = 0;
    map->info    = info;
}
void GameRuntime::_clearScreen() {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
}
void GameRuntime::_drawFrame() {}

// ============================================================
// setFrameLimiter – 启用/禁用快进模式（与 main.c 逻辑一致）
// ============================================================
void GameRuntime::setFrameLimiter(bool limit)
{
#ifdef __SWITCH__
    if (!m_frameLimiter && limit) {
        // 重新启用帧率限制器：排空音频缓冲区，防止音频杂音
        while (enqueuedBuffers > 2) {
            _audioWait(100000000);   // 超时 100 毫秒
        }
    }
    // 同步文件作用域变量：_postAudioBuffer 读它来决定是否丢帧
    frameLimiter = limit;
#endif
    m_frameLimiter = limit;
    brls::Logger::debug("[GameRuntime] frameLimiter={}", limit);
}

// ============================================================
// draw() – 每帧由 Borealis 调用
// ============================================================
void GameRuntime::draw(NVGcontext* vg, float x, float y,
                       float width, float height,
                       brls::Style /*style*/, brls::FrameContext* /*ctx*/)
{
    // 等游戏加载完成后再进行输入/模拟/渲染。
    bool loaded = m_gameLoaded;
    if (loaded) {

    // ----------------------------------------------------------------
    // 输入轮询（仅 Switch 平台）
    // 每显示帧调用一次 padUpdate()，提取：
    //   • 退出请求  – HidNpadButton_X  （取消键）
    //   • 快进      – 按住 ZR          （禁用帧率限制器）
    //   • 游戏按键  – 通过 mGBA 输入映射表
    // ----------------------------------------------------------------
    uint32_t gameKeys = 0;
#ifdef __SWITCH__
    padUpdate(&m_pad);
    u32 padkeys = padGetButtons(&m_pad);

    // 按 X 键退出游戏
    if (padkeys & HidNpadButton_X) {
        m_wantsExit = true;
    }

    // 按住 ZR 快进；松开时恢复正常速度
    bool ffHeld = (padkeys & HidNpadButton_ZR) != 0;
    if (ffHeld == m_frameLimiter) {   // 检测到状态切换
        setFrameLimiter(!ffHeld);
    }

    // 将硬件按键映射为 mGBA 按键位掩码
    gameKeys = mInputMapKeyBits(&gameRunner->core->inputMap,
                                AUTO_INPUT, padkeys, 0);

    // 左摇杆模拟十字键输入
    HidAnalogStickState js = padGetStickPos(&m_pad, 0);
    int kl = mInputMapKey(&gameRunner->core->inputMap, AUTO_INPUT, __builtin_ctz(HidNpadButton_StickLLeft));
    int kr = mInputMapKey(&gameRunner->core->inputMap, AUTO_INPUT, __builtin_ctz(HidNpadButton_StickLRight));
    int ku = mInputMapKey(&gameRunner->core->inputMap, AUTO_INPUT, __builtin_ctz(HidNpadButton_StickLUp));
    int kd = mInputMapKey(&gameRunner->core->inputMap, AUTO_INPUT, __builtin_ctz(HidNpadButton_StickLDown));
    if (kl == -1) kl = mInputMapKey(&gameRunner->core->inputMap, AUTO_INPUT, __builtin_ctz(HidNpadButton_Left));
    if (kr == -1) kr = mInputMapKey(&gameRunner->core->inputMap, AUTO_INPUT, __builtin_ctz(HidNpadButton_Right));
    if (ku == -1) ku = mInputMapKey(&gameRunner->core->inputMap, AUTO_INPUT, __builtin_ctz(HidNpadButton_Up));
    if (kd == -1) kd = mInputMapKey(&gameRunner->core->inputMap, AUTO_INPUT, __builtin_ctz(HidNpadButton_Down));
    if (js.x < -ANALOG_DEADZONE && kl != -1) gameKeys |= (1u << kl);
    if (js.x >  ANALOG_DEADZONE && kr != -1) gameKeys |= (1u << kr);
    if (js.y < -ANALOG_DEADZONE && kd != -1) gameKeys |= (1u << kd);
    if (js.y >  ANALOG_DEADZONE && ku != -1) gameKeys |= (1u << ku);
#endif

    // ----------------------------------------------------------------
    // 推进模拟器。
    // 普通模式：每显示帧执行 1 次 runFrame（约 60 fps）。
    // 快进模式：每显示帧执行 m_framecap 次 runFrame（×N 速度）。
    // 在所有 runFrame 调用前后保存/恢复 FBO 绑定，
    // 防止模拟器自身的 GL 渲染器（如有）破坏当前状态。
    // ----------------------------------------------------------------
    unsigned runsThisFrame = m_frameLimiter ? 1u : m_framecap;

    {
        GLint savedFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &savedFbo);
        for (unsigned i = 0; i < runsThisFrame; ++i) {
            gameRunner->core->setKeys(gameRunner->core, gameKeys);
            gameRunner->core->runFrame(gameRunner->core);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)savedFbo);
    }

    // ---- 查询实际游戏分辨率 ----
    unsigned videoW = 240, videoH = 160;
    gameRunner->core->desiredVideoDimensions(gameRunner->core, &videoW, &videoH);

    // ---- 若过滤模式发生变化，更新源纹理参数并重建 NVG 图像 ----
    if (m_display.filterMode != m_activeFilter) {
        m_activeFilter = m_display.filterMode;
        GLenum glFilter = (m_activeFilter == FilterMode::Nearest) ? GL_NEAREST : GL_LINEAR;
        glBindTexture(GL_TEXTURE_2D, m_srcTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_chain.setFilter(glFilter);     // 同步更新 ShaderChain 的 FBO 输出纹理
        _invalidateNvgImage();           // NVG 图像需以新过滤标志重建
    }

    // ---- 将原始像素上传到源纹理 ----
    // m_softBuffer 行步长 = 256 px（通过 setVideoBuffer(..., 256) 设置）。
    // GL_UNPACK_ROW_LENGTH 告知 OpenGL 真实行间距，
    // 防止步长不匹配导致的图像斜移问题。
    if (m_softBuffer && videoW > 0 && videoH > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_srcTex);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        (GLsizei)videoW, (GLsizei)videoH,
                        GL_RGBA, GL_UNSIGNED_BYTE, m_softBuffer);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // ---- 执行着色器链 ----
    // ShaderChain::run() 内部会保存并恢复所有 GL 状态，
    // 保证外部的 NVG 上下文不被破坏。
    GLuint finalTex = m_chain.run(m_srcTex, videoW, videoH);

    // ---- 若输出纹理发生变化则（重新）创建 NVG 图像 ----
    // 首帧以及 FBO 重新分配时（分辨率变化或通道增删）均会触发
    if (finalTex != m_nvgTex) {
        _invalidateNvgImage();
    }
    if (m_nvgImage <= 0 && finalTex != 0) {
        m_nvgCtx = vg;
        int nvgW = (int)m_chain.outputW();
        int nvgH = (int)m_chain.outputH();
        int nvgFlags = NVG_IMAGE_NODELETE |
                       (m_activeFilter == FilterMode::Nearest ? NVG_IMAGE_NEAREST : 0);
#if defined(NANOVG_GLES3)
        m_nvgImage = nvglCreateImageFromHandleGLES3(vg, finalTex, nvgW, nvgH, nvgFlags);
#elif defined(NANOVG_GLES2)
        m_nvgImage = nvglCreateImageFromHandleGLES2(vg, finalTex, nvgW, nvgH, nvgFlags);
#elif defined(NANOVG_GL2)
        m_nvgImage = nvglCreateImageFromHandleGL2(vg, finalTex, nvgW, nvgH, nvgFlags);
#else
        m_nvgImage = nvglCreateImageFromHandleGL3(vg, finalTex, nvgW, nvgH, nvgFlags);
#endif
        m_nvgTex = finalTex;
        brls::Logger::debug("[GameRuntime] NVG image created: id={}, {}×{}",
                            m_nvgImage, nvgW, nvgH);
    }

    // ----------------------------------------------------------------
    // NVG 渲染
    // ----------------------------------------------------------------
    if (m_nvgImage > 0 && m_chain.outputW() > 0 && m_chain.outputH() > 0) {
        DisplayRect dr = m_display.computeRect(x, y, width, height,
                                               m_chain.outputW(), m_chain.outputH());

        NVGpaint paint = nvgImagePattern(vg, dr.x, dr.y, dr.w, dr.h, 0.f, m_nvgImage, 1.f);
        nvgBeginPath(vg);
        nvgRect(vg, dr.x, dr.y, dr.w, dr.h);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
    }

    }  // end if (loaded)
}

} // namespace beiklive

#endif // __SWITCH__