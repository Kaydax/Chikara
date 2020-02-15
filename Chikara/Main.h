#include "Renderer.h"
#include "Midi.h"

class Main
{
  public:
    void run();
    void recreateSwapChain();
  private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanupSwapChain();
    void cleanup();
};