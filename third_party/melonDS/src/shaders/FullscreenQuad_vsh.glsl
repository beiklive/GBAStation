#version 460

const vec4 Positions[4] = vec4[]
(
    vec4(-1.0, -1.0, 0.5, 1.0),
    vec4(1.0, -1.0, 0.5, 1.0),
    vec4(-1.0, 1.0, 0.5, 1.0),
    vec4(1.0, 1.0, 0.5, 1.0)
);

void main()
{
    gl_Position = Positions[gl_VertexID];
}