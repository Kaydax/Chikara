#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec2 vNoteSize;
layout(location = 3) in float winWidth;
layout(location = 4) in float winHeight;

layout(location = 0) out vec4 outColor;

const float PI = 3.1415926535897932384626;
const float border = 0.0015;

void main() {
  if (vNoteSize.y == 0.0) {
    // uninitialized outColor doesn't matter if not rendering anything
    return;
  }

  vec2 vUV = fragTexCoord;
  float t = (sin(vUV.x * PI) + 1) / 2;
  vec3 color = vec3(fragColor * t);
  float aspect = winHeight / winWidth;

  if(vUV.x < border / vNoteSize.x || vUV.x > 1.0 - border / vNoteSize.x || vUV.y < border / vNoteSize.y * aspect || vUV.y > 1.0 - border / vNoteSize.y * aspect)
  {
    color = vec3(color * 0.5);
  }
  outColor = vec4(color, 1.0);
  //texture(texSampler, fragTexCoord * 2.0) *
}
