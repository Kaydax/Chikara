#include "Main.h"

Renderer r;

void Main::run()
{
  initWindow(); //Setup everything for the window
  initVulkan(); //Setup everything for Vulkan
  mainLoop(); //The main loop for the application
  //Midi m("E:/Midi/tau2.5.9.mid");
  cleanup(); //Cleanup everything because we closed the application
}

void Main::initWindow()
{
  glfwInit(); //Init glfw
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Set the glfw api to GLFW_NO_API because we are using Vulkan
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); //Change the ability to resize the window
  r.window = glfwCreateWindow(width, height, "Chikara", nullptr, nullptr); //Now we create the window
  glfwSetWindowUserPointer(r.window, this);
  glfwSetFramebufferSizeCallback(r.window, r.framebufferResizeCallback);
}

void Main::initVulkan()
{
  r.createInstance(); //Create the Vulkan Instance
  r.createSurface();
  r.setupDevice(); //Pick the physical device
  r.createLogicalDevice(); //Create the logical device to use
  r.createSwapChain();
  r.createImageViews();
  r.createRenderPass();
  r.createDescriptorSetLayout();
  r.createGraphicsPipeline();
  r.createCommandPool();
  r.createDepthResources();
  r.createFramebuffers();
  r.createTextureImage();
  r.createTextureImageView();
  r.createTextureSampler();
  r.createVertexBuffer();
  r.createIndexBuffer();
  r.createUniformBuffers();
  r.createDescriptorPool();
  r.createDescriptorSets();
  r.createCommandBuffers();
  r.createSyncObjects();
}

void Main::mainLoop()
{
  while(!glfwWindowShouldClose(r.window))
  {
    glfwPollEvents();
    r.drawFrame();
  }

  vkDeviceWaitIdle(r.device);
}

#pragma region Recreating the swap chain

void Main::cleanupSwapChain()
{
  vkDestroyImageView(r.device, r.depth_img_view, nullptr);
  vkDestroyImage(r.device, r.depth_img, nullptr);
  vkFreeMemory(r.device, r.depth_img_mem, nullptr);

  for(size_t i = 0; i < r.swap_chain_framebuffers.size(); i++)
  {
    vkDestroyFramebuffer(r.device, r.swap_chain_framebuffers[i], nullptr);
  }

  vkFreeCommandBuffers(r.device, r.cmd_pool, static_cast<uint32_t>(r.cmd_buffers.size()), r.cmd_buffers.data());

  vkDestroyPipeline(r.device, r.graphics_pipeline, nullptr);
  vkDestroyPipelineLayout(r.device, r.pl_layout, nullptr);
  vkDestroyRenderPass(r.device, r.render_pass, nullptr);

  for(size_t i = 0; i < r.swap_chain_img_views.size(); i++)
  {
    vkDestroyImageView(r.device, r.swap_chain_img_views[i], nullptr);
  }

  vkDestroySwapchainKHR(r.device, r.swap_chain, nullptr);

  for(size_t i = 0; i < r.swap_chain_imgs.size(); i++)
  {
    vkDestroyBuffer(r.device, r.uniform_buffers[i], nullptr);
    vkFreeMemory(r.device, r.uniform_buffers_mem[i], nullptr);
  }

  vkDestroyDescriptorPool(r.device, r.descriptor_pool, nullptr);
}

void Main::recreateSwapChain()
{
  int width = 0, height = 0;
  glfwGetFramebufferSize(r.window, &width, &height);
  while(width == 0 || height == 0)
  {
    glfwGetFramebufferSize(r.window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(r.device);

  cleanupSwapChain();

  r.createSwapChain();
  r.createImageViews();
  r.createRenderPass();
  r.createGraphicsPipeline();
  r.createDepthResources();
  r.createFramebuffers();
  r.createUniformBuffers();
  r.createDescriptorPool();
  r.createDescriptorSets();
  r.createCommandBuffers();
}

#pragma endregion

#pragma region Cleanup

void Main::cleanup()
{
  cleanupSwapChain();

  vkDestroySampler(r.device, r.tex_sampler, nullptr);
  vkDestroyImageView(r.device, r.tex_img_view, nullptr);
  vkDestroyImage(r.device, r.tex_img, nullptr);
  vkFreeMemory(r.device, r.tex_img_mem, nullptr);

  vkDestroyDescriptorSetLayout(r.device, r.descriptor_set_layout, nullptr);

  vkDestroyBuffer(r.device, r.index_buffer, nullptr);
  vkFreeMemory(r.device, r.index_buffer_mem, nullptr);

  vkDestroyBuffer(r.device, r.vertex_buffer, nullptr);
  vkFreeMemory(r.device, r.vertex_buffer_mem, nullptr);

  for(size_t i = 0; i < max_frames_in_flight; i++)
  {
    vkDestroySemaphore(r.device, r.render_fin_semaphore[i], nullptr);
    vkDestroySemaphore(r.device, r.img_available_semaphore[i], nullptr);
    vkDestroyFence(r.device, r.in_flight_fences[i], nullptr);
  }

  vkDestroyCommandPool(r.device, r.cmd_pool, nullptr);
  vkDestroyDevice(r.device, nullptr);
  vkDestroySurfaceKHR(r.inst, r.surface, nullptr);
  vkDestroyInstance(r.inst, nullptr);

  glfwDestroyWindow(r.window);

  glfwTerminate(); //Now we terminate
}

#pragma endregion

int main()
{
  Main app; //Get the main class and call it app

  try
  {
    app.run(); //Startup the application
  } catch(const std::exception & e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE; //Something broke...
  }

  return EXIT_SUCCESS;
}