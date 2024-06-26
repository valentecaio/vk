
#include "utils/common.hpp"
#include "utils/utils.hpp"

#include "vk/kilauea.hpp"
#include "vk/vertex.hpp"

using namespace vk;

#define RESIZABLE true

class Application {
  public:
    void run() {
      initWindow();
      kilauea = Kilauea(window);
      kilauea.init();

      // resize callback
      if (RESIZABLE) {
        glfwSetWindowUserPointer(window, &kilauea);
        glfwSetFramebufferSizeCallback(window, Kilauea::framebufferResizeCallback);
      }

      mainLoop();
      cleanup();
    }

  private:
    GLFWwindow* window;  // window handle
    Kilauea kilauea;     // vulkan handle

    void initWindow() {
      glfwInit();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // disable openGL context
      if (!RESIZABLE)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

      // 4th argument is monitor (fullscreen), 5th is share (only for openGL)
      window = glfwCreateWindow(WIDTH, HEIGHT, "Kilauea", nullptr, nullptr);
    }

    void mainLoop() {
      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // TODO: update vertices
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