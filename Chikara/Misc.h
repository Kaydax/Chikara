#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

uint32_t encode_color(glm::vec3 col);
glm::vec3 decode_color(uint32_t col);