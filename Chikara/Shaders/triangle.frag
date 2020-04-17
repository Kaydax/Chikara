#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec2 vNoteSize;

layout(location = 0) out vec4 outColor;

const float PI = 3.1415926535897932384626;
const float border = 0.001;

void main() {
  vec2 vUV = fragTexCoord;
  float t = sin(vUV.x * PI);
  vec3 color = vec3(fragColor * t);
  if(vUV.x < border / vNoteSize.x || vUV.x > 1.0 - border / vNoteSize.x || vUV.y < border / vNoteSize.y || vUV.y > 1.0 - border / vNoteSize.y)
  {
    color = vec3(color * 0.2);
  }
  outColor = vec4(color, 1.0);
  //texture(texSampler, fragTexCoord * 2.0) *
}
