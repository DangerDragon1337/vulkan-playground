#pragma once

#include <cstddef>
#include <utility>
#define VULKAN_HPP_NO_CONSTRUCTORS
#include "vulkan/vulkan.hpp"
#include <vector>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN

#include <cstdint>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class MakeTriangle
{
private:
    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;

        static vk::VertexInputBindingDescription getBindingDescription()
        {
            return vk::VertexInputBindingDescription {
                .binding = 0, // Binding this buffer to slot 0 (can have multiple buffers)
                .stride = sizeof(Vertex), // Stride for the next vertex/instance
                .inputRate = vk::VertexInputRate::eVertex, // Move to the next vertex after data entry (eInstance can also be used)
            };
        }

        static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
        {
            return {
                vk::VertexInputAttributeDescription {
                    .location = 0, // Matches the location of the variable in the shader
                    .binding = 0,
                    .format = vk::Format::eR32G32Sfloat, // Two 32-bit signed floats
                    .offset = offsetof(Vertex, pos), // diff where the vertex starts and this attribute starts in bytes
                },
                vk::VertexInputAttributeDescription {
                    .location = 1,
                    .binding = 0,
                    .format = vk::Format::eR32G32B32Sfloat,
                    .offset = offsetof(Vertex, color),
                },
            };
        }
    };
    
    GLFWwindow *window{nullptr};
    bool framebufferResized{false};
    vk::raii::Context context;
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
    vk::raii::SurfaceKHR surface{nullptr};
    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device device{nullptr};
    uint32_t graphicsQueueIndex = ~0;
    vk::raii::Queue graphicsQueue{nullptr};
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    vk::raii::SwapchainKHR swapChain{nullptr};
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::raii::ImageView> swapChainImageViews;
    vk::raii::PipelineLayout pipelineLayout{nullptr};
    vk::raii::Pipeline graphicsPipeline{nullptr};
    vk::raii::CommandPool graphicsCommandPool{nullptr};
    vk::raii::CommandPool transferCommandPool{nullptr};
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    vk::raii::Buffer vertexBuffer{nullptr};
    vk::raii::Buffer indexBuffer{nullptr};
    vk::raii::DeviceMemory vertexBufferMemory{nullptr};
    vk::raii::DeviceMemory indexBufferMemory{nullptr};
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;
    uint32_t frameIndex{0};

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void initWindow();
    void createInstance();
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT *pCallBackData,
        void *pUserData
    );
    void setupDebugMessenger();
    void createSurface();
    bool isDeviceSuitable(vk::raii::PhysicalDevice const &physicalDevice) const;
    void pickPhysicalDevice();
    void createLogicalDevice();
    uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) const;
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilites) const;
    void createSwapChain();
    void cleanupSwapChain();
    void recreateSwapChain();
    void createImageViews();
    void getVertices();
    void getIndices();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    void createGraphicsPipeline();
    void createCommandPool();
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties
    );
    void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize bufferSize);
    void createVertexBuffer();
    void createIndexBuffer();
    void createCommandBuffers();
    void transition_image_layout(
        uint32_t imageIndex,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask
    ) const;
    void recordCommandBuffer(uint32_t imageIndex);
    void createSyncObjects();
    void drawFrame();
    void initVulkan();
    void mainLoop();
    void cleanup() const;

public:
    void run();
};
