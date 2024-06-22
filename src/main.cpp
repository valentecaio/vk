
#include "utils/common.hpp"
#include "utils/utils.hpp"

#include "vk/kilauea.hpp"

using namespace vk;

// application class //

class Application {
public:
  void run() {
    initWindow();
    kilauea.init(window);
    mainLoop();
    cleanup();
  }

private:
  GLFWwindow* window;  // window handle
  Kilauea kilauea;     // vulkan handle

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable openGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // disable resizing

    // 4th argument is monitor (fullscreen), 5th is share (only for openGL)
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    // int c;
    // auto monitors = glfwGetMonitors(&c);
    // window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", monitors[0], nullptr);
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      kilauea.drawFrame();
    }

    kilauea.waitIdle();
  }

  void cleanup() {
    kilauea.cleanup();

    // glfw
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};

int main() {
  Application app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}