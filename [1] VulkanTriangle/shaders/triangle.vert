#version 450

const vec3[] colors = {
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
};

const vec3[] positions = {
    vec3(-0.5, 0.5, 0.0),
    vec3(0.0, -0.5, 0.0),
    vec3(0.5, 0.5, 0.0),
};

layout(location = 0) out vec3 outColor;


void main()
{
    outColor = colors[gl_VertexIndex];
    gl_Position = vec4(positions[gl_VertexIndex], 1.0);
}