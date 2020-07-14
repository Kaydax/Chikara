#include "Renderer.h"
#include "Midi.h"

class Main
{
  public:
    void run(int argc, wchar_t** argv);
    void recreateSwapChain();
  private:
    void initWindow();
    void initVulkan();
    void mainLoop(wchar_t** argv);
    void cleanupSwapChain();
    void cleanup();
};