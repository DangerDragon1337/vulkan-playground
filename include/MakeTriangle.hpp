#pragma once

#include <sys/types.h>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN

#include "constants.hpp"
#include "functions.hpp"
#include <cstdint>
#include <limits>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <iostream>
#include <map>

class MakeTriangle
{
private:
    GLFWwindow *window{nullptr};
    vk::raii::Context context;
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
    vk::raii::SurfaceKHR surface{nullptr};
    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device device{nullptr};
    uint32_t queueIndex = ~0;
    vk::raii::Queue graphicsQueue{nullptr};
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    vk::raii::SwapchainKHR swapChain{nullptr};
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::raii::ImageView> swapChainImageViews;
    vk::raii::PipelineLayout pipelineLayout{nullptr};
    vk::raii::Pipeline graphicsPipeline{nullptr};
    vk::raii::CommandPool commandPool{nullptr};
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;
    uint32_t frameIndex{0};

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
    bool isDeviceSuitable(
        vk::raii::PhysicalDevice const &physicalDevice
    ) const;
    void pickPhysicalDevice();
    void createLogicalDevice();
    u_int32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) const;
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilites) const;
    void createSwapChain();
    void createImageViews();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    void createGraphicsPipeline();
    void createCommandPool();
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
