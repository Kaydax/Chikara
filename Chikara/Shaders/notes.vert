#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float time;
    float pre_time;
    float keyboard_height;
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

const int N_NOTES = 128;
const float blackWidth = 0.6;
const float blackKey3Offset = 0.5f;
const float blackKey2Offset = 0.3f;
const int whiteMap[12] = int[12](0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 7);
const bool isBlackKey[12] = bool[12](false, true, false, true, false, false, true, false, true, false, true, false);
const int blackOffset[12] = int[12](0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5);
const float blackXOffset[12] = float[12](0.0f, blackWidth / 2 * blackKey2Offset, 0.0f, -(blackWidth / 2 * blackKey2Offset), 0.0f, 0.0f, blackWidth / 2 * blackKey3Offset, 0.0f, 0.0f, 0.0f, -(blackWidth / 2 * blackKey3Offset), 0.0f);
const int nWhiteKeys = N_NOTES / 12 * 7 + whiteMap[(N_NOTES % 12)];

float getNoteOffset(int note)
{
  int octave = note / 12;

  int note2 = note % 12;
  int note3 = note2;
  note2 -= blackOffset[note2];
  float boff = isBlackKey[note3] ? -blackWidth / 2.0 - blackXOffset[note3] : 0.0;
  float x = (float(note2) + float(octave) * 7.0 + boff) / float(nWhiteKeys);

  return x;
}

void main() {
    if (noteStart == noteEnd) {
        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        vNoteSize = vec2(0.0, 0.0);
        return;
    }
    float fakeNoteEnd = noteEnd;
    if (noteEnd == uintBitsToFloat(0x7F800000))
      fakeNoteEnd = ubo.time + 1000;
    vec2 vtx_pos;
    float flotes = float(N_NOTES); //float notes
    bool black = isBlackKey[noteKey % 12];
    // any other cases should be impossible
    switch (gl_VertexIndex) {
    case 0:
    case 1:
        vtx_pos.y = noteStart - ubo.time;
        break;
    case 2:
    case 3:
        vtx_pos.y = fakeNoteEnd - ubo.time;
        break;
    }
    vtx_pos.y /= ubo.pre_time;
    vtx_pos.y += ubo.keyboard_height;
    vtx_pos.x = inPos.x / float(nWhiteKeys);
    vtx_pos.x *= black ? blackWidth : 1.0; //make thinner if black
    vtx_pos.x += getNoteOffset(noteKey);
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vtx_pos, 0.0, 1.0);
    fragColor = vec3(float(noteColor & 0xFF) / 256.0f, float((noteColor >> 8) & 0xFF) / 256.0f, float((noteColor >> 16) & 0xFF) / 256.0f);
    fragTexCoord = inTexCoord;
    vNoteSize = vec2((black ? blackWidth : 1.0) / float(nWhiteKeys), fakeNoteEnd - noteStart);
}
