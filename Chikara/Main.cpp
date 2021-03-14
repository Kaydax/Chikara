#include "Main.h"
#include "KDMAPI.h"
#include "Config.h"
#include "Utils.h"
#include "Platform.h"

#include <inttypes.h>
#include <fmt/locale.h>
#include <fmt/format.h>

// msvc complains about narrowing conversion with bin2c
#pragma warning(push)
#pragma warning(disable : 4838)
#pragma warning(disable : 4309)
#include "Shaders/notes_f.h"
#include "Shaders/notes_v.h"
#include "Shaders/notes_g.h"
#pragma warning(pop)

Renderer r;
GlobalTime* gtime;
Midi* midi;
MidiTrack* trk;

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
  std::wstring filename;
  if (argc < 2) {
    filename = Platform::OpenMIDIFileDialog();
    if (filename.empty())
      return;
  } else {
    filename = argv[1];
  }

  auto config_path = Config::GetConfigPath();
  Config::GetConfig().Load(config_path);
  KDMAPI::Init();
  SetConsoleOutputCP(65001); // utf-8
  fmt::print("Loading {}\n", Utils::wstringToUtf8(Utils::GetFileName(filename)));

  try
  {
    std::cout << "RPC Enabled: " << Config::GetConfig().discord_rpc << std::endl;
    if(Config::GetConfig().discord_rpc) Utils::InitDiscord();
  } catch(const std::exception& e)
  {
    std::cout << "RPC Enabled: 0 (Discord Not Installed)" << std::endl;
    Config::GetConfig().discord_rpc = false;
  }
  
  wchar_t* filename_temp = _wcsdup(filename.c_str());
  midi = new Midi(filename_temp);
  r.note_event_buffer = midi->note_event_buffer;
  r.midi_renderer_time = &midi->renderer_time;
  r.note_stacks.resize(midi->track_count * 16);
  r.note_count = &midi->note_count;
  r.notes_played = &midi->notes_played;
  r.song_len = midi->song_len;
  // playback thread spawned in mainLoop to ensure it's synced with render
  //printf(file_name);
  midi->SpawnLoaderThread();
  initWindow(filename); //Setup everything for the window
  initVulkan(); //Setup everything for Vulkan
  gt = new GlobalTime(Config::GetConfig().start_delay);
  gtime = gt;
  mainLoop(filename); //The main loop for the application
  cleanup(); //Cleanup everything because we closed the application
  free(filename_temp);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    exit(1);
  if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
  {
    r.paused = !r.paused;
    r.paused ? gtime->pause() : gtime->resume();
  }
}

void Main::initWindow(std::wstring midi)
{
  glfwInit(); //Init glfw
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //Set the glfw api to GLFW_NO_API because we are using Vulkan
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); //Change the ability to resize the window
  if(Config::GetConfig().transparent)
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); //Window Transparancy
  auto filename = Utils::wstringToUtf8(Utils::GetFileName(midi));
  
  if(Config::GetConfig().fullscreen)
  {
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    r.window = glfwCreateWindow(mode->width, mode->height, std::string("Chikara | " + filename).c_str(), glfwGetPrimaryMonitor(), nullptr); //Now we create the window
    r.window_width = mode->width;
    r.window_height = mode->height;
  } else {
    r.window = glfwCreateWindow(default_width, default_height, std::string("Chikara | " + filename).c_str(), nullptr, nullptr); //Now we create the window
  }
  glfwSetWindowUserPointer(r.window, &r);
  glfwSetFramebufferSizeCallback(r.window, r.framebufferResizeCallback);
  glfwSetKeyCallback(r.window, key_callback);
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
  r.createRenderPass(&r.additional_note_render_pass, true, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  r.createDescriptorSetLayout();
  r.createGraphicsPipeline(notes_v, notes_v_length, notes_f, notes_f_length, notes_g, notes_g_length, r.note_render_pass, &r.note_pipeline_layout, &r.note_pipeline);
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
  //r.createVertexBuffer(instanced_quad, 4, r.note_vertex_buffer, r.note_vertex_buffer_mem);
  //r.createInstanceBuffer(sizeof(InstanceData) * MAX_NOTES, r.note_instance_buffer, r.note_instance_buffer_mem);
  //r.createIndexBuffer(instanced_quad_indis, 6, r.note_index_buffer, r.note_index_buffer_mem);
  r.createNoteDataBuffer(sizeof(NoteData) * MAX_NOTES, r.note_buffer, r.note_buffer_mem);
  r.createUniformBuffers(r.uniform_buffers, r.uniform_buffers_mem);
  r.createDescriptorPool();
  r.createImGuiDescriptorPool();
  r.createDescriptorSets();
  r.createCommandBuffers();
  r.createImGuiCommandBuffers();
  r.createSyncObjects();
  r.initImGui();
  r.PrepareKeyboard();
}

auto timer = std::chrono::steady_clock();
auto last_time = timer.now();
uint64_t frame_counter = 0;
uint64_t fps = 0;

void Main::mainLoop(std::wstring midi_name)
{
  static auto start_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds((long long)Config::GetConfig().start_delay);

  long long start = (std::chrono::system_clock::now().time_since_epoch() + std::chrono::seconds((long long)Config::GetConfig().start_delay)) / std::chrono::milliseconds(1);
  long long end_time = (std::chrono::system_clock::now().time_since_epoch() + std::chrono::seconds(5) + std::chrono::seconds((long long)midi->song_len)) / std::chrono::milliseconds(1);
  midi->SpawnPlaybackThread(gt, Config::GetConfig().start_delay);
  /*
  char buffer[256];
  sprintf(buffer, "Note Count: %s", fmt::format(std::locale(""), "{:n}", midi->note_count));
  */
  if (Config::GetConfig().discord_rpc) {
    auto rpc_text = fmt::format(std::locale(""), "Note Count: {:n}", midi->note_count);
    Utils::UpdatePresence(rpc_text.c_str(), "Playing: ", Utils::wstringToUtf8(Utils::GetFileName(midi_name)), (uint64_t)start, (uint64_t)end_time);
  }
  while(!glfwWindowShouldClose(r.window))
  {
    r.pre_time = Config::GetConfig().note_speed;
    //float time;
    //auto current_time = std::chrono::high_resolution_clock::now();
    //time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
    r.midi_renderer_time->store(gt->getTime() + r.pre_time);
    
    glfwPollEvents();
    r.drawFrame(gt);
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
  vkDestroyRenderPass(r.device, r.additional_note_render_pass, nullptr);
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
  r.window_width = width;
  r.window_height = height;

  vkDeviceWaitIdle(r.device);

  r.destroyImGui();

  cleanupSwapChain();

  r.createSwapChain();
  r.createImageViews();
  r.createRenderPass(&r.note_render_pass, true, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  r.createRenderPass(&r.additional_note_render_pass, true, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  r.createGraphicsPipeline(notes_v, notes_v_length, notes_f, notes_f_length, notes_g, notes_g_length, r.note_render_pass, &r.note_pipeline_layout, &r.note_pipeline);
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
  r.PrepareKeyboard();
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

  vkDestroyBuffer(r.device, r.note_buffer, nullptr);
  vkFreeMemory(r.device, r.note_buffer_mem, nullptr);

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
  Utils::DestroyDiscord();
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