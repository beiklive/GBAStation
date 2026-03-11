// crt_simple.glsl – BeikLive 示例着色器
// 简易 CRT 显示器模拟效果：
//   - 扫描线（偶数/奇数行交替暗/亮）
//   - 像素轻微晕染（水平 3-tap 模糊）
//   - 边缘暗角（Vignette）
//   - 轻微颜色溢出（Color Bleeding）
//
// 使用方法: 在配置文件中设置
//   shader.preset=resources/shaders/crt_simple.glsl

#pragma stage vertex
varying vec2 vTexCoord;
void main() {
    gl_Position = MVPMatrix * vec4(VertexCoord, 0.0, 1.0);
    vTexCoord   = TexCoord;
}

#pragma stage fragment
uniform vec4 SourceSize;   // xy=源分辨率, zw=1/源分辨率
uniform vec4 OutputSize;   // xy=输出分辨率

#define SCANLINE_DARK  0.55   // 暗行亮度 (0~1)
#define BLUR_AMOUNT    0.15   // 水平模糊系数
#define VIGNETTE_STR   0.25   // 暗角强度

varying vec2 vTexCoord;
void main() {
    vec2 d = SourceSize.zw;

    // -- 水平 3-tap 模糊（模拟荧光点扩散）--
    vec4 left   = texture2D(Texture, vTexCoord + vec2(-d.x, 0.0));
    vec4 center = texture2D(Texture, vTexCoord);
    vec4 right  = texture2D(Texture, vTexCoord + vec2( d.x, 0.0));
    vec4 color  = mix(center, (left + center + right) / 3.0, BLUR_AMOUNT);

    // -- 扫描线 --
    float line   = mod(floor(vTexCoord.y * OutputSize.y), 2.0);
    float scanFactor = (line < 1.0) ? 1.0 : SCANLINE_DARK;
    color.rgb *= scanFactor;

    // -- 暗角（Vignette）--
    // 将 UV 从 [0,1] 映射到 [-1,1]，计算到中心的距离
    vec2 uv = vTexCoord * 2.0 - 1.0;
    float vignette = 1.0 - dot(uv * vec2(0.5, 0.8), uv * vec2(0.5, 0.8));
    vignette = clamp(pow(vignette, 1.5), 0.0, 1.0);
    color.rgb *= mix(1.0 - VIGNETTE_STR, 1.0, vignette);

    gl_FragColor = vec4(color.rgb, 1.0);
}
