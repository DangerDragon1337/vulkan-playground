#include <fstream>
#include <functions.hpp>
#include <ios>
#include <stdexcept>

namespace functions
{
    std::vector<const char*> getRequiredInstanceExtensions()
    {
        uint32_t glfwExtensionCount{0};
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (layers::enableValidationLayers)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }
        return extensions;
    }

    std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary); // Opens the file at the end in binary cuz text causes errors or smth

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open the file");
        }

        std::vector<char> buffer(file.tellg()); // tellg gives size till pointer and we at end
        file.seekg(0, std::ios::beg); // goes to beginning ig
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        file.close();
        return buffer;
    }
}