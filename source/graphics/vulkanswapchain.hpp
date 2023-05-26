#ifndef PALACE_GRAPHICS_VULKANSWAPCHAIN_HPP
#define PALACE_GRAPHICS_VULKANSWAPCHAIN_HPP

#include "vulkaninclude.hpp"

namespace graphics::vk_ {

class SwapChain {
private:
    struct {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    } swapChainSupportDetails;

public:
    SwapChain() = default;
    SwapChain(vk::SurfaceKHR surface, vk::PhysicalDevice physicalDevice);

};

}

#endif
