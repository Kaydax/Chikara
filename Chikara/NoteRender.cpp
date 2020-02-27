#include "NoteRender.h"

NoteRender::NoteRender(Renderer* _parent, uint32_t _thread_count)
{
  parent = _parent;
  thread_count = _thread_count;
}

NoteRender::~NoteRender()
{

}


void NoteRender::generateWorkflow()
{
  VkDeviceSize buffer_size = sizeof(Vertex) * BUFFER_CAPACITY * 4;
  VkDeviceSize indicies_size = BUFFER_CAPACITY * 6;

  uint32_t bufferCount = BUFFERS_PER_THREAD * thread_count;
  uint32_t framebufferCount = (uint32_t)parent->swap_chain_framebuffers.size();
  uint32_t commandCount = bufferCount * framebufferCount;

  vertex_buffers = new VkBuffer[bufferCount];
  vertex_buffers_mem = new VkDeviceMemory[bufferCount];
  command_buffers = new VkCommandBuffer[commandCount];


  // generate index buffer

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_mem;

  parent->createBuffer(indicies_size * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_mem);

  void* data;
  vkMapMemory(parent->device, staging_buffer_mem, 0, indicies_size * sizeof(uint32_t), 0, &data);
  uint32_t* data_int = (uint32_t*)data;
  for(int i = 0; i < indicies_size / 6; i++)
  {
    data_int[i * 6 + 0] = i * 4 + 0;
    data_int[i * 6 + 1] = i * 4 + 1;
    data_int[i * 6 + 2] = i * 4 + 2;
    data_int[i * 6 + 3] = i * 4 + 2;
    data_int[i * 6 + 4] = i * 4 + 3;
    data_int[i * 6 + 5] = i * 4 + 0;
  }
  vkUnmapMemory(parent->device, staging_buffer_mem);

  parent->createBuffer(indicies_size * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_mem);
  parent->copyBuffer(staging_buffer, index_buffer, indicies_size * sizeof(uint32_t));

  vkDestroyBuffer(parent->device, staging_buffer, nullptr);
  vkFreeMemory(parent->device, staging_buffer_mem, nullptr);

  // generate command buffers

  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = parent->cmd_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = commandCount;

  if(vkAllocateCommandBuffers(parent->device, &alloc_info, command_buffers) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to allocate command buffers!");
  }

  for(int i = 0; i < bufferCount; i++)
  {
    // generate vertex buffer
    parent->createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertex_buffers[i], vertex_buffers_mem[i]);

    //Start recording the command buffer
    for(size_t j = 0; j < framebufferCount; j++)
    {
      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

      VkCommandBuffer cmdBuffer = command_buffers[j * bufferCount + i];

      if(vkBeginCommandBuffer(cmdBuffer, &begin_info) != VK_SUCCESS)
      {
        throw std::runtime_error("VKERR: Failed to begin recording command buffer!");
      }

      //Start the render pass
      VkRenderPassBeginInfo render_pass_info = {};
      render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      render_pass_info.renderPass = parent->render_pass;
      render_pass_info.framebuffer = parent->swap_chain_framebuffers[j]; //Create a framebuffer for each swap chain image
      render_pass_info.renderArea.offset = { 0, 0 };
      render_pass_info.renderArea.extent = parent->swap_chain_extent; //Render area defines where shader loads and stores take place. Anything outside this area are undefined, allowing for better performance

      std::array<VkClearValue, 2> clear_values = {};
      clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
      clear_values[1].depthStencil = { 1.0f, 0 };

      render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
      render_pass_info.pClearValues = clear_values.data();

      //Now the render pass can begin
      vkCmdBeginRenderPass(cmdBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, parent->graphics_pipeline);

      VkBuffer buffers[] = { vertex_buffers[i] };
      VkDeviceSize offsets[] = { 0 };
      vkCmdBindVertexBuffers(cmdBuffer, 0, 1, buffers, offsets);
      vkCmdBindIndexBuffer(cmdBuffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
      vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, parent->pl_layout, 0, 1, &parent->descriptor_sets[j], 0, nullptr);

      vkCmdDrawIndexed(cmdBuffer, indicies_size, 1, 0, 0, 0);

      vkCmdEndRenderPass(cmdBuffer);

      if(vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
      {
        throw std::runtime_error("VKERR: Failed to record command buffer!");
      }
    }
  }

  VkSemaphoreCreateInfo semaphore_info = {};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  keySemaphores = new VkSemaphore[bufferCount];
  for(int i = 0; i < bufferCount; i++)
  {
    if(vkCreateSemaphore(parent->device, &semaphore_info, nullptr, &keySemaphores[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("VKERR: Failed to create synchronization objects for a frame!");
    }
  }
}

uint32_t NoteRender::getVertexBuffer(uint32_t thread, uint32_t b)
{
  return thread * BUFFERS_PER_THREAD + (b % BUFFERS_PER_THREAD);
}

uint32_t NoteRender::getCommandBuffer(uint32_t thread, uint32_t b)
{
  return getVertexBuffer(thread, (b % BUFFERS_PER_THREAD)) + parent->current_frame_index * BUFFERS_PER_THREAD * thread_count;
}

void NoteRender::renderFrame(list<Note*>** note_buffer)
{
  vkResetFences(parent->device, 1, &parent->in_flight_fences[parent->current_frame]);

  VkDeviceSize buffer_size = sizeof(Vertex) * BUFFER_CAPACITY * 4;
  VkDeviceSize indicies_size = sizeof(Vertex) * BUFFER_CAPACITY * 6;

  uint32_t bufferCount = BUFFERS_PER_THREAD * thread_count;
  uint32_t framebufferCount = (uint32_t)parent->swap_chain_framebuffers.size();
  uint32_t commandCount = bufferCount * framebufferCount;

  VkSemaphore* lastSemaphores = new VkSemaphore[thread_count];
  for(int i = 0; i < thread_count; i++) lastSemaphores[i] = VK_NULL_HANDLE;

  VkSemaphore* lastsmf = &parent->img_available_semaphore[parent->current_frame];

  for(int key = 0; key < 256; key++)
  {
    uint32_t threadId = key % thread_count;
    list<Note*>* notes = note_buffer[key];
    int n = 0;
    uint32_t b = 0;

    uint32_t vb = getVertexBuffer(threadId, b);
    VkDeviceMemory vertex_buffer_mem = vertex_buffers_mem[vb];
    VkBuffer vertex_buffer = vertex_buffers[vb];
    VkCommandBuffer cmdBuffer = command_buffers[getCommandBuffer(threadId, b)];

    void* data;
    vkMapMemory(parent->device, vertex_buffer_mem, 0, buffer_size, 0, &data);

    Vertex* data_v = (Vertex*)data;

    data_v[0] = { {0,0},{1,1,1},{0,1} };
    data_v[1] = { {1,0},{1,1,1},{1,1} };
    data_v[2] = { {1,1},{1,1,1},{1,0} };
    data_v[3] = { {0,1},{1,1,1},{0,0} };

    //for(auto const& i : *notes)
    //{
    //  if(n >= BUFFER_CAPACITY)
    //  {
    //    vkUnmapMemory(parent->device, vertex_buffer_mem);

    //    //Submit to the command buffer
    //    VkSubmitInfo submit_info = {};
    //    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;


    //    //VkSemaphore wait_semaphores[] = { parent->img_available_semaphore[parent->current_frame] }; //The semaphores we are waiting for
    //    submit_info.pWaitSemaphores = lastsmf;
    //    submit_info.waitSemaphoreCount = 1;
    //    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; //Wait with writing colors until the image is available
    //    submit_info.pWaitDstStageMask = wait_stages;
    //    submit_info.commandBufferCount = 1;
    //    submit_info.pCommandBuffers = &cmdBuffer;

    //    submit_info.signalSemaphoreCount = 1;
    //    submit_info.pSignalSemaphores = &keySemaphores[getVertexBuffer(threadId, b)];

    //    lastSemaphores[threadId] = keySemaphores[getVertexBuffer(threadId, b)];
    //    lastsmf = &keySemaphores[getVertexBuffer(threadId, b)];

    //    if(vkQueueSubmit(parent->graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
    //    {
    //      throw std::runtime_error("VKERR: Failed to submit draw to command buffer!");
    //    }

    //    b++;
    //    n = 0;

    //    uint32_t vb = getVertexBuffer(0, b);
    //    vertex_buffer_mem = vertex_buffers_mem[vb];
    //    vertex_buffer = vertex_buffers[vb];
    //    cmdBuffer = command_buffers[getCommandBuffer(0, b)];

    //    vkMapMemory(parent->device, vertex_buffer_mem, 0, buffer_size, 0, &data);

    //    data_v = (Vertex*)data;
    //  }
    //  n++;
    //}

    vkUnmapMemory(parent->device, vertex_buffer_mem);

    //Submit to the command buffer
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //VkSemaphore wait_semaphores[] = { parent->img_available_semaphore[parent->current_frame] }; //The semaphores we are waiting for
    submit_info.pWaitSemaphores = lastsmf;
    submit_info.waitSemaphoreCount = 1;
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; //Wait with writing colors until the image is available
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmdBuffer;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &keySemaphores[getVertexBuffer(threadId, b)];

    //lastSemaphores[threadId] = keySemaphores[getVertexBuffer(threadId, b)];
    //lastsmf = &keySemaphores[getVertexBuffer(threadId, b)];

    //if(vkQueueSubmit(parent->graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
    //{
    //  throw std::runtime_error("VKERR: Failed to submit draw to command buffer!");
    //}
  }

  parent->next_step_semaphores.clear();
  //parent->next_step_semaphores.resize(thread_count);
  parent->next_step_semaphores.push_back(*lastsmf);
  //for(int i = 0; i < thread_count; i++) parent->next_step_semaphores.push_back(lastSemaphores[i]);

  delete[] lastSemaphores;

  //VkDeviceMemory vertex_buffer_mem = vertex_buffers_mem[0];
  //VkBuffer vertex_buffer = vertex_buffers[0];
  //VkCommandBuffer cmdBuffer = command_buffers[parent->current_frame_index * bufferCount + 0];

  //void* data;
  //vkMapMemory(parent->device, vertex_buffer_mem, 0, buffer_size, 0, &data);

  //Vertex* data_v = (Vertex*)data;
  //data_v[0] = { {0,0},{1,1,1},{0,1} };
  //data_v[1] = { {1,0},{1,1,1},{1,1} };
  //data_v[2] = { {1,1},{1,1,1},{1,0} };
  //data_v[3] = { {0,1},{1,1,1},{0,0} };



  if(vkQueueSubmit(parent->graphics_queue, 0, 0, parent->in_flight_fences[parent->current_frame]) != VK_SUCCESS)
  {
    throw std::runtime_error("VKERR: Failed to submit draw to command buffer!");
  }
}