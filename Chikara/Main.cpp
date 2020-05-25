#include "Main.h"
#ifndef KDMAPI_H
#include "KDMAPI.h"
#endif

// msvc complains about narrowing conversion with bin2c
#pragma warning(push)
#pragma warning(disable : 4838)
#pragma warning(disable : 4309)
#include "Shaders/notes_f.h"
#include "Shaders/notes_v.h"
#pragma warning(pop)

Renderer r;
Midi* midi;

Vertex instanced_quad[] {
  { {0,1}, {0,1} },
  { {1,1}, {1,1} },
  { {1,0}, {1,0} },
  { {0,0}, {0,0} },
};

uint32_t instanced_quad_indis[] = {
    0, 1, 2,
    2, 3, 0,
};

void Main::run(int argc, wchar_t** argv)
{
  if (argc < 2) {
    printf("Usage: Chikara.exe [midi]\n");
    return;
  }

  KDMAPI::Init();
  midi = new Midi(argv[1]);
  r.note_event_buffer = midi->note_event_buffer;
  r.midi_renderer_time = &midi->renderer_time;
  r.note_stacks.resize(midi->track_count * 16);
  // playback thread spawned in mainLoop to ensure it's synced with render
  midi->SpawnLoaderThread();
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
  r.createRenderPass(&r.note_render_pass, true, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  r.createDescriptorSetLayout();
  r.createGraphicsPipeline(notes_v, notes_v_length, notes_f, notes_f_length, r.note_render_pass, &r.note_pipeline_layout, &r.note_pipeline);
  r.createRenderPass(&r.imgui_render_pass, false, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  r.createPipelineCache();
  r.createCommandPool(&r.cmd_pool, 0);
  r.createCommandPool(&r.imgui_cmd_pool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  r.createDepthResources();
  r.createFramebuffers();
  r.createImGuiFramebuffers();
  //r.createTextureImage();
  //r.createTextureImageView();
  //r.createTextureSampler();
  r.createVertexBuffer(instanced_quad, 4, r.note_vertex_buffer, r.note_vertex_buffer_mem);
  r.createInstanceBuffer(sizeof(InstanceData) * MAX_NOTES, r.note_instance_buffer, r.note_instance_buffer_mem);
  r.createIndexBuffer(instanced_quad_indis, 6, r.note_index_buffer, r.note_index_buffer_mem);
  r.createUniformBuffers(r.uniform_buffers, r.uniform_buffers_mem);
  r.createDescriptorPool();
  r.createImGuiDescriptorPool();
  r.createDescriptorSets();
  r.createCommandBuffers();
  r.createImGuiCommandBuffers();
  r.createSyncObjects();
  r.initImGui();
}

auto timer = std::chrono::steady_clock();
auto last_time = timer.now();
uint64_t frame_counter = 0;
uint64_t fps = 0;

void Main::mainLoop()
{
  static auto start_time = std::chrono::high_resolution_clock::now();
  r.pre_time = 0.25;
  midi->SpawnPlaybackThread(start_time);
  while(!glfwWindowShouldClose(r.window))
  {
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
    r.midi_renderer_time->store(time + r.pre_time);

    glfwPollEvents();
    r.drawFrame(time);

    //Output fps
    ++frame_counter;
    if(last_time + std::chrono::seconds(1) < timer.now())
    {
      last_time = timer.now();
      fps = frame_counter;
      frame_counter = 0;
      std::cout << std::endl << fps << " fps";
      std::string title = std::string("Chikara | FPS: ") + std::to_string(fps);
      glfwSetWindowTitle(r.window, title.c_str());
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
    vkDestroyFramebuffer(r.device, r.swap_chain_framebuffers[i], nullptr);
  for (size_t i = 0; i < r.imgui_swap_chain_framebuffers.size(); i++)
    vkDestroyFramebuffer(r.device, r.imgui_swap_chain_framebuffers[i], nullptr);

  vkFreeCommandBuffers(r.device, r.cmd_pool, static_cast<uint32_t>(r.cmd_buffers.size()), r.cmd_buffers.data());
  vkFreeCommandBuffers(r.device, r.imgui_cmd_pool, static_cast<uint32_t>(r.imgui_cmd_buffers.size()), r.imgui_cmd_buffers.data());

  vkDestroyPipeline(r.device, r.note_pipeline, nullptr);
  vkDestroyPipelineLayout(r.device, r.note_pipeline_layout, nullptr);
  vkDestroyRenderPass(r.device, r.note_render_pass, nullptr);
  vkDestroyRenderPass(r.device, r.imgui_render_pass, nullptr);

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
  vkDestroyDescriptorPool(r.device, r.imgui_descriptor_pool, nullptr);
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

  r.destroyImGui();

  cleanupSwapChain();

  r.createSwapChain();
  r.createImageViews();
  r.createRenderPass(&r.note_render_pass, true, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  r.createGraphicsPipeline(notes_v, notes_v_length, notes_f, notes_f_length, r.note_render_pass, &r.note_pipeline_layout, &r.note_pipeline);
  r.createRenderPass(&r.imgui_render_pass, false, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  r.createDepthResources();
  r.createFramebuffers();
  r.createImGuiFramebuffers();
  r.createUniformBuffers(r.uniform_buffers, r.uniform_buffers_mem);
  r.createDescriptorPool();
  r.createImGuiDescriptorPool();
  r.createDescriptorSets();
  r.createCommandBuffers();
  r.createImGuiCommandBuffers();
  r.initImGui();
}

#pragma endregion

#pragma region Cleanup

void Main::cleanup()
{
  if(enable_validation_layers)
  {
    r.DestroyDebugUtilsMessengerEXT(r.inst, r.debug_msg, nullptr);
  }

  r.destroyImGui();

  cleanupSwapChain();

  //vkDestroySampler(r.device, r.tex_sampler, nullptr);
  vkDestroyImageView(r.device, r.tex_img_view, nullptr);
  vkDestroyImage(r.device, r.tex_img, nullptr);
  vkFreeMemory(r.device, r.tex_img_mem, nullptr);

  vkDestroyDescriptorSetLayout(r.device, r.descriptor_set_layout, nullptr);

  vkDestroyBuffer(r.device, r.note_index_buffer, nullptr);
  vkFreeMemory(r.device, r.note_index_buffer_mem, nullptr);

  vkDestroyBuffer(r.device, r.note_instance_buffer, nullptr);
  vkFreeMemory(r.device, r.note_instance_buffer_mem, nullptr);

  vkDestroyBuffer(r.device, r.note_vertex_buffer, nullptr);
  vkFreeMemory(r.device, r.note_vertex_buffer_mem, nullptr);

  for(size_t i = 0; i < max_frames_in_flight; i++)
  {
    vkDestroySemaphore(r.device, r.render_fin_semaphore[i], nullptr);
    vkDestroySemaphore(r.device, r.img_available_semaphore[i], nullptr);
    vkDestroyFence(r.device, r.in_flight_fences[i], nullptr);
  }

  vkDestroyCommandPool(r.device, r.cmd_pool, nullptr);
  vkDestroyCommandPool(r.device, r.imgui_cmd_pool, nullptr);
  vkDestroyPipelineCache(r.device, r.pipeline_cache, nullptr);
  vkDestroyDevice(r.device, nullptr);
  vkDestroySurfaceKHR(r.inst, r.surface, nullptr);
  vkDestroyInstance(r.inst, nullptr);

  glfwDestroyWindow(r.window);

  glfwTerminate(); //Now we terminate
}

#pragma endregion

int wmain(int argc, wchar_t** argv)
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