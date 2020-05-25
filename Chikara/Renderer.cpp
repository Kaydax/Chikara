#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "Renderer.h"
#include "Main.h"
#include "Midi.h"
#include "KDMAPI.h"

Main m;

glm::vec3 colors[16] = {
  {51 / 255.0f, 102 / 255.0f, 255 / 255.0f},
  {255 / 255.0f, 126 / 255.0f, 51 / 255.0f},
  {51 / 255.0f, 255 / 255.0f, 102 / 255.0f},
  {255 / 255.0f, 51 / 255.0f, 129 / 255.0f},
  {51 / 255.0f, 255 / 255.0f, 255 / 255.0f},
  {228 / 255.0f, 51 / 255.0f, 255 / 255.0f},
  {153 / 255.0f, 255 / 255.0f, 51 / 255.0f},
  {75 / 255.0f, 51 / 255.0f, 255 / 255.0f},
  {255 / 255.0f, 204 / 255.0f, 51 / 255.0f},
  {51 / 255.0f, 180 / 255.0f, 255 / 255.0f},
  {255 / 255.0f, 51 / 255.0f, 51 / 255.0f},
  {51 / 255.0f, 255 / 255.0f, 177 / 255.0f},
  {255 / 255.0f, 51 / 255.0f, 204 / 255.0f},
  {78 / 255.0f, 255 / 255.0f, 51 / 255.0f},
  {153 / 255.0f, 51 / 255.0f, 255 / 255.0f},
  {231 / 255.0f, 255 / 255.0f, 51 / 255.0f}
};

#pragma region Create the instance

//Create the Vulkan instance
void Renderer::createInstance()
{
  if(enable_validation_layers && !checkValidationLayerSupport())
  {
    throw std::runtime_error("VKERR: Validation Layer requested, but it seems it's not available!");
  }

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Chikara";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  auto extensions = getRequiredExtensions();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();
  
  VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
  if(enable_validation_layers)
  {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();

    populateDebugMessengerCreateInfo(debug_create_info);
    create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
  }
  else
  {
    create_info.enabledLayerCount = 0;

    create_info.pNext = nullptr;
  }

  VkResult res = vkCreateInstance(&create_info, nullptr, &inst);
  if(res != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create instance!");
  }
}

void Renderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
  create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debugCallback;
}

void Renderer::setupDebugMessenger()
{
  if(!enable_validation_layers) return;

  VkDebugUtilsMessengerCreateInfoEXT create_info = {};
  populateDebugMessengerCreateInfo(create_info);

  if(CreateDebugUtilsMessengerEXT(inst, &create_info, nullptr, &debug_msg) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to setup debug messenger!");
  }
}

#pragma endregion

#pragma region Create the window surface and setup the rendering device

//Create the window surface
void Renderer::createSurface()
{
  if(glfwCreateWindowSurface(inst, window, nullptr, &surface) != VK_SUCCESS)
  {
    throw std::runtime_error("GLFW: Failed to create window surface!");
  }
}

//Setup the rendering device
void Renderer::setupDevice()
{
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(inst, &device_count, nullptr);

  if(device_count == 0)
  {
    throw std::runtime_error("VKERR: Failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(inst, &device_count, devices.data());

  for(const auto& device : devices)
  {
    if(isDeviceSuitable(device))
    {
      pdevice = device;
      break;
    }
  }

  if(pdevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("VKERR: Failed to find a suitable GPU!");
  }
}

//Create the logical rendering device
void Renderer::createLogicalDevice()
{
  QueueFamilyIndices indis = findQueueFamilies(pdevice);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::set<uint32_t> unique_queue_fams = { indis.graphics_fam.value(), indis.present_fam.value() };

  float queue_priority = 1.0f;
  for(uint32_t queue_fam : unique_queue_fams)
  {
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_fam;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceFeatures device_feats = {};

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.pEnabledFeatures = &device_feats;
  create_info.enabledExtensionCount = static_cast<uint32_t>(device_exts.size());
  create_info.ppEnabledExtensionNames = device_exts.data();

  if(enable_validation_layers)
  {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
  }
  else
  {
    create_info.enabledLayerCount = 0;
  }

  if(vkCreateDevice(pdevice, &create_info, nullptr, &device) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(device, indis.graphics_fam.value(), 0, &graphics_queue);
  vkGetDeviceQueue(device, indis.present_fam.value(), 0, &present_queue);
}

#pragma endregion

#pragma region Swap Chain

//Create the swap chain
void Renderer::createSwapChain()
{
  SwapChainSupportDetails swap_chain_support = querySwapChainSupport(pdevice);

  VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
  VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
  VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);

  image_count = swap_chain_support.capabilities.minImageCount + 1;
  if(swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
  {
    image_count = swap_chain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface; //The drawing surface we are using
  create_info.minImageCount = image_count; //The minimum amount of images in the swap chain
  create_info.imageFormat = surface_format.format; //The image color format
  create_info.imageColorSpace = surface_format.colorSpace; //The color space the image is using
  create_info.imageExtent = extent; //The bounds of the image
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indis = findQueueFamilies(pdevice);
  uint32_t queue_fam_indis[] = { indis.graphics_fam.value(), indis.present_fam.value() };

  if(indis.graphics_fam != indis.present_fam)
  {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //Images can be used across multiple queue families without explicit ownership transfers.
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_fam_indis;
  }
  else
  {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //An image is owned by one queue family at a time and ownership must be explicitly transfered before using it in another queue family. This option offers the best performance.
    create_info.queueFamilyIndexCount = 0; // Optional
    create_info.pQueueFamilyIndices = nullptr; // Optional
  }

  create_info.preTransform = swap_chain_support.capabilities.currentTransform; //The transform for the images in the swap chain
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //The alpha channel for the images in the swap chain (Why... idk)
  create_info.presentMode = present_mode; //The mode the surface is using
  create_info.clipped = VK_TRUE; //If true, clip anything that is being hidden by another window or something of the sort.
  create_info.oldSwapchain = VK_NULL_HANDLE; //This is the reference to the old swap chain if ever it gets recreated, currently its null because we are only using one swap chain

  if(vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create swap chain!");
  }
  else
  {
    std::cout << std::endl << "VKSUCCESS: Swap Chain created!" << std::endl;
  }

  //Now lets retrieve the images from the swap chain
  vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
  swap_chain_imgs.resize(image_count);
  vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_imgs.data());

  swap_chain_img_format = surface_format.format;
  swap_chain_extent = extent;
}

//Choose the swap surface color format
VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
  for(const auto& available_format : available_formats)
  {
    if(available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return available_format;
    }
  }

  return available_formats[0];
}

//Choose what kind of rendering mode the surface should be
VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
  for(const auto& available_present_mode : available_present_modes)
  {
    if(available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
    {
      return available_present_mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

//Set the bounds of the swap chain rendering
VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
  if(capabilities.currentExtent.width != UINT32_MAX)
  {
    return capabilities.currentExtent;
  }
  else
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actual_extent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actual_extent.width));
    actual_extent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actual_extent.height));

    return actual_extent;
  }
}

