#include "Main.h"

Renderer r;
Midi* midi;

void Main::run(int argc, char** argv)
{
  if (argc < 2) {
    printf("Usage: Chikara.exe [midi]\n");
    return;
  }
  midi = new Midi(argv[1]);
  r.note_buffer = midi->note_buffer;
  initWindow(); //Setup everything for the window
  initVulkan(); //Setup everything for Vulkan
  mainLoop(); //The main loop for the application
  cleanup(); //Cleanup everything because we closed the application
}

void Main::initWindow()
{
  glfwInit(); //Init glfw
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Set the glfw api to GLFW_NO_API because we are using Vulkan
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); //Change the ability to resize the window
  r.window = glfwCreateWindow(width, height, "Chikara", nullptr, nullptr); //Now we create the window
  glfwSetWindowUserPointer(r.window, &r);
  glfwSetFramebufferSizeCallback(r.window, r.framebufferResizeCallback);
}

void Main::initVulkan()
{
  r.createInstance(); //Create the Vulkan Instance
  r.setupDebugMessenger();
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
  r.createInstanceBuffer();
  r.createIndexBuffer();
  r.createUniformBuffers();
  r.createDescriptorPool();
  r.createDescriptorSets();
  r.createCommandBuffers();
  r.createSyncObjects();
}

auto timer = chrono::steady_clock();
auto last_time = timer.now();
uint64_t frame_counter = 0;
uint64_t fps = 0;

void Main::mainLoop()
{
  static auto start_time = std::chrono::high_resolution_clock::now();
  r.pre_time = 1.0;
  while(!glfwWindowShouldClose(r.window))
  {
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    glfwPollEvents();
    r.drawFrame(time);

    //Output fps
    ++frame_counter;
    if(last_time + chrono::seconds(1) < timer.now())
    {
      last_time = timer.now();
      fps = frame_counter;
      frame_counter = 0;
      cout << endl << fps << " fps";
    }
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
  if(enable_validation_layers)
  {
    r.DestroyDebugUtilsMessengerEXT(r.inst, r.debug_msg, nullptr);
  }

  cleanupSwapChain();

  vkDestroySampler(r.device, r.tex_sampler, nullptr);
  vkDestroyImageView(r.device, r.tex_img_view, nullptr);
  vkDestroyImage(r.device, r.tex_img, nullptr);
  vkFreeMemory(r.device, r.tex_img_mem, nullptr);

  vkDestroyDescriptorSetLayout(r.device, r.descriptor_set_layout, nullptr);

  vkDestroyBuffer(r.device, r.index_buffer, nullptr);
  vkFreeMemory(r.device, r.index_buffer_mem, nullptr);

  vkDestroyBuffer(r.device, r.instance_buffer, nullptr);
  vkFreeMemory(r.device, r.instance_buffer_mem, nullptr);

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

int main(int argc, char** argv)
{
  Main app; //Get the main class and call it app

  try
  {
    app.run(argc, argv); //Startup the application
  } catch(const std::exception & e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE; //Something broke...
  }

  return EXIT_SUCCESS;
}