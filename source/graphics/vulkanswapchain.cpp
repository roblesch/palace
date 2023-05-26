#include "vulkanswapchain.hpp"

namespace graphics::vk_ {

SwapChain::SwapChain(vk::SurfaceKHR surface, vk::PhysicalDevice physicalDevice)
{
    // choose swap surface format
    vk::SurfaceFormatKHR surfaceFormat;
    for (const auto& format : physicalDevice.getSurfaceFormatsKHR(surface)) {
        if (format.format == vk::Format::eB8G8R8A8Srgb)
            surfaceFormat = format;
    }

    // choose present mode
    vk::PresentModeKHR presentMode;
    for (const auto& mode : physicalDevice.getSurfacePresentModesKHR(surface)) {
        if (mode == vk::PresentModeKHR::eMailbox)
            presentMode = mode;
    }
}

}