#pragma endregion

#pragma region Image Views

void Renderer::createImageViews()
{
  swap_chain_img_views.resize(swap_chain_imgs.size());

  for(uint32_t i = 0; i < swap_chain_imgs.size(); i++)
  {
    swap_chain_img_views[i] = createImageView(swap_chain_imgs[i], swap_chain_img_format, VK_IMAGE_ASPECT_COLOR_BIT);
  }
}

#pragma endregion

#pragma region Render Pass

void Renderer::createRenderPass(VkRenderPass* pass, bool has_depth, VkAttachmentLoadOp load_op, VkImageLayout initial_layout, VkImageLayout final_layout)
{
  //Setup the color attachment
  VkAttachmentDescription color_attachment = {};
  color_attachment.format = swap_chain_img_format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = load_op;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = initial_layout;
  color_attachment.finalLayout = final_layout;

  VkAttachmentDescription depth_attachment = {};
  if (has_depth) {
    depth_attachment.format = findDepthFormat();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref = {};
  if (has_depth) {
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  //Subpasses
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  if (has_depth)
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  //Create the render pass object
  std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;
  if (has_depth) {
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
  } else {
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
  }

  if (vkCreateRenderPass(device, &render_pass_info, nullptr, pass) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create render pass!");
  }
}

#pragma endregion

#pragma region Descriptors

void Renderer::createDescriptorSetLayout()
{
  VkDescriptorSetLayoutBinding ubo_layout_binding = {};
  ubo_layout_binding.binding = 0;
  ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo_layout_binding.descriptorCount = 1;
  ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  ubo_layout_binding.pImmutableSamplers = nullptr; // Optional

  VkDescriptorSetLayoutBinding sampler_layout_binding = {};
  sampler_layout_binding.binding = 1;
  sampler_layout_binding.descriptorCount = 1;
  sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.pImmutableSamplers = nullptr;
  sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };

  VkDescriptorSetLayoutCreateInfo layout_info = {};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
  layout_info.pBindings = bindings.data();

  if(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create descriptor set layout!");
  }
}

void Renderer::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = static_cast<uint32_t>(swap_chain_imgs.size());
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[1].descriptorCount = static_cast<uint32_t>(swap_chain_imgs.size());

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();
  pool_info.maxSets = static_cast<uint32_t>(swap_chain_imgs.size());

  if(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create descriptor pool!");
  }
}

void Renderer::createImGuiDescriptorPool()
{
  VkDescriptorPoolSize pool_sizes[] =
  {
      { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  if (vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_descriptor_pool) != VK_SUCCESS)
    throw std::runtime_error("VKERR: Failed to create ImGui descriptor pool!");
}

void Renderer::createDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> layouts(swap_chain_imgs.size(), descriptor_set_layout);
  VkDescriptorSetAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = descriptor_pool;
  alloc_info.descriptorSetCount = static_cast<uint32_t>(swap_chain_imgs.size());
  alloc_info.pSetLayouts = layouts.data();

  descriptor_sets.resize(swap_chain_imgs.size());
  if(vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to allocate descriptor sets!");
  }

  for(size_t i = 0; i < swap_chain_imgs.size(); i++)
  {
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = uniform_buffers[i];
    buffer_info.offset = 0;
    buffer_info.range = sizeof(UniformBufferObject);

    /*
    VkDescriptorImageInfo img_info = {};
    img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    img_info.imageView = tex_img_view;
    img_info.sampler = tex_sampler;
    */

    // remember to change this to 2 if uncommenting the other code
    std::array<VkWriteDescriptorSet, 1> descriptor_writes = {};
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet = descriptor_sets[i];
    descriptor_writes[0].dstBinding = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].pBufferInfo = &buffer_info;

    /*
    descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].dstSet = descriptor_sets[i];
    descriptor_writes[1].dstBinding = 1;
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[1].descriptorCount = 1;
    descriptor_writes[1].pImageInfo = &img_info;
    */


    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
  }
}

#pragma endregion

#pragma region Create the graphics pipeline

