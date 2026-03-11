// passthrough.glsl – BeikLive 示例着色器
// 直通着色器：原样输出游戏画面，不做任何颜色处理。
// 可用于测试着色器链功能是否正常工作。
//
// 使用方法: 在配置文件中设置
//   shader.preset=resources/shaders/passthrough.glsl

#pragma stage vertex
varying vec2 vTexCoord;
void main() {
    gl_Position = MVPMatrix * vec4(VertexCoord, 0.0, 1.0);
    vTexCoord   = TexCoord;
}

#pragma stage fragment
varying vec2 vTexCoord;
void main() {
    gl_FragColor = texture2D(Texture, vTexCoord);
}
