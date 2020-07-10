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
//const float xTable[256] = float[256](0.000000,0.004094,0.006711,0.012013,0.013423,0.020134,0.023826,0.026846,0.031544,0.033557,0.039262,0.040268,0.046980,0.051074,0.053691,0.058993,0.060403,0.067114,0.070805,0.073826,0.078523,0.080537,0.086242,0.087248,0.093960,0.098054,0.100671,0.105973,0.107383,0.114094,0.117785,0.120805,0.125503,0.127517,0.133221,0.134228,0.140940,0.145034,0.147651,0.152953,0.154362,0.161074,0.164765,0.167785,0.172483,0.174497,0.180201,0.181208,0.187919,0.192013,0.194631,0.199933,0.201342,0.208054,0.211745,0.214765,0.219463,0.221477,0.227181,0.228188,0.234899,0.238993,0.241611,0.246913,0.248322,0.255034,0.258725,0.261745,0.266443,0.268456,0.274161,0.275168,0.281879,0.285973,0.288591,0.293893,0.295302,0.302013,0.305705,0.308725,0.313423,0.315436,0.321141,0.322148,0.328859,0.332953,0.335570,0.340872,0.342282,0.348993,0.352685,0.355705,0.360403,0.362416,0.368121,0.369128,0.375839,0.379933,0.382550,0.387852,0.389262,0.395973,0.399664,0.402685,0.407383,0.409396,0.415101,0.416107,0.422819,0.426913,0.429530,0.434832,0.436242,0.442953,0.446644,0.449664,0.454362,0.456376,0.462081,0.463087,0.469799,0.473893,0.476510,0.481812,0.483221,0.489933,0.493624,0.496644,0.501342,0.503356,0.509060,0.510067,0.516779,0.520872,0.523490,0.528792,0.530201,0.536913,0.540604,0.543624,0.548322,0.550336,0.556040,0.557047,0.563758,0.567852,0.570470,0.575772,0.577181,0.583893,0.587584,0.590604,0.595302,0.597315,0.603020,0.604027,0.610738,0.614832,0.617450,0.622752,0.624161,0.630872,0.634564,0.637584,0.642282,0.644295,0.650000,0.651007,0.657718,0.661812,0.664430,0.669732,0.671141,0.677852,0.681544,0.684564,0.689262,0.691275,0.696980,0.697987,0.704698,0.708792,0.711409,0.716711,0.718121,0.724832,0.728523,0.731544,0.736242,0.738255,0.743960,0.744966,0.751678,0.755772,0.758389,0.763691,0.765101,0.771812,0.775503,0.778524,0.783221,0.785235,0.790940,0.791946,0.798658,0.802752,0.805369,0.810671,0.812081,0.818792,0.822483,0.825503,0.830201,0.832215,0.837919,0.838926,0.845638,0.849732,0.852349,0.857651,0.859060,0.865772,0.869463,0.872483,0.877181,0.879195,0.884899,0.885906,0.892617,0.896711,0.899329,0.904631,0.906040,0.912752,0.916443,0.919463,0.924161,0.926175,0.931879,0.932886,0.939597,0.943691,0.946309,0.951611,0.953020,0.959732,0.963423,0.966443,0.971141,0.973154,0.978859,0.979866,0.986577,0.990671,0.993289,0.998591);

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
    fragColor = vec3(float(noteColor & 0xFF) / 255.0f, float((noteColor >> 8) & 0xFF) / 255.0f, float((noteColor >> 16) & 0xFF) / 255.0f);
    fragTexCoord = inTexCoord;
    vNoteSize = vec2(mix(1.0, blackWidth, black) / float(nWhiteKeys), noteEnd - noteStart);
    winWidth = ubo.width;
    winHeight = ubo.height;
}
