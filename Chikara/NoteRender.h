#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <optional>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <array>

#include "Renderer.h"
#include "Midi.h"

using namespace std;

#define BUFFERS_PER_THREAD 8
#define BUFFER_CAPACITY 2048

class NoteRender
{
public:
  NoteRender(Renderer* parent, uint32_t thread_count);
  ~NoteRender();
  void generateWorkflow();
  void renderFrame(list<Note*>** note_buffer);
private:
  uint32_t thread_count;
  Renderer* parent;
  VkBuffer* vertex_buffers;
  VkDeviceMemory* vertex_buffers_mem;
  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_mem;
  VkCommandBuffer* command_buffers;
};

