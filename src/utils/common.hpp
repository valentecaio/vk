#pragma once

// glfw
// replace the include <vulkan/vulkan.h> (needs to be before the include of glfw3.h)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// lib/tiny_obj_loader
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// lib/stb
// #define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// lib/tiny_gltf
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

// lib/glm
// https://docs.vulkan.org/tutorial/latest/05_Uniform_buffers/01_Descriptor_pool_and_sets.html#_alignment_requirements
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // force GLM to use aligned types in structs
#define GLM_FORCE_DEPTH_ZERO_TO_ONE        // openGL uses [-1, 1] for depth, vulkan uses [0, 1]
#define GLM_ENABLE_EXPERIMENTAL            // enable hash for glm::vec3
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale
#include <glm/gtx/hash.hpp>


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


// constants
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

