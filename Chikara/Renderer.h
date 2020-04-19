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
#include "Midi.h"

#pragma endregion

#define MAX_NOTES_MULT 1000 // 10,000,000
#define MAX_NOTES_BASE 10000
#define MAX_NOTES MAX_NOTES_BASE * MAX_NOTES_MULT
#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1

const uint32_t width = 800;
const uint32_t height = 600;

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
  glm::vec3 color; // should be an index into a color table

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
    attrib_descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
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
  std::list<Note*>** note_buffer;
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
  std::vector<VkFramebuffer> swap_chain_framebuffers;

  VkRenderPass render_pass;
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pl_layout;
  VkPipeline graphics_pipeline;

  VkCommandPool cmd_pool;
  VkDescriptorPool descriptor_pool;
  std::vector<VkDescriptorSet> descriptor_sets;

  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_mem;
  VkBuffer instance_buffer;
  VkDeviceMemory instance_buffer_mem;
  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_mem;

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

  std::list<Note*> notes_shown;
  size_t last_notes_shown_count;

  bool framebuffer_resized = false;

  float pre_time;

  uint32_t current_frame_index;

  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void setupDevice();
  void createLogicalDevice();
  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createDescriptorSetLayout();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createDepthResources();
  void createTextureImage();
  void createTextureImageView();
  void createTextureSampler();
  void createVertexBuffer();
  void createInstanceBuffer();
  void createIndexBuffer();
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void createCommandBuffers();
  void createSyncObjects();
  void drawFrame(float time);

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

  VkShaderModule createShaderModule(const std::vector<char>& code);

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  static std::vector<char> readFile(const std::string& filename);

  uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};