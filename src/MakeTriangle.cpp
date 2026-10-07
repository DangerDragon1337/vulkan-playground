#include "MakeTriangle.hpp"
#include "constants.hpp"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <complex>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

void MakeTriangle::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(constants::SCREEN_WIDTH, constants::SCREEN_HEIGHT, "Vulkan", nullptr, nullptr);
}

void MakeTriangle::createInstance()
{
    constexpr vk::ApplicationInfo appInfo
    {
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .apiVersion = vk::ApiVersion14,
    };

    auto requiredExtensions = functions::getRequiredInstanceExtensions();
    auto extensionProperties = context.enumerateInstanceExtensionProperties();

    auto unsupportedPropertyIt = std::ranges::find_if(requiredExtensions, [&extensionProperties](const auto& requiredExtension)
    {
        return std::ranges::none_of(extensionProperties, [requiredExtension](const auto& extensionProperty)
        {
            return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
        });
    });
    if (unsupportedPropertyIt != requiredExtensions.end())
    {
        throw std::runtime_error( "Required extension not supported: " + std::string(*unsupportedPropertyIt) );
    }

    std::vector<const char*> requiredLayers;
    if (layers::enableValidationLayers)
    {
        requiredLayers.assign(layers::validationLayers.begin(), layers::validationLayers.end());
    }

    auto layerProperties = context.enumerateInstanceLayerProperties();
    auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, [&layerProperties](const auto& requiredLayer)
    {
        return std::ranges::none_of(layerProperties, [requiredLayer](const auto& layerProperty)
        {
            return strcmp(layerProperty.layerName, requiredLayer) == 0;
        });
    });
    if (unsupportedLayerIt != requiredLayers.end())
    {
        throw std::runtime_error( "Required layer not supported: " + std::string(*unsupportedLayerIt) );
    }

    vk::InstanceCreateInfo createInfo
    {
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<u_int32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<u_int32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    instance = vk::raii::Instance(context, createInfo);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL MakeTriangle::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallBackData,
    void *pUserData
)
{
    std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallBackData->pMessage << std::endl;
    return vk::False;
}

void MakeTriangle::setupDebugMessenger()
{
    if (!(layers::enableValidationLayers)) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags
    (
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );

    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags
    (
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
    );

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT
    {
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &debugCallback,
    };

    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

void MakeTriangle::createSurface()
{
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
    {
        throw std::runtime_error("Failed to create a window surface!");
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);
}

bool MakeTriangle::isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice) const
{
    bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

    auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    bool supportGraphics = std::ranges::any_of
    (
        queueFamilies, [](const auto& qfp)
        {
            return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
        }
    );

    std::vector<const char*> requiredDeviceExtension = { vk::KHRSwapchainExtensionName };
    auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    bool supportsAllExtensions = std::ranges::all_of
    (
        requiredDeviceExtension, [&availableDeviceExtensions](const auto& requiredDeviceExtension)
        {
            return std::ranges::any_of
            (
                availableDeviceExtensions, [requiredDeviceExtension](const auto& availableDeviceExtension)
                {
                    return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
                }
            );
        }
    );

    auto features = physicalDevice.template getFeatures2
    <
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    >();
    bool supportedRequiredFeatures =
    {
        features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState
    };

    return supportsVulkan1_3 && supportGraphics && supportsAllExtensions && supportedRequiredFeatures;
}

void MakeTriangle::pickPhysicalDevice()
{
    auto physicalDevices = vk::raii::PhysicalDevices( instance );
    if (physicalDevices.empty())
    {
        throw std::runtime_error( "failed to find GPUs with Vulkan support!" );
    }

    std::multimap<int, vk::raii::PhysicalDevice> candidates;

    for (const auto& pd : physicalDevices)
    {
        auto deviceProperties = pd.getProperties();
        auto deviceFeatures = pd.getFeatures();
        u_int32_t score = 0;

        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            score += 1000;
        }

        score += deviceProperties.limits.maxImageDimension2D;

        if (!deviceFeatures.geometryShader)
        {
            continue;
        }
        candidates.insert(std::make_pair(score, pd));
    }

    if (!candidates.empty() && candidates.rbegin()->first > 0)
    {
        physicalDevice = candidates.rbegin()->second;
    }
    else
    {
        throw std::runtime_error( "failed to find a suitable GPU!" );
    }
}

void MakeTriangle::createLogicalDevice()
{
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    for (uint32_t qfpIndex{0}; qfpIndex < queueFamilyProperties.size(); ++qfpIndex)
    {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            (physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)))
        {
            queueIndex = qfpIndex;
            break;
        }
    }

    if (queueIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    auto graphicsQueueFamilyProperty = std::ranges::find_if
    (
        queueFamilyProperties, [](const auto& qfp)
        {
            return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
        }
    );
    auto graphicsIndex = static_cast<u_int32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
    float queuePriority = 0.5f;
    
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo =
    {
        .queueFamilyIndex = graphicsIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    vk::StructureChain
    <
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    >
    featureChain =
    {
        { },
        { .shaderDrawParameters = true },
        { .dynamicRendering = true },
        { .extendedDynamicState = true },
    };

    std::vector<const char*> requiredDeviceExtension = { vk::KHRSwapchainExtensionName };

    vk::DeviceCreateInfo deviceCreateInfo
    {
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<u_int32_t>(requiredDeviceExtension.size()),
        .ppEnabledExtensionNames = requiredDeviceExtension.data(),
    };

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
}

u_int32_t MakeTriangle::chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) const
{
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);

    if (surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.maxImageCount < minImageCount)
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }

    return minImageCount;
}

vk::SurfaceFormatKHR MakeTriangle::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const
{
    assert(!availableFormats.empty());
    const auto formatIt = std::ranges::find_if
    (
        availableFormats, [](const auto& format)
        {
            return format.format == vk::Format::eB8G8R8A8Srgb && // B8 -> 8 bits for blue and so on...
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        }
    );
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR MakeTriangle::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const
{
    assert(
        std::ranges::any_of(
            availablePresentModes, [](auto presentMode)
            {
                return presentMode == vk::PresentModeKHR::eFifo;
            }
        )
    );

    return std::ranges::any_of(
        availablePresentModes, [](const vk::PresentModeKHR value)
        {
            return vk::PresentModeKHR::eMailbox == value;
        }
    ) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

vk::Extent2D MakeTriangle::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<u_int32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return
    {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
}

void MakeTriangle::createSwapChain()
{
    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    u_int32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

    std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR( *surface );
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

    std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR( *surface );
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo
    {
        .surface = *surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.format, // How the image is stored
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = true
    };

    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    swapChainImages = swapChain.getImages(); // Gets the memory locations of the image buffers allocated in the gpu
}

// ImageViews tells how to access the vk::Image in VRAM
void MakeTriangle::createImageViews()
{
    assert(!swapChainImages.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo
    {
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    for (const auto& image : swapChainImages)
    {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }
}

// [[nodiscard]] thing avoids using this function without assignment
[[nodiscard]] vk::raii::ShaderModule MakeTriangle::createShaderModule(const std::vector<char>& code) const
{
    vk::ShaderModuleCreateInfo createInfo
    {
        .codeSize = code.size() * sizeof(char),
        // SPIR-V needs 32 ubit words and shi
        .pCode = reinterpret_cast<const uint32_t*>(code.data()), // reinterpret_cast changes the type of pointer but doesn't modify data
    };

    vk::raii::ShaderModule shaderModule{ device, createInfo };
    return shaderModule;
}

void MakeTriangle::createGraphicsPipeline()
{
    auto shaderCode = functions::readFile("shaders/Shaders.spv");
    vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo
    {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain",
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo
    {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain",
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

    // TriangleList will do (1, 2, 3), (4, 5, 6), TriangleStrip will do (1, 2, 3), (2, 3, 4)
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly
    {
        .topology = vk::PrimitiveTopology::eTriangleList,
    };

    // Viewport is the transformation of the image in the swap chain
    vk::Viewport viewport
    {
        .x = 0.0f,
        .y = 0.0f,
        // Swapchain Extent is the resolution of the image in the swapchain
        .width = static_cast<float>(swapChainExtent.width),
        .height = static_cast<float>(swapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    // Scissor rectangle will be the the one rasterizer doesn't discard, rest it discards like a cutoff rectangle
    vk::Rect2D scissor{ vk::Offset2D{ 0, 0 }, swapChainExtent };

    std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vk::PipelineDynamicStateCreateInfo dynamicState
    {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    vk::PipelineViewportStateCreateInfo viewportState
    {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer
    {
        // Clamps the z value of objects behind the near plane as z = 0 and ahead of the far plane as z = 1
        .depthClampEnable = vk::False, // Used for lights at infinity (orthographic projection) (only for near plane)
        .rasterizerDiscardEnable = vk::False, // Won't rasterize if set to true, basically when you don't need to see your simulation
        .polygonMode = vk::PolygonMode::eFill, // Other things are lines and points instead of fill
        .cullMode = vk::CullModeFlagBits::eBack, // Can be disabled if the object is just one layer thick with only one normal which'll always render invisble from one side
        .frontFace = vk::FrontFace::eClockwise, // Usually counterClockwise but Vulkan uses +Y downwards, OpenGL has it upwards and can be used to switch between the two without causing issues when it gets flipped
        .depthBiasClamp = vk::False, // During shadow mapping, if bias is not added, sometimes the center of the fragment can be behind the approximated (floats) value that is given leading to the fragment shadowing itself but if the bias is too much from the slope factor, it can lead to the shadow being far away than what it should be, leading to peter-panning
        .lineWidth = 1.0f, // means 1 pixel wide
    };

    // Way easier to do than upscaling the whole image, using fragment shader and then downscaling
    vk::PipelineMultisampleStateCreateInfo multisampling
    {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    // This is per framebuffer
    vk::PipelineColorBlendAttachmentState colorBlendAttachment // Research on this later
    {
        .blendEnable = vk::False, // Used for the first method of blending (mixing)
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA,
    };

    // This is for every framebuffer
    vk::PipelineColorBlendStateCreateInfo colorBlending
    {
        .logicOpEnable = vk::False, // Used for the second method of blending (bitwise ops), will disable the methods above though
        .logicOp = vk::LogicOp::eCopy, // Safe fallback (can be removed, tutorial just mentioned this)
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    // Used for uniforms in shaders (needed even when no uniforms are there)
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo
    {
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };
    pipelineLayout = vk::raii::PipelineLayout( device, pipelineLayoutInfo );

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo
    {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainSurfaceFormat.format,
    };

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo
    {
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = nullptr,
    };

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = 
    {
        graphicsPipelineCreateInfo,
        pipelineRenderingCreateInfo,
    };

    // 2nd argument is for PipelineCache for stuff that is common across pipelines
    graphicsPipeline = vk::raii::Pipeline( device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>() );
}

void MakeTriangle::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo
    {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // Makes it so resetting (clearing their recording) command buffers can happen individually
        .queueFamilyIndex = queueIndex,
    };

    commandPool = vk::raii::CommandPool( device, poolInfo );
}

void MakeTriangle::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo
    {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary, // primary is for execution but cannot be called from other command buffers, secondary is vice versa
        .commandBufferCount = constants::MAX_FRAMES_IN_FLIGHT,
    };

    commandBuffers = vk::raii::CommandBuffers( device, allocInfo ); // CommandBuffers generates a vector
}

void MakeTriangle::transition_image_layout(
    uint32_t imageIndex,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
) const
{
    // Pipeline barriers are used to make sure there are no race conditions (between stages or stored data)
    // Stage -> wait, Access -> do
    // This barrier targets vk::Image
    vk::ImageMemoryBarrier2 barrier
    {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask, // Flushes data to the vram from gpu caches to make it globally available
        .dstStageMask = dst_stage_mask, // Runs after src_stage_mask is done (like eFragmentShader after eColorAttachmentOutput)
        .dstAccessMask = dst_access_mask, // Invalidates (clears out) old data in caches and fetches data from vram for quick usage
        .oldLayout = old_layout,
        .newLayout = new_layout,
        // Ignored cuz same queues
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // Releases ownership
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // Gains ownership
        .image = swapChainImages[imageIndex],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor, // Tells about the data stored
            .baseMipLevel = 0, // Mipmap ahh things
            .levelCount = 1,
            .baseArrayLayer = 0, // 3D ahh things
            .layerCount = 1,
        }
    };

    vk::DependencyInfo dependency_info
    {
        .dependencyFlags = {}, // How dependencies are evaluated for diff parts (like diff gpus, diff images in VR or diff tiles in mobile GPUs)
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    commandBuffers[frameIndex].pipelineBarrier2( dependency_info );
}

void MakeTriangle::recordCommandBuffer(uint32_t imageIndex)
{
    auto& commandBuffer = commandBuffers[frameIndex];
    commandBuffer.begin({});

    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f ,0.0f ,0.0f ,1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear, // Clears the background to a color instead of loading last image to draw on
        .storeOp = vk::AttachmentStoreOp::eStore, // Stores the image for rendering to screen
        .clearValue = clearColor,
    };

    // This thing can render stuff in between the viewport/scissor rectangle which is helpful for widgets and stuff 
    vk::RenderingInfo renderingInfo = {
        .renderArea = {
            .offset = { 0, 0 },
            .extent = swapChainExtent,
        }, // Where all the juicy rendering stuff happens that was described in attachmentInfo
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo,
    };

    commandBuffer.beginRendering( renderingInfo );
    commandBuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *graphicsPipeline );
    // Viewport and Scissor are dynamic
    commandBuffer.setViewport(
        0,
        vk::Viewport(
            0.0f,
            0.0f,
            static_cast<float>(swapChainExtent.width),
            static_cast<float>(swapChainExtent.height),
            0.0f,
            1.0f
        )
    );
    commandBuffer.setScissor( 0, vk::Rect2D( vk::Offset2D( 0, 0 ), swapChainExtent ) );

    commandBuffer.draw( 3, 1, 1, 0 ); // vertexCount, instanceCount, firstVertex, firstInstance (default values for instancing as we're not doing that)
    commandBuffer.endRendering();

    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {}, // Gives the data from VRAM to the display engine
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    commandBuffer.end();
}

void MakeTriangle::createSyncObjects()
{
    assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty()); // If function accidently gets called twice while these aren't empty

    // All memory buffers will be considered for presenting
    for (int i{0}; i < swapChainImages.size(); ++i) // All the memory buffers allocated (like 2 for rendering, other for storing images that got rendered, one for presenting)
    {
        renderFinishedSemaphores.emplace_back( device, vk::SemaphoreCreateInfo() ); // Rendering is done and can be read by the display engine
    }

    // Only (MAX_FRAMES_IN_FLIGHT) will be considered for rendering
    for (int i{0}; i < constants::MAX_FRAMES_IN_FLIGHT; ++i) // The memory buffers in which an image is getting rendered into
    {
        presentCompleteSemaphores.emplace_back( device, vk::SemaphoreCreateInfo() ); // Presentation by display engine is complete and it can be overwritten in the VRAM
        inFlightFences.emplace_back( device, vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled } ); // Sends the signal to the CPU if an image for presented
    }
}

void MakeTriangle::drawFrame()
{
    auto fenceRusult = device.waitForFences( *inFlightFences[frameIndex], vk::True, UINT64_MAX ); // vk::Fence, waits for all the fences, max time before CPU gets unblocked (kinda infinity here)
    if (fenceRusult != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to wait for the fence!");
    }
    device.resetFences(*inFlightFences[frameIndex]); // Unsignals the fence

    // Acquires the block of memory in VRAM which the display engine just presented
    auto [result, imageIndex] = swapChain.acquireNextImage( UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr ); // timeout, presentation complete, fence

    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
    const vk::SubmitInfo submitInfo
    {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex], // Wait for this to complete
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphores[frameIndex], // Signal this when completed
    };

    graphicsQueue.submit( submitInfo, *inFlightFences[frameIndex] );

    const vk::PresentInfoKHR presentInfoKHR
    {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[frameIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain, // Can be multiple for diff monitors/images
        .pImageIndices = &imageIndex, // Can be multiple for diff monitors/images
        .pResults = nullptr, // If you have multiple swapchains for multiple monitors and need an array of results for it
    };

    result = graphicsQueue.presentKHR(presentInfoKHR);

    frameIndex = (frameIndex + 1) % constants::MAX_FRAMES_IN_FLIGHT;
}

void MakeTriangle::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void MakeTriangle::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }

    device.waitIdle();
}

void MakeTriangle::cleanup() const
{
    
}

void MakeTriangle::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}