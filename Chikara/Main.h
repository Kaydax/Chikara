#include <string>
#include "Renderer.h"
#include "Midi.h"

class Main
{
  public:
    void run(int argc, wchar_t** argv);
    void recreateSwapChain();
  private:
    void initWindow(std::wstring midi);
    void initVulkan();
    void mainLoop(std::wstring midi);
    void cleanupSwapChain();
    void cleanup();
};