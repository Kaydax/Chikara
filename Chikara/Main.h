#include "Renderer.h"
#include "Midi.h"

class Main
{
  public:
    void run(int argc, char** argv);
    void recreateSwapChain();
  private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanupSwapChain();
    void cleanup();
};