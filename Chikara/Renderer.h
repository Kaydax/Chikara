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

#pragma endregion

const int width = 800;
const int height = 600;

const int max_frames_in_flight = 2;

const std::vector<const char*> validation_layers = {
  "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_exts = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
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

struct Vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 tex_coord;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription binding_description = {};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, 3> attrib_descriptions = {};
    attrib_descriptions[0].binding = 0;
    attrib_descriptions[0].location = 0;
    attrib_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrib_descriptions[0].offset = offsetof(Vertex, pos);

    attrib_descriptions[1].binding = 0;
    attrib_descriptions[1].location = 1;
    attrib_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrib_descriptions[1].offset = offsetof(Vertex, color);

    attrib_descriptions[2].binding = 0;
    attrib_descriptions[2].location = 2;
    attrib_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrib_descriptions[2].offset = offsetof(Vertex, tex_coord);

    return attrib_descriptions;
  }
};

struct UniformBufferObject
{
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

#pragma endregion

const std::vector<Vertex> vertices = {
  {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, //0
  {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, //1
  {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, //2
  {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, //3

  {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, //4
  {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, //5
  {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, //6
  {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}} //7
};

const std::vector<uint16_t> indices = {
  //Top
  0, 1, 2,
  2, 3, 0,
    
  //Bottom
  6, 5, 4, 
  4, 7, 6,

  //Side 1
  0, 4, 5,
  5, 1, 0,

  5, 6, 2,
  2, 1, 5,

  6, 7, 3,
  3, 2, 6
};

class Renderer
{
  public:
    GLFWwindow* window;
    VkInstance inst;
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

    bool framebuffer_resized = false;

    void createInstance();
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
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();
    void drawFrame();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
  private:
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& img_mem);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_mem);
    void copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
    void updateUniformBuffer(uint32_t current_img);
    void endSingleTimeCommands(VkCommandBuffer cmd_buffer);
    void transitionImageLayout(VkImage img, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
    void copyBufferToImage(VkBuffer buffer, VkImage img, uint32_t width, uint32_t height);

    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtSupport(VkPhysicalDevice device);
    bool checkValidationLayerSupport();
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
};