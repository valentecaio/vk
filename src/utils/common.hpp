#pragma once

// glfw
// replace the include <vulkan/vulkan.h>, needs to be before glfw3.h
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// tinyobjloader (under lib/)
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// stb (under lib/)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// glm (under lib/glm-1.0.1/)
// https://docs.vulkan.org/tutorial/latest/05_Uniform_buffers/01_Descriptor_pool_and_sets.html#_alignment_requirements
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // force GLM to use aligned types in structs
#define GLM_FORCE_DEPTH_ZERO_TO_ONE        // openGL uses [-1, 1] for depth, vulkan uses [0, 1]
#define GLM_ENABLE_EXPERIMENTAL            // enable hash for glm::vec3
#include <glm.hpp>
#include <gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <gtx/hash.hpp>


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
#include <array>
#include <chrono>


namespace vk {

// constants
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

const int MAX_FRAMES_IN_FLIGHT = 2;

}
