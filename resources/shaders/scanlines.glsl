// scanlines.glsl – BeikLive 示例着色器
// 模拟 CRT 扫描线效果：每隔一行略微压暗，产生水平扫描线纹理。
//
// 使用方法: 在配置文件中设置
//   shader.preset=resources/shaders/scanlines.glsl
//
// 原理：
//   通过 mod(floor(vTexCoord.y * OutputSize.y), 2.0) 判断当前像素
//   是否处于"暗行"，若是则将亮度乘以 SCANLINE_DARK 系数。

#pragma stage vertex
varying vec2 vTexCoord;
void main() {
    gl_Position = MVPMatrix * VertexCoord;
    vTexCoord   = TexCoord.xy;
}

#pragma stage fragment
uniform vec4 OutputSize;   // xy = 输出分辨率，zw = 1/分辨率

// 扫描线暗行亮度系数 (0.0 = 全黑, 1.0 = 无效果)
#define SCANLINE_DARK 0.5

varying vec2 vTexCoord;
void main() {
    vec4 color = texture2D(Texture, vTexCoord);

    // 计算当前像素对应的输出行号
    float line = mod(floor(vTexCoord.y * OutputSize.y), 2.0);
    // 偶数行正常，奇数行变暗
    float factor = (line < 1.0) ? 1.0 : SCANLINE_DARK;

    gl_FragColor = vec4(color.rgb * factor, color.a);
}
