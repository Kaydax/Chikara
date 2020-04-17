#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
} ubo;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 3) in float noteStart;
layout(location = 4) in float noteEnd;
layout(location = 5) in int noteKey;
layout(location = 6) in vec3 noteColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    vec2 vtx_pos;
    // any other cases should be impossible
    switch (gl_VertexIndex) {
    case 0:
        vtx_pos.x = float(noteKey) / 256.0;
        vtx_pos.y = noteStart - ubo.time;
        break;
    case 1:
        vtx_pos.x = (float(noteKey) / 256.0) + (1 / 256.0);
        vtx_pos.y = noteStart - ubo.time;
        break;
    case 2:
        vtx_pos.x = (float(noteKey) / 256.0) + (1 / 256.0);
        vtx_pos.y = noteEnd - ubo.time;
        break;
    case 3:
        vtx_pos.x = float(noteKey) / 256.0;
        vtx_pos.y = noteEnd - ubo.time;
        break;
    }
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vtx_pos, 0.0, 1.0);
    fragColor = inColor * noteColor;
    fragTexCoord = inTexCoord;
}
