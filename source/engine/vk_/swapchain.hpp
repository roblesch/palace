#ifndef PALACE_ENGINE_VK_SWAPCHAIN_HPP
#define PALACE_ENGINE_VK_SWAPCHAIN_HPP

#include "include.hpp"

namespace vk_ {

class Swapchain {
private:
    SDL_Window* m_window;
    vk::Device* m_device;

    vk::UniqueSwapchainKHR m_uniqueSwapchain;
    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_uniqueImageViews;
    std::vector<vk::UniqueFramebuffer> m_uniqueFramebuffers;

public:
    Swapchain() = default;
    Swapchain(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::Device& device, vk::RenderPass& renderPass);

    std::vector<vk::UniqueFramebuffer>& getUniqueFramebuffers();
    vk::SwapchainKHR& getSwapchain();
};

} // namespace vk_

#endif
