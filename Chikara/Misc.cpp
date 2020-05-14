#include "Misc.h"

uint32_t encode_color(glm::vec3& col) {
  return (((uint32_t)(col.r * 255) << 24) & 0xff000000) |
    (((uint32_t)(col.g * 255) << 16) & 0xff0000) |
    (((uint32_t)(col.b * 255) << 8) & 0xff00) |
    0xFF;
}

glm::vec3 decode_color(uint32_t col) {
  return {
    static_cast<float>((col >> 24) & 0xFF) / 255.0,
    static_cast<float>((col >> 16) & 0xFF) / 255.0,
    static_cast<float>((col >> 8) & 0xFF) / 255.0
  };
}