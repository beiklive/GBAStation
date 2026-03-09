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
// 文件作用域音频状态
// ============================================================
struct StereoSample { int16_t left; int16_t right; };
static int    audioBufferActive = 0;
static int    enqueuedBuffers   = 0;
static StereoSample audioBuffer[N_BUFFERS][BUFFER_SIZE / 4]
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

// 供 libretro 音频回调写入当前 Switch 音频输出缓冲区
// samples: 交错 int16_t 立体声，count: 帧数
static void _pushAudio(const int16_t* samples, size_t count) {
#ifdef __SWITCH__
    _audioWait(0);
    while (enqueuedBuffers >= N_BUFFERS - 1) {
        if (!frameLimiter) return;
        _audioWait(10000000);
    }
    if (enqueuedBuffers >= N_BUFFERS) return;

    size_t toCopy = (count < SAMPLES) ? count : SAMPLES;
    StereoSample* buf = audioBuffer[audioBufferActive];
    for (size_t i = 0; i < toCopy; ++i) {
        buf[i].left  = samples[i * 2];
        buf[i].right = samples[i * 2 + 1];
    }
    audoutBuffer[audioBufferActive].data_size = toCopy * 4;
    audoutAppendAudioOutBuffer(&audoutBuffer[audioBufferActive]);
    ++audioBufferActive;
    audioBufferActive %= N_BUFFERS;
    ++enqueuedBuffers;
#else
    (void)samples; (void)count;
#endif
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

    // TODO: libretro 卸载 ROM
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

    // TODO: 通过 libretro 接口加载 ROM
    // 1. dlopen/加载 libretro 核心 .nro/.so
    // 2. retro_init()
    // 3. retro_load_game() 传入 m_gamePath
    // 4. 从 retro_system_av_info 获取 videoW/videoH 和音频采样率
    brls::Logger::warning("[GameRuntime] loadGame: libretro backend not yet implemented");
    m_wantsExit = false;

    m_softBuffer = new color_t[256 * 256]();

    reloadDisplayConfig();
    if (gameRunner && gameRunner->settingConfig)
        m_display.save(*gameRunner->settingConfig);

    m_gameLoaded = true;
    brls::Logger::info("[GameRuntime] loadGame stub success");
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
    if (ffHeld == m_frameLimiter) {
        setFrameLimiter(!ffHeld);
    }

    // TODO: 将 padkeys 转换为 libretro 按键掩码后传给 retro_set_input_state 回调
    (void)gameKeys;
#endif

    // ----------------------------------------------------------------
    // 推进模拟器
    // TODO: 调用 retro_run() 以推进一帧；libretro video_refresh 回调
    //       将新帧写入 m_softBuffer
    // ----------------------------------------------------------------
    unsigned runsThisFrame = m_frameLimiter ? 1u : m_framecap;
    (void)runsThisFrame;

    // ---- 固定 GBA 游戏分辨率 ----
    unsigned videoW = 240, videoH = 160;

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