// sharpen.glsl – BeikLive 示例着色器
// 简单锐化滤镜：通过 Unsharp Mask (USM) 算法增强游戏画面边缘细节。
//
// 使用方法: 在配置文件中设置
//   shader.preset=resources/shaders/sharpen.glsl
//
// 参数说明：
//   SHARPEN_STRENGTH  – 锐化强度，0.0 无效果，>0 增强边缘

#pragma stage vertex
varying vec2 vTexCoord;
void main() {
    gl_Position = MVPMatrix * vec4(VertexCoord, 0.0, 1.0);
    vTexCoord   = TexCoord;
}

#pragma stage fragment
uniform vec4 SourceSize;   // xy = 源纹理分辨率，zw = 1/分辨率

#define SHARPEN_STRENGTH 0.4

varying vec2 vTexCoord;
void main() {
    vec2 d = SourceSize.zw;  // 单像素偏移量

    // 采样中心及四个相邻像素
    vec4 center = texture2D(Texture, vTexCoord);
    vec4 left   = texture2D(Texture, vTexCoord + vec2(-d.x,  0.0));
    vec4 right  = texture2D(Texture, vTexCoord + vec2( d.x,  0.0));
    vec4 up     = texture2D(Texture, vTexCoord + vec2( 0.0, -d.y));
    vec4 down   = texture2D(Texture, vTexCoord + vec2( 0.0,  d.y));

    // 拉普拉斯边缘检测
    vec4 laplacian = center * 4.0 - (left + right + up + down);

    // 叠加到原图（Unsharp Mask）
    gl_FragColor = vec4(clamp(center.rgb + SHARPEN_STRENGTH * laplacian.rgb, 0.0, 1.0), 1.0);
}
