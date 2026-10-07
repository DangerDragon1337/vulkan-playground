#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <constants.hpp>
#include <cstdint>
#include <fstream>
#include <GLFW/glfw3.h>
#include <vector>
namespace functions
{
    std::vector<const char*> getRequiredInstanceExtensions();
    std::vector<char> readFile(const std::string& filename);
}