void Renderer::createGraphicsPipeline(const char* vert_spv, size_t vert_spv_length, const char* frag_spv, size_t frag_spv_length, VkRenderPass render_pass, VkPipelineLayout* layout, VkPipeline* pipeline)
{
  //Create the shader modules
  VkShaderModule vert_shader_module = createShaderModule(vert_spv, vert_spv_length);
  VkShaderModule frag_shader_module = createShaderModule(frag_spv, frag_spv_length);

  //Now to actually use the shaders we have to assign it to a pipeline stage
  VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
  vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT; //Assign the vertex shader to the vertex pipeline stage
  vert_shader_stage_info.module = vert_shader_module; //The module that contains the shader code
  vert_shader_stage_info.pName = "main"; //The function to invoke when reading the shader code

  VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
  frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT; //Assign the fragment shader to the fragment pipeline stage
  frag_shader_stage_info.module = frag_shader_module; //The module that contains the shader code
  frag_shader_stage_info.pName = "main"; //The function to invoke when reading the shader code

  //Put the two structures for the modules into an array so that they can be used later in the pipeline creation step
  VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

  //Binding and attribute
  VkVertexInputBindingDescription binding_description[] = { Vertex::getBindingDescription(), InstanceData::getBindingDescription() };
  auto attrib_descriptions = Vertex::getAttributeDescriptions();
  {
    auto inst_attrib_descriptions = InstanceData::getAttributeDescriptions();
    attrib_descriptions.insert(attrib_descriptions.end(), inst_attrib_descriptions.begin(), inst_attrib_descriptions.end());
  }

  //Describe the format of the vertex data that will be passed to the vertex shader
  VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = sizeof(binding_description) / sizeof(VkVertexInputBindingDescription);
  vertex_input_info.pVertexBindingDescriptions = binding_description;
  vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrib_descriptions.size());;
  vertex_input_info.pVertexAttributeDescriptions = attrib_descriptions.data();

  //Describe the input assembly modes
  VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //Triangle from every 3 vertices without reuse
  input_assembly.primitiveRestartEnable = VK_FALSE; //If true, it allows us to break up lines and triangles in the _STRIP topology modes

  //Setup the viewport
  VkViewport viewport = {};
  viewport.x = 0.0f; //Should always be 0
  viewport.y = 0.0f; //Should always be 0
  viewport.width = (float)swap_chain_extent.width; //The width and height should be the swap chain extend width and height because of the fact it may differ from the window's width and height
  viewport.height = (float)swap_chain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  //A scissor is just a filter that makes the rasterizer discard any pixels being rendered outside of it
  VkRect2D scissor = {};
  scissor.offset = { 0, 0 };
  scissor.extent = swap_chain_extent;

  //Combine the viewport and the scissor into the viewport state
  VkPipelineViewportStateCreateInfo viewport_state = {};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = &viewport;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &scissor;

  //Create the rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE; //If true, then fragments beyond the near/far planes are clamped to them as opposed to discarding them. Useful for creating shadow maps
  rasterizer.rasterizerDiscardEnable = VK_FALSE; //If true, geometry never passes through the rasterizer stage, basically disabling any output to the framebuffer
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f; //Thickness of lines in terms of number of fragments
  rasterizer.cullMode = VK_CULL_MODE_NONE; //Face culling
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //Vertex order for the front faces
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f; // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

  //Multi-sampling (Anti-aliasing)
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // Optional
  multisampling.pSampleMask = nullptr; // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE; // Optional

  //Depth Stencil
  VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
  depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil.depthTestEnable = VK_TRUE;
  depth_stencil.depthWriteEnable = VK_TRUE;
  depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depth_stencil.depthBoundsTestEnable = VK_FALSE;
  depth_stencil.stencilTestEnable = VK_FALSE;

  //Color blending
  VkPipelineColorBlendAttachmentState color_blend_attachment = {};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blending = {};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f; // Optional
  color_blending.blendConstants[1] = 0.0f; // Optional
  color_blending.blendConstants[2] = 0.0f; // Optional
  color_blending.blendConstants[3] = 0.0f; // Optional

  //Create the pipeline layout
  VkPipelineLayoutCreateInfo pipeline_layout_info = {};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1; // Optional
  pipeline_layout_info.pSetLayouts = &descriptor_set_layout; // Optional
  pipeline_layout_info.pushConstantRangeCount = 0; // Optional
  pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

  if(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, layout) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create pipeline layout!");
  }
  else
  {
    std::cout << std::endl << "VKSUCCESS: Pipeline layout created!" << std::endl;
  }

  //Create the pipeline. All of the inputs should be obvious
  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pDepthStencilState = &depth_stencil;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = nullptr; // Optional
  pipeline_info.layout = *layout;
  pipeline_info.renderPass = render_pass;
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional | Allows us to create a new graphics pipeline deriving from an existing pipeline
  pipeline_info.basePipelineIndex = -1; // Optional | Allows us to create a new graphics pipeline deriving from an existing pipeline

  if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, pipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create graphics pipeline!");
  }
  else
  {
    std::cout << std::endl << "VKSUCCESS: Graphics pipeline created!" << std::endl;
  }

  //Now destroy the shader modules
  vkDestroyShaderModule(device, frag_shader_module, nullptr);
  vkDestroyShaderModule(device, vert_shader_module, nullptr);
}

VkShaderModule Renderer::createShaderModule(const char* code, size_t length)
{
  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = length;
  create_info.pCode = reinterpret_cast<const uint32_t*>(code);

  VkShaderModule shader_module;
  if(vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create shader module!");
  }

  return shader_module;
}

#pragma endregion

#pragma region Pipeline Cache

void Renderer::createPipelineCache()
{
  VkPipelineCacheCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  // everything else is 0, this doesn't actually cache anything

  if (vkCreatePipelineCache(device, &info, nullptr, &pipeline_cache) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create pipeline cache!");
  }
}

#pragma endregion

#pragma region Depth Buffer

void Renderer::createDepthResources()
{
  VkFormat depth_format = findDepthFormat();

  createImage(swap_chain_extent.width, swap_chain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_img, depth_img_mem);
  depth_img_view = createImageView(depth_img, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

#pragma endregion

#pragma region Create Framebuffer

void Renderer::createFramebuffers()
{
  swap_chain_framebuffers.resize(swap_chain_img_views.size());

  for(size_t i = 0; i < swap_chain_img_views.size(); i++)
  {
    std::array<VkImageView, 2> attachments = {
        swap_chain_img_views[i],
        depth_img_view
    };

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = note_render_pass;
    framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebuffer_info.pAttachments = attachments.data();
    framebuffer_info.width = swap_chain_extent.width;
    framebuffer_info.height = swap_chain_extent.height;
    framebuffer_info.layers = 1;

    if(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swap_chain_framebuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("VKERR: Failed to create framebuffer!");
    }
  }
}

void Renderer::createImGuiFramebuffers()
{
  imgui_swap_chain_framebuffers.resize(swap_chain_img_views.size());

  for (size_t i = 0; i < swap_chain_img_views.size(); i++)
  {
    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = imgui_render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &swap_chain_img_views[i];
    framebuffer_info.width = swap_chain_extent.width;
    framebuffer_info.height = swap_chain_extent.height;
    framebuffer_info.layers = 1;

    if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &imgui_swap_chain_framebuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("VKERR: Failed to create ImGui framebuffer!");
  }
}

#pragma endregion

#pragma region Create Command Pool

void Renderer::createCommandPool(VkCommandPool* pool, VkCommandPoolCreateFlags flags)
{
  QueueFamilyIndices queue_fam_indis = findQueueFamilies(pdevice);

  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = queue_fam_indis.graphics_fam.value();
  pool_info.flags = flags;

  if(vkCreateCommandPool(device, &pool_info, nullptr, pool) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create command pool!");
  }
}

#pragma endregion

#pragma region Textures

void Renderer::createTextureImage()
{
  //Get the image and load it
  int tex_width, tex_height, tex_chans;
  stbi_uc* pixels = stbi_load("Textures/texture.jpg", &tex_width, &tex_height, &tex_chans, STBI_rgb_alpha);
  VkDeviceSize img_size = tex_width * tex_height * 4;

  if(!pixels)
  {
    throw std::runtime_error("STB: Failed to load texture image!");
  }

  //Stage the image by creating a buffer
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_mem;

  createBuffer(img_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_mem);

  //Map the memory
  void* data;
  vkMapMemory(device, staging_buffer_mem, 0, img_size, 0, &data);
  memcpy(data, pixels, static_cast<size_t>(img_size));
  vkUnmapMemory(device, staging_buffer_mem);

  //Cleanup
  stbi_image_free(pixels);

  //Create the image
  createImage(tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex_img, tex_img_mem);

  transitionImageLayout(tex_img, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(staging_buffer, tex_img, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
  transitionImageLayout(tex_img, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  //Destroy
  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_mem, nullptr);
}

void Renderer::createTextureImageView()
{
  tex_img_view = createImageView(tex_img, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::createTextureSampler()
{
  VkSamplerCreateInfo sampler_info = {};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = VK_FILTER_LINEAR;
  sampler_info.minFilter = VK_FILTER_LINEAR;
  sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler_info.anisotropyEnable = VK_FALSE;
  sampler_info.maxAnisotropy = 16;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.mipLodBias = 0.0f;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = 0.0f;

  if(vkCreateSampler(device, &sampler_info, nullptr, &tex_sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: failed to create texture sampler!");
  }
}

void Renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& img_mem)
{
  VkImageCreateInfo img_info = {};
  img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  img_info.imageType = VK_IMAGE_TYPE_2D;
  img_info.extent.width = width;
  img_info.extent.height = height;
  img_info.extent.depth = 1;
  img_info.mipLevels = 1;
  img_info.arrayLayers = 1;
  img_info.format = format;
  img_info.tiling = tiling;
  img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  img_info.usage = usage;
  img_info.samples = VK_SAMPLE_COUNT_1_BIT;
  img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if(vkCreateImage(device, &img_info, nullptr, &img) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create image!");
  }

  VkMemoryRequirements mem_reqs;
  vkGetImageMemoryRequirements(device, img, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = findMemoryType(mem_reqs.memoryTypeBits, properties);

  if(vkAllocateMemory(device, &alloc_info, nullptr, &img_mem) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to allocate image memory!");
  }

  vkBindImageMemory(device, img, img_mem, 0);
}

void Renderer::transitionImageLayout(VkImage img, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
  VkCommandBuffer cmd_buffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = img;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0; // TODO
  barrier.dstAccessMask = 0; // TODO

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags dest_stage;

  if(old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if(old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(
    cmd_buffer,
    source_stage, dest_stage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  endSingleTimeCommands(cmd_buffer);
}

#pragma endregion

#pragma region Buffers

void Renderer::createVertexBuffer(Vertex vertices[], size_t count, VkBuffer& buffer, VkDeviceMemory& buffer_mem)
{
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_mem;

  createBuffer(count * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_mem);

  void* data;
  vkMapMemory(device, staging_buffer_mem, 0, count * sizeof(Vertex), 0, &data);
  memcpy(data, vertices, count * sizeof(Vertex));
  vkUnmapMemory(device, staging_buffer_mem);

  createBuffer(count * sizeof(Vertex), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, buffer_mem);
  copyBuffer(staging_buffer, buffer, count * sizeof(Vertex));

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_mem, nullptr);
}

void Renderer::createInstanceBuffer(VkDeviceSize size, VkBuffer& buffer, VkDeviceMemory& buffer_mem)
{
  //createBuffer(sizeof(InstanceData) * MAX_NOTES, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, note_instance_buffer, note_instance_buffer_mem);
  createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, buffer_mem);
}

void Renderer::createIndexBuffer(uint32_t indices[], size_t count, VkBuffer& buffer, VkDeviceMemory& buffer_mem)
{
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_mem;

  createBuffer(count * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_mem);

  void* data;
  vkMapMemory(device, staging_buffer_mem, 0, count * sizeof(uint32_t), 0, &data);
  memcpy(data, indices, count * sizeof(uint32_t));
  vkUnmapMemory(device, staging_buffer_mem);

  createBuffer(count * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, buffer_mem);
  copyBuffer(staging_buffer, buffer, count * sizeof(uint32_t));

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_mem, nullptr);
}

void Renderer::createUniformBuffers(std::vector<VkBuffer>& buffer, std::vector<VkDeviceMemory>& buffer_mem)
{
  VkDeviceSize buffer_size = sizeof(UniformBufferObject);

  buffer.resize(swap_chain_imgs.size());
  buffer_mem.resize(swap_chain_imgs.size());

  for(size_t i = 0; i < swap_chain_imgs.size(); i++)
  {
    createBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer[i], buffer_mem[i]);
  }
}

/*
void Renderer::createNoteVertexBuffer()
{
  // this data will be replaced with instancing
  Vertex vertices[] {
    { {0,1}, {0,1} },
    { {1,1}, {1,1} },
    { {1,0}, {1,0} },
    { {0,0}, {0,0} },
  };

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_mem;

  createBuffer(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_mem);

  void* data;
  vkMapMemory(device, staging_buffer_mem, 0, sizeof(vertices), 0, &data);
  memcpy(data, vertices, sizeof(vertices));
  vkUnmapMemory(device, staging_buffer_mem);

  createBuffer(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, note_vertex_buffer, note_vertex_buffer_mem);
  copyBuffer(staging_buffer, note_vertex_buffer, sizeof(vertices));

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_mem, nullptr);
}

void Renderer::createNoteInstanceBuffer()
{
  createBuffer(sizeof(InstanceData) * MAX_NOTES, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, note_instance_buffer, note_instance_buffer_mem);
}

void Renderer::createNoteIndexBuffer()
{
  uint32_t indices[] = {
    0, 1, 2,
    2, 3, 0,
  };

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_mem;

  createBuffer(sizeof(indices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_mem);

  void* data;
  vkMapMemory(device, staging_buffer_mem, 0, sizeof(indices), 0, &data);
  memcpy(data, indices, sizeof(indices));
  vkUnmapMemory(device, staging_buffer_mem);

  createBuffer(sizeof(indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, note_index_buffer, note_index_buffer_mem);
  copyBuffer(staging_buffer, note_index_buffer, sizeof(indices));

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_mem, nullptr);
}

void Renderer::createNoteUniformBuffers()
{
  VkDeviceSize buffer_size = sizeof(UniformBufferObject);

  uniform_buffers.resize(swap_chain_imgs.size());
  uniform_buffers_mem.resize(swap_chain_imgs.size());

  for(size_t i = 0; i < swap_chain_imgs.size(); i++)
  {
    createBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffers[i], uniform_buffers_mem[i]);
  }
}
*/

void Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_mem)
{
  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if(vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create buffer!");
  }

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(device, buffer, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = findMemoryType(mem_reqs.memoryTypeBits, properties);

  if(vkAllocateMemory(device, &alloc_info, nullptr, &buffer_mem) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device, buffer, buffer_mem, 0);
}

void Renderer::copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
  VkCommandBuffer cmd_buffer = beginSingleTimeCommands();

  VkBufferCopy copy_region = {};
  copy_region.size = size;
  vkCmdCopyBuffer(cmd_buffer, src_buffer, dst_buffer, 1, &copy_region);

  endSingleTimeCommands(cmd_buffer);
}

void Renderer::createCommandBuffers()
{
  //Allocate the command buffer
  cmd_buffers.resize(swap_chain_framebuffers.size() * MAX_NOTES_MULT);

  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = cmd_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = (uint32_t)cmd_buffers.size();

  if(vkAllocateCommandBuffers(device, &alloc_info, cmd_buffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to allocate command buffers!");
  }

  //Start recording the command buffer
  for(size_t swap_chain = 0; swap_chain < swap_chain_framebuffers.size(); swap_chain++)
  {
    for (size_t note_buf_idx = 0; note_buf_idx < MAX_NOTES_MULT; note_buf_idx++)
    {
      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

      if (vkBeginCommandBuffer(cmd_buffers[note_buf_idx + swap_chain * MAX_NOTES_MULT], &begin_info) != VK_SUCCESS)
      {
        throw std::runtime_error("VKERR: Failed to begin recording command buffer!");
      }

      //Start the render pass
      VkRenderPassBeginInfo render_pass_info = {};
      render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      render_pass_info.renderPass = note_render_pass;
      render_pass_info.framebuffer = swap_chain_framebuffers[swap_chain]; //Create a framebuffer for each swap chain image
      render_pass_info.renderArea.offset = { 0, 0 };
      render_pass_info.renderArea.extent = swap_chain_extent; //Render area defines where shader loads and stores take place. Anything outside this area are undefined, allowing for better performance

      std::array<VkClearValue, 2> clear_values = {};
      clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
      clear_values[1].depthStencil = { 1.0f, 0 };

      render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
      render_pass_info.pClearValues = clear_values.data();

      auto idx = note_buf_idx + swap_chain * MAX_NOTES_MULT;

      //Now the render pass can begin
      vkCmdBeginRenderPass(cmd_buffers[idx], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(cmd_buffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, note_pipeline);

      VkBuffer vertex_buffers[] = { note_vertex_buffer };
      VkDeviceSize offsets[] = { 0 };
      vkCmdBindVertexBuffers(cmd_buffers[idx], VERTEX_BUFFER_BIND_ID, 1, vertex_buffers, offsets);
      vkCmdBindVertexBuffers(cmd_buffers[idx], INSTANCE_BUFFER_BIND_ID, 1, &note_instance_buffer, offsets);
      vkCmdBindIndexBuffer(cmd_buffers[idx], note_index_buffer, 0, VK_INDEX_TYPE_UINT32);
      vkCmdBindDescriptorSets(cmd_buffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, note_pipeline_layout, 0, 1, &descriptor_sets[swap_chain], 0, nullptr);

      vkCmdDrawIndexed(cmd_buffers[idx], 6, MAX_NOTES_BASE * (note_buf_idx + 1), 0, 0, 0);

      vkCmdEndRenderPass(cmd_buffers[idx]);

      if (vkEndCommandBuffer(cmd_buffers[idx]) != VK_SUCCESS)
      {
        throw std::runtime_error("VKERR: Failed to record command buffer!");
      }
    }
  }
}

void Renderer::createImGuiCommandBuffers()
{
  //Allocate the command buffer
  imgui_cmd_buffers.resize(swap_chain_framebuffers.size());

  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = imgui_cmd_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = (uint32_t)imgui_cmd_buffers.size();

  if (vkAllocateCommandBuffers(device, &alloc_info, imgui_cmd_buffers.data()) != VK_SUCCESS)
    throw std::runtime_error("VKERR: Failed to allocate command buffers!");
}


#pragma endregion

#pragma region Create sync objects

void Renderer::createSyncObjects()
{
  img_available_semaphore.resize(max_frames_in_flight);
  render_fin_semaphore.resize(max_frames_in_flight);
  in_flight_fences.resize(max_frames_in_flight);
  imgs_in_flight.resize(swap_chain_imgs.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info = {};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for(size_t i = 0; i < max_frames_in_flight; i++)
  {
    if(
      vkCreateSemaphore(device, &semaphore_info, nullptr, &img_available_semaphore[i]) != VK_SUCCESS || 
      vkCreateSemaphore(device, &semaphore_info, nullptr, &render_fin_semaphore[i]) != VK_SUCCESS || 
      vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS
    )
    {

      throw std::runtime_error("VKERR: Failed to create synchronization objects for a frame!");
    }
  }
}

#pragma endregion

#pragma region Drawing

void Renderer::drawFrame(float time)
{
  auto start_time = std::chrono::high_resolution_clock::now();

  vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &in_flight_fences[current_frame]);

  uint32_t img_index;
  VkResult result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, img_available_semaphore[current_frame], VK_NULL_HANDLE, &img_index);

  if(result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    m.recreateSwapChain();
    return;
  }
  else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("VKERR: Failed to acquire swap chain image!");
  }

  //Check if a previous frame is using this image (i.e. there is its fence to wait on)
  if(imgs_in_flight[img_index] != VK_NULL_HANDLE)
  {
    vkWaitForFences(device, 1, &imgs_in_flight[img_index], VK_TRUE, UINT64_MAX);
  }

  //Mark the image as now being in use by this frame
  imgs_in_flight[img_index] = imgs_in_flight[current_frame];

  //Update the Uniform Buffer
  updateUniformBuffer(img_index, time);

  concurrency::parallel_for(size_t(0), size_t(256), [&](size_t i) {
    moodycamel::ReaderWriterQueue<NoteEvent>* note_events = note_event_buffer[i];
    while (true) {
      NoteEvent* peek = note_events->peek();
      if (peek == nullptr)
        break;
      NoteEvent event = *peek;
      std::stack<Note*>& note_stack = note_stacks[event.track][i];
      if (event.time < time + pre_time) {
        note_events->try_dequeue(event);
        switch (event.type) {
        case NoteEventType::NoteOn: {
          Note n;
          n.track = event.track;
          n.start = event.time;
          n.end = INFINITY;
          n.key = i;

          notes_shown[i].push_front(n);
          notes_per_key[i]++;
          note_stack.push(&notes_shown[i].front());
          break;
        }
        case NoteEventType::NoteOff: {
          if (!note_stack.empty()) {
            // dangling pointer here should be impossible
            Note* n = note_stack.top();
            note_stack.pop();
            n->end = event.time;
          }
          break;
        }
        case NoteEventType::TrackEnded: {
          for (auto& n : notes_shown[i]) {
            if ((n.track & ~0xF) >> 4 == event.track && n.end == INFINITY)
              n.end = event.time;
          }
          break;
        }
        }
      }
      else {
        break;
      }
    }
  });

  /*
  concurrency::parallel_for(size_t(0), size_t(256), [&](size_t i) {
    moodycamel::ReaderWriterQueue<Note*>* notes = note_buffer[i];
    //for(int j = 0; j < 1; j++)
    while (true)
    {
      Note** peek = notes->peek();
      if (peek == nullptr)
        break;
      Note* n = *peek;
      if (n->end < n->start)
      {
        throw std::runtime_error("The fucking midi broke");
      }
      if (n->start < time + pre_time)
      {
        notes->try_dequeue(n);
        notes_shown[n->key].push_front(n);
        notes_per_key[i]++;
      }
      else
      {
        break;
      }
    }
    });
   */

  size_t notes_shown_size = 0;
  for (auto& vec : notes_shown)
    notes_shown_size += vec.size();

  if (notes_shown_size > MAX_NOTES)
    throw std::runtime_error("UNIMPLEMENTED!!!!");

  void* data;
  int note_cmd_buf = 0;
  if (last_notes_shown_count > 0) {
    vkMapMemory(device, note_instance_buffer_mem, 0, sizeof(InstanceData) * last_notes_shown_count, 0, &data);
    if (last_notes_shown_count > notes_shown_size)
      memset((char*)data + notes_shown_size * sizeof(InstanceData), 0, sizeof(InstanceData) * (last_notes_shown_count - notes_shown_size));

    InstanceData* data_i = (InstanceData*)data;
    //cout << endl << "Notes playing: " << notes_shown.size();

    last_notes_shown_count = notes_shown_size;

    size_t key_indices[256] = {};
    size_t cur_offset = 0;

    // yep, this iterates over the keys twice...
    for (int i = 0; i < 256; i++) {
      if (g_sharp_table[i]) {
        key_indices[i] = cur_offset;
        cur_offset += notes_per_key[i];
      }
    }
    for (int i = 0; i < 256; i++) {
      if (!g_sharp_table[i]) {
        key_indices[i] = cur_offset;
        cur_offset += notes_per_key[i];
      }
    }

    concurrency::parallel_for(size_t(0), size_t(256), [&](size_t i) {
      auto& list = notes_shown[i];
      for (auto it = list.begin(); it != list.end();)
      {
        Note n = *it;
        if (time >= n.end)
        {
          //event_queue[i].push_back(MAKELONG(MAKEWORD((n->channel) | (8 << 4), n->key), MAKEWORD(n->velocity, 0)));
          data_i[key_indices[i]++] = { 0, 0, 0, {0,0,0} };
          notes_per_key[i]--;
          //delete n;
          it = list.erase(it);
        }
        else
        {
          data_i[key_indices[i]++] = { static_cast<float>(n.start), static_cast<float>(n.end), n.key, colors[n.track & 0xF] };
          it++;
        }
      }
      });

    for (int i = 0; i < MAX_NOTES_MULT; i++) {
      if (notes_shown_size <= (MAX_NOTES_BASE * (i + 1))) {
        note_cmd_buf = i;
        break;
      }
    }

    //memcpy(data, data_v, (size_t)buffer_size);
    vkUnmapMemory(device, note_instance_buffer_mem);
  } else {
    last_notes_shown_count = notes_shown_size;
  }

  // render imgui command buffers
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGuiFrame();
  ImGui::Render();

  {
    vkResetCommandPool(device, imgui_cmd_pool, 0);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(imgui_cmd_buffers[img_index], &begin_info);

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = imgui_render_pass;
    render_pass_info.framebuffer = imgui_swap_chain_framebuffers[img_index];
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = swap_chain_extent;
    vkCmdBeginRenderPass(imgui_cmd_buffers[img_index], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imgui_cmd_buffers[img_index]);
    vkCmdEndRenderPass(imgui_cmd_buffers[img_index]);

    if (vkEndCommandBuffer(imgui_cmd_buffers[img_index]) != VK_SUCCESS)
      throw std::runtime_error("VKERR: Failed to record ImGui command buffer!");
  }

  //Submit to the command buffer
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  std::array<VkCommandBuffer, 2> command_buffers = { cmd_buffers[note_cmd_buf + img_index * MAX_NOTES_MULT], imgui_cmd_buffers[img_index] };

  VkSemaphore wait_semaphores[] = { img_available_semaphore[current_frame] }; //The semaphores we are waiting for
  VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; //Wait with writing colors until the image is available
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
  submit_info.pCommandBuffers = command_buffers.data();

  VkSemaphore signal_semaphores[] = { render_fin_semaphore[current_frame] }; //What semaphores we signal once the cmd buffer finished execution
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  vkResetFences(device, 1, &in_flight_fences[current_frame]);

  if(vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to submit draw to command buffer!");
  }

  //Presentation
  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;

  VkSwapchainKHR swap_chains[] = { swap_chain }; //An array of all our swap chains
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swap_chains;
  present_info.pImageIndices = &img_index;
  present_info.pResults = nullptr; // Optional

  result = vkQueuePresentKHR(present_queue, &present_info);

  if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized)
  {
    framebuffer_resized = false;
    m.recreateSwapChain();
  }
  else if(result != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to present swap chain image!");
  }

  current_frame = (current_frame + 1) % max_frames_in_flight;

  auto elapsed_time = std::chrono::duration<double, std::ratio<1>>(std::chrono::high_resolution_clock::now() - start_time).count();
  max_elapsed_time = max(max_elapsed_time, elapsed_time);
}

void Renderer::ImGuiFrame() {
  float framerate = ImGui::GetIO().Framerate;
  const ImGuiStat statistics[] = {
    {ImGuiStatType::Float, "FPS: ", &framerate},
    {ImGuiStatType::Double, "Longest frame: ", &max_elapsed_time},
  };

  size_t longest_len = 0;
  const char* longest_str = nullptr;
  for (auto& stat : statistics) {
    auto len = strlen(stat.name);
    if (len > longest_len) {
      longest_len = len;
      longest_str = stat.name;
    }
  }

  ImGui::ShowDemoWindow();
  ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
  auto first_column_len = ImGui::CalcTextSize(longest_str).x;
  auto second_column_len = 64.0f;
  ImGui::SetNextWindowSize(ImVec2(first_column_len + second_column_len + 16.0f, 0.0f));
  ImGui::Begin("stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav |
    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, first_column_len + 8.0f);
    ImGui::SetColumnWidth(1, second_column_len + 8.0f);
    for (auto& stat : statistics) {
      ImGui::Text(stat.name);
      ImGui::NextColumn();
      switch (stat.type) {
      case ImGuiStatType::Float:
        ImGui::Text("%.1f", *(float*)stat.value);
        break;
      case ImGuiStatType::Double:
        ImGui::Text("%.1lf", *(double*)stat.value);
        break;
      default:
        throw std::runtime_error("invalid statistic type");
      }
      ImGui::NextColumn();
    }
    ImGui::Columns(1);
  ImGui::End();
}

void Renderer::updateUniformBuffer(uint32_t current_img, float time)
{
  //Now lets rotate the model around the Z-axis based on the time. This will rotate 90 degrees per second
  UniformBufferObject ubo = {};
  //ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.model = glm::identity<glm::mat4>();

  //Now lets look at the model from a 45 degree angle
  //ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::identity<glm::mat4>();

  //A perspective projection with a 45 degree FOV
  //ubo.proj = glm::perspective(glm::radians(45.0f), swap_chain_extent.width / (float)swap_chain_extent.height, 0.1f, 10.0f);
  //ubo.proj = glm::identity<glm::mat4>();
  ubo.proj = glm::identity<glm::mat4>();
  ubo.proj = glm::translate(ubo.proj, glm::vec3(-1, -1, 0));
  ubo.proj = glm::scale(ubo.proj, glm::vec3(2, 2, -1));
  ubo.proj = glm::translate(ubo.proj, glm::vec3(0, 1, 0));

  //Now lets flip the Y coordinates because of the fact GLM was created for OpenGL
  ubo.proj[1][1] *= -1;

  // blah blah blah blah
  ubo.time = time;
  ubo.pre_time = pre_time;

  //Now lets copy the data in the uniform buffer into the current uniform buffer
  void* data;
  vkMapMemory(device, uniform_buffers_mem[current_img], 0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(device, uniform_buffers_mem[current_img]);
}

#pragma endregion

#pragma region Check device support

//Check if the device supports swap chains
SwapChainSupportDetails Renderer::querySwapChainSupport(VkPhysicalDevice device)
{
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

  if(format_count != 0)
  {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

  if(present_mode_count != 0)
  {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
  }

  return details;
}

//Make sure the device is suitable for use
bool Renderer::isDeviceSuitable(VkPhysicalDevice device)
{
  QueueFamilyIndices indis = findQueueFamilies(device);

  bool exts_supported = checkDeviceExtSupport(device);

  bool swap_chain_adequate = false;
  if(exts_supported)
  {
    SwapChainSupportDetails swap_chain_support = querySwapChainSupport(device);
    swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
  }

  VkPhysicalDeviceFeatures supported_feats;
  vkGetPhysicalDeviceFeatures(device, &supported_feats);

  return indis.isComplete() && exts_supported && swap_chain_adequate && supported_feats.samplerAnisotropy;
}

//Check what the device supports
bool Renderer::checkDeviceExtSupport(VkPhysicalDevice device)
{
  uint32_t ext_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, nullptr);

  std::vector<VkExtensionProperties> available_exts(ext_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, available_exts.data());

  std::set<std::string> required_exts(device_exts.begin(), device_exts.end());

  for(const auto& ext : available_exts)
  {
    required_exts.erase(ext.extensionName);
  }

  return required_exts.empty();
}

//Find out what queue families the device supports
QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice device)
{
  QueueFamilyIndices indis;

  uint32_t queue_fam_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_fam_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_fams(queue_fam_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_fam_count, queue_fams.data());

  int i = 0;
  for(const auto& queue_fam : queue_fams)
  {
    if(queue_fam.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indis.graphics_fam = i;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

    if(present_support)
    {
      indis.present_fam = i;
    }

    if(indis.isComplete())
    {
      break;
    }

    i++;
  }

  queue_families = indis;
  return indis;
}

//Check what validation layers the device supports
bool Renderer::checkValidationLayerSupport()
{
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  //avbl_layers = available layers
  std::vector<VkLayerProperties> avbl_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, avbl_layers.data());

  for(const char* layer_name : validation_layers)
  {
    bool layer_found = false;

    for(const auto& layer_props : avbl_layers)
    {
      if(strcmp(layer_name, layer_props.layerName) == 0)
      {
        layer_found = true;
        break;
      }
    }

    if(!layer_found)
    {
      return false;
    }
  }

  return true;
}

std::vector<const char*> Renderer::getRequiredExtensions()
{
  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

  if(enable_validation_layers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

#pragma endregion

#pragma region Other things

bool Renderer::hasStencilComponent(VkFormat format)
{
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat Renderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
  for(VkFormat format : candidates)
  {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(pdevice, format, &props);

    if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
    {
      return format;
    }
    else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
    {
      return format;
    }
  }

  throw std::runtime_error("VKERR: Failed to find supported format!");
}

VkFormat Renderer::findDepthFormat()
{
  return findSupportedFormat(
    { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}

VkImageView Renderer::createImageView(VkImage img, VkFormat format, VkImageAspectFlags aspect_flags)
{
  VkImageViewCreateInfo view_info = {};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = img;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;
  view_info.subresourceRange.aspectMask = aspect_flags;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  VkImageView img_view;
  if(vkCreateImageView(device, &view_info, nullptr, &img_view) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to create image view!");
  }

  return img_view;
}

VkCommandBuffer Renderer::beginSingleTimeCommands()
{
  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = cmd_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer cmd_buffer;
  vkAllocateCommandBuffers(device, &alloc_info, &cmd_buffer);

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(cmd_buffer, &begin_info);

  return cmd_buffer;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer cmd_buffer)
{
  vkEndCommandBuffer(cmd_buffer);

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd_buffer;

  vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphics_queue);

  vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buffer);
}

void Renderer::copyBufferToImage(VkBuffer buffer, VkImage img, uint32_t width, uint32_t height)
{
  VkCommandBuffer cmd_buffer = beginSingleTimeCommands();

  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = { 0, 0, 0 };
  region.imageExtent = {
      width,
      height,
      1
  };

  vkCmdCopyBufferToImage(
    cmd_buffer,
    buffer,
    img,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  endSingleTimeCommands(cmd_buffer);
}

std::vector<char> Renderer::readFile(const std::string& filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if(!file.is_open())
  {
    throw std::runtime_error("Chikara: Failed to open file!");
  }

  size_t file_size = (size_t)file.tellg();
  std::vector<char> buffer(file_size);

  file.seekg(0);
  file.read(buffer.data(), file_size);

  file.close();

  return buffer;
}

void Renderer::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
  renderer->framebuffer_resized = true;
}

uint32_t Renderer::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties mem_props;
  vkGetPhysicalDeviceMemoryProperties(pdevice, &mem_props);

  for(uint32_t i = 0; i < mem_props.memoryTypeCount; i++)
  {
    if((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }

  throw std::runtime_error("VKERR: Failed to find suitable memory type!");
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

VkResult Renderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if(func != nullptr)
  {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  }
  else
  {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void Renderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if(func != nullptr)
  {
    func(instance, debugMessenger, pAllocator);
  }
}

void Renderer::CheckVkResult(VkResult err) {
  if (err != VK_SUCCESS)
    std::runtime_error("VKERR: CheckVKResult failed!");
}

void Renderer::initImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(window, true);

  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = inst;
  init_info.PhysicalDevice = pdevice;
  init_info.Device = device;
  init_info.QueueFamily = queue_families.graphics_fam.value(); // would've failed much earlier if this had no value
  init_info.Queue = graphics_queue;
  init_info.PipelineCache = pipeline_cache;
  init_info.DescriptorPool = imgui_descriptor_pool;
  init_info.Allocator = nullptr;
  init_info.MinImageCount = image_count;
  init_info.ImageCount = image_count;
  init_info.CheckVkResultFn = CheckVkResult;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&init_info, imgui_render_pass);

  VkCommandBuffer cmd_buf = beginSingleTimeCommands();
  ImGui_ImplVulkan_CreateFontsTexture(cmd_buf);
  endSingleTimeCommands(cmd_buf);
}

void Renderer::destroyImGui() {
  ImGui_ImplGlfw_Shutdown();
  ImGui_ImplVulkan_Shutdown();
}

#pragma endregion