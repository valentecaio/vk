#pragma once

// glfw
// replace the include <vulkan/vulkan.h>, needs to be before glfw3.h
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// glm
// openGL uses [-1, 1] for depth, vulkan uses [0, 1]
#define GLM_FORCE_DEPTH_ZERO_TO_ONE


// standard
#include <iostream>  // std::cerr, std::endl
#include <stdexcept> // std::exception, std::runtime_error
#include <fstream>   // std::ifstream
#include <cstdlib>   // EXIT_SUCCESS, EXIT_FAILURE
#include <cstring>   // strcmp
#include <map>       // std::multimap
#include <algorithm> // std::clamp
#include <limits>    // std::numeric_limits
#include <vector>
#include <set>
#include <optional>


namespace vk {

// constants
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

}
