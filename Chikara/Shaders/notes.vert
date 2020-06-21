#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    float pre_time;
    float keyboard_height;
    float width;
    float height;
} ubo;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoord;
//layout(location = 2) in vec3 inColor;

layout(location = 2) in float noteStart;
layout(location = 3) in float noteEnd;
layout(location = 4) in int noteKey;
layout(location = 5) in uint noteColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec2 vNoteSize;
layout(location = 3) out float winWidth;
layout(location = 4) out float winHeight;

const int N_NOTES = 128;
const float blackWidth = 0.6;
const int whiteMap[12] = int[12](0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 7);
const bool isBlackKey[12] = bool[12](false, true, false, true, false, false, true, false, true, false, true, false);
const int nWhiteKeys = N_NOTES / 12 * 7 + whiteMap[(N_NOTES % 12)];

// Generated with
/*
#include <stdio.h>

const int N_NOTES = 128;
const float blackWidth = 0.6;
const float blackKey3Offset = 0.5f;
const float blackKey2Offset = 0.3f;
const int whiteMap[12] = {0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 7};
const bool isBlackKey[12] = {false, true, false, true, false, false, true, false, true, false, true, false};
const int blackOffset[12] = {0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5};
const float blackXOffset[12] = {0.0f, blackWidth / 2 * blackKey2Offset, 0.0f, -(blackWidth / 2 * blackKey2Offset), 0.0f, 0.0f, blackWidth / 2 * blackKey3Offset, 0.0f, 0.0f, 0.0f, -(blackWidth / 2 * blackKey3Offset), 0.0f};
const int nWhiteKeys = N_NOTES / 12 * 7 + whiteMap[(N_NOTES % 12)];

int main() {
  printf("const float xTable[%d] = float[%d](", N_NOTES, N_NOTES);
  for (int note = 0; note < N_NOTES; note++) {
    int octave = note / 12;
    int note2 = note % 12;
    int note3 = note2;
    note2 -= blackOffset[note2];
    float boff = isBlackKey[note3] ? -blackWidth / 2.0 - blackXOffset[note3] : 0.0;
    float x = (float(note2) + float(octave) * 7.0 + boff) / float(nWhiteKeys);
    printf("%f", x);
    if (note != N_NOTES - 1)
      printf(",");
  }
  printf(");\n");
  return 0;
}
*/
const float xTable[128] = float[128](0.000000,0.008133,0.013333,0.023867,0.026667,0.040000,0.047333,0.053333,0.062667,0.066667,0.078000,0.080000,0.093333,0.101467,0.106667,0.117200,0.120000,0.133333,0.140667,0.146667,0.156000,0.160000,0.171333,0.173333,0.186667,0.194800,0.200000,0.210533,0.213333,0.226667,0.234000,0.240000,0.249333,0.253333,0.264667,0.266667,0.280000,0.288133,0.293333,0.303867,0.306667,0.320000,0.327333,0.333333,0.342667,0.346667,0.358000,0.360000,0.373333,0.381467,0.386667,0.397200,0.400000,0.413333,0.420667,0.426667,0.436000,0.440000,0.451333,0.453333,0.466667,0.474800,0.480000,0.490533,0.493333,0.506667,0.514000,0.520000,0.529333,0.533333,0.544667,0.546667,0.560000,0.568133,0.573333,0.583867,0.586667,0.600000,0.607333,0.613333,0.622667,0.626667,0.638000,0.640000,0.653333,0.661467,0.666667,0.677200,0.680000,0.693333,0.700667,0.706667,0.716000,0.720000,0.731333,0.733333,0.746667,0.754800,0.760000,0.770533,0.773333,0.786667,0.794000,0.800000,0.809333,0.813333,0.824667,0.826667,0.840000,0.848133,0.853333,0.863867,0.866667,0.880000,0.887333,0.893333,0.902667,0.906667,0.918000,0.920000,0.933333,0.941467,0.946667,0.957200,0.960000,0.973333,0.980667,0.986667);

void main() {
    // TODO: test if an early return is actually faster
    /*
    if (noteStart == noteEnd) {
        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        vNoteSize = vec2(0.0, 0.0);
        return;
    }
    */
    vec2 vtx_pos;
    float flotes = float(N_NOTES); //float notes
    bool black = isBlackKey[noteKey % 12];
    vtx_pos.y = mix(noteStart, noteEnd, gl_VertexIndex > 1) - ubo.time;
    vtx_pos.y /= ubo.pre_time;
    vtx_pos.y += ubo.keyboard_height;
    vtx_pos.x = inPos.x / float(nWhiteKeys);
    vtx_pos.x *= mix(1.0, blackWidth, black); //make thinner if black
    vtx_pos.x += xTable[noteKey];
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vtx_pos, 0.0, 1.0);
    fragColor = vec3(float(noteColor & 0xFF) / 256.0f, float((noteColor >> 8) & 0xFF) / 256.0f, float((noteColor >> 16) & 0xFF) / 256.0f);
    fragTexCoord = inTexCoord;
    vNoteSize = vec2(mix(1.0, blackWidth, black) / float(nWhiteKeys), noteEnd - noteStart);
    winWidth = ubo.width;
    winHeight = ubo.height;
}
