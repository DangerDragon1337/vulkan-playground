#pragma once

#include <cstdint>
#include <vector>

namespace constants
{
    inline constexpr uint32_t SCREEN_WIDTH{800};
    inline constexpr uint32_t SCREEN_HEIGHT{600};
    inline constexpr int MAX_FRAMES_IN_FLIGHT{2};
}

namespace layers
{
    inline const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    #ifdef NDEBUG
    inline constexpr bool enableValidationLayers{false};
    #else
    inline constexpr bool enableValidationLayers{true};
    #endif
}