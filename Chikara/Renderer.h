#pragma once

#pragma region Includes

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
#include <stack>
#include "Midi.h"
#include "CustomList.h"

#pragma endregion

#define MAX_NOTES_MULT 10000 // 100,000,000
#define MAX_NOTES_BASE 10000
#define MAX_NOTES MAX_NOTES_BASE * MAX_NOTES_MULT
#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

#define NOTE_DEPTH_BUFFER_SIZE 2048

const uint32_t default_width = 1280;
const uint32_t default_height = 720;

const int max_frames_in_flight = 1;

const std::vector<const char*> validation_layers = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_exts = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

#pragma region Structs

typedef enum {
  Float,
  Double,
  Uint64,
} ImGuiStatType;

struct ImGuiStat
{
  ImGuiStatType type;
  const char* name;
  void* value;
  bool enabled;
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphics_fam;
  std::optional<uint32_t> present_fam;

  bool isComplete()
  {
    return graphics_fam.has_value() && present_fam.has_value();
  }
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

#define VERTEX_ATTRIB_COUNT 2

struct Vertex
{
  glm::vec2 pos;
  //glm::vec3 color;
  glm::vec2 tex_coord;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription binding_description;
    binding_description.binding = VERTEX_BUFFER_BIND_ID;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description;
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIB_COUNT> attrib_descriptions = {};
    attrib_descriptions[0].binding = VERTEX_BUFFER_BIND_ID;
    attrib_descriptions[0].location = 0;
    attrib_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrib_descriptions[0].offset = offsetof(Vertex, pos);

    attrib_descriptions[1].binding = VERTEX_BUFFER_BIND_ID;
    attrib_descriptions[1].location = 1;
    attrib_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrib_descriptions[1].offset = offsetof(Vertex, tex_coord);

    //attrib_descriptions[2].binding = VERTEX_BUFFER_BIND_ID;
    //attrib_descriptions[2].location = 2;
    //attrib_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    //attrib_descriptions[2].offset = offsetof(Vertex, color);

    return std::vector(attrib_descriptions.begin(), attrib_descriptions.end());
  }
};

// will probably never be used, but consistency is good
#define INSTANCE_ATTRIB_COUNT 4

struct InstanceData {
  float start;
  float end;
  int key;
  uint32_t color;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription binding_description = {};
    binding_description.binding = INSTANCE_BUFFER_BIND_ID;
    binding_description.stride = sizeof(InstanceData);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return binding_description;
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
  {
    // this will always be appended to Vertex, so locations are offset
    std::array<VkVertexInputAttributeDescription, INSTANCE_ATTRIB_COUNT> attrib_descriptions = {};
    attrib_descriptions[0].binding = INSTANCE_BUFFER_BIND_ID;
    attrib_descriptions[0].location = VERTEX_ATTRIB_COUNT + 0;
    attrib_descriptions[0].format = VK_FORMAT_R32_SFLOAT;
    attrib_descriptions[0].offset = offsetof(InstanceData, start);

    attrib_descriptions[1].binding = INSTANCE_BUFFER_BIND_ID;
    attrib_descriptions[1].location = VERTEX_ATTRIB_COUNT + 1;
    attrib_descriptions[1].format = VK_FORMAT_R32_SFLOAT;
    attrib_descriptions[1].offset = offsetof(InstanceData, end);

    attrib_descriptions[2].binding = INSTANCE_BUFFER_BIND_ID;
    attrib_descriptions[2].location = VERTEX_ATTRIB_COUNT + 2;
    attrib_descriptions[2].format = VK_FORMAT_R32_SINT;
    attrib_descriptions[2].offset = offsetof(InstanceData, key);

    attrib_descriptions[3].binding = INSTANCE_BUFFER_BIND_ID;
    attrib_descriptions[3].location = VERTEX_ATTRIB_COUNT + 3;
    attrib_descriptions[3].format = VK_FORMAT_R32_UINT;
    attrib_descriptions[3].offset = offsetof(InstanceData, color);

    return std::vector(attrib_descriptions.begin(), attrib_descriptions.end());
  }
};

struct UniformBufferObject
{
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  float time;
  float pre_time;
  float keyboard_height;
  float width;
  float height;
};

#pragma endregion

//const std::vector<Vertex> vertices = {
//  {{0,0},{1,1,1},{0,1}}, //0
//  {{1,0},{1,1,1},{1,1}}, //1
//  {{1,1},{1,1,1},{1,0}}, //2
//  {{0,1},{1,1,1},{0,0}} //3
//};

//const std::vector<uint32_t> indices = {
//  //Top
//  0, 1, 2,
//  2, 3, 0
//};

class Renderer
{
public:
  moodycamel::ReaderWriterQueue<NoteEvent>** note_event_buffer;
  std::vector<std::array<std::stack<Note*>, 256>> note_stacks;
  std::atomic<float>* midi_renderer_time;
  uint64_t* note_count;
  uint64_t* notes_played;
  uint64_t* nps;
  GLFWwindow* window;
  VkInstance inst;
  VkDebugUtilsMessengerEXT debug_msg;
  VkSurfaceKHR surface;

  VkPhysicalDevice pdevice = VK_NULL_HANDLE;
  VkDevice device;

  VkQueue graphics_queue;
  VkQueue present_queue;

  VkSwapchainKHR swap_chain;
  std::vector<VkImage> swap_chain_imgs;
  VkFormat swap_chain_img_format;
  VkExtent2D swap_chain_extent;
  std::vector<VkImageView> swap_chain_img_views;

  VkPipelineCache pipeline_cache;
  uint32_t image_count;

  // render pass #1: note waterfall
  VkRenderPass note_render_pass;
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout note_pipeline_layout;
  VkPipeline note_pipeline;
  VkBuffer note_vertex_buffer;
  VkDeviceMemory note_vertex_buffer_mem;
  VkBuffer note_instance_buffer;
  VkDeviceMemory note_instance_buffer_mem;
  VkBuffer note_index_buffer;
  VkDeviceMemory note_index_buffer_mem;
  std::vector<VkFramebuffer> swap_chain_framebuffers;

  // render pass #2: imgui
  VkRenderPass imgui_render_pass;
  VkDescriptorPool imgui_descriptor_pool;
  VkCommandPool imgui_cmd_pool;
  std::vector<VkCommandBuffer> imgui_cmd_buffers;
  std::vector<VkFramebuffer> imgui_swap_chain_framebuffers;

  VkCommandPool cmd_pool;
  VkDescriptorPool descriptor_pool;
  std::vector<VkDescriptorSet> descriptor_sets;

  VkImage tex_img;
  VkDeviceMemory tex_img_mem;
  VkImageView tex_img_view;
  VkSampler tex_sampler;

  VkImage depth_img;
  VkDeviceMemory depth_img_mem;
  VkImageView depth_img_view;

  std::vector<VkBuffer> uniform_buffers;
  std::vector<VkDeviceMemory> uniform_buffers_mem;
  std::vector<VkCommandBuffer> cmd_buffers;

  std::vector<VkSemaphore> img_available_semaphore;
  std::vector<VkSemaphore> render_fin_semaphore;
  std::vector<VkFence> in_flight_fences;
  std::vector<VkFence> imgs_in_flight;
  size_t current_frame = 0;

  std::vector<VkSemaphore> next_step_semaphores;

  std::array<CustomList<Note>, 256> notes_shown;
  size_t last_notes_shown_count;

  std::array<std::array<bool, NOTE_DEPTH_BUFFER_SIZE>, 256> note_depth_buffer;
  std::array<size_t, 256> notes_hidden;

  float key_left[257];
  float key_widths[257];
  float keyboard_height = 0;
  float keyboard_time = 0;
  char key_color[257] = {};

  bool show_settings = false;
  bool vsync = false;
  bool hide_notes = false;

  bool framebuffer_resized = false;

  float pre_time;
  double max_elapsed_time = 0;
  bool first_frame = true;

  uint32_t current_frame_index;

  int window_width = default_width;
  int window_height = default_height;

  Renderer();

  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void setupDevice();
  void createLogicalDevice();
  void createSwapChain();
  void createImageViews();
  void createDescriptorSetLayout();
  void createGraphicsPipeline(const char* vert_spv, size_t vert_spv_length, const char* frag_spv, size_t frag_spv_length, VkRenderPass render_pass, VkPipelineLayout* layout, VkPipeline* pipeline);
  void createPipelineCache();
  void createCommandPool(VkCommandPool* pool, VkCommandPoolCreateFlags flags);
  void createDepthResources();
  void createTextureImage();
  void createTextureImageView();
  void createTextureSampler();
  void createRenderPass(VkRenderPass* pass, bool has_depth, VkAttachmentLoadOp load_op, VkImageLayout initial_layout, VkImageLayout final_layout);
  void createVertexBuffer(Vertex vertices[], size_t count, VkBuffer& buffer, VkDeviceMemory& buffer_mem);
  void createInstanceBuffer(VkDeviceSize size, VkBuffer& buffer, VkDeviceMemory& buffer_mem);
  void createIndexBuffer(uint32_t indices[], size_t count, VkBuffer& buffer, VkDeviceMemory& buffer_mem);
  void createUniformBuffers(std::vector<VkBuffer>& buffer, std::vector<VkDeviceMemory>& buffer_mem);

  // render pass #1: note waterfall
  void createDescriptorPool();
  void createCommandBuffers();
  void createFramebuffers();

  // render pass #2: keyboard
  void createImGuiDescriptorPool();
  void createImGuiCommandBuffers();
  void createImGuiFramebuffers();

  void createDescriptorSets();
  void createSyncObjects();
  static void CheckVkResult(VkResult err);
  void initImGui();
  void destroyImGui();
  void drawFrame(float time);
  void PrepareKeyboard();
  void ImGuiFrame();

  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_mem);
  void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
  VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
  void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
private:
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
  void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& img_mem);
  void updateUniformBuffer(uint32_t current_img, float time);
  void endSingleTimeCommands(VkCommandBuffer cmd_buffer);
  void transitionImageLayout(VkImage img, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
  void copyBufferToImage(VkBuffer buffer, VkImage img, uint32_t width, uint32_t height);

  bool isDeviceSuitable(VkPhysicalDevice device);
  bool checkDeviceExtSupport(VkPhysicalDevice device);
  bool checkValidationLayerSupport();
  std::vector<const char*> getRequiredExtensions();
  bool hasStencilComponent(VkFormat format);

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
  VkFormat findDepthFormat();

  VkImageView createImageView(VkImage img, VkFormat format, VkImageAspectFlags aspect_flags);

  VkCommandBuffer beginSingleTimeCommands();

  VkShaderModule createShaderModule(const char* code, size_t length);

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  QueueFamilyIndices queue_families;

  static std::vector<char> readFile(const std::string& filename);

  uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};