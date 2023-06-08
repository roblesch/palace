#ifndef PALACE_ENGINE_VK_SWAPCHAIN_HPP
#define PALACE_ENGINE_VK_SWAPCHAIN_HPP

#include "include.hpp"

namespace vk_ {

class Swapchain {
private:
    vk::UniqueSwapchainKHR m_uniqueSwapchain;
    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_uniqueImageViews;
    std::vector<vk::UniqueFramebuffer> m_uniqueFramebuffers;

    void create(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass, vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);

public:
    static vk::UniqueImageView createImageViewUnique(vk::Device& device, vk::Image& image, const vk::Format format);

    Swapchain() = default;
    Swapchain(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass);

    vk::Framebuffer& framebuffer(size_t frame);
    vk::ImageView& imageView(size_t frame);
    vk::SwapchainKHR& swapchain();

    void recreate(SDL_Window* window, vk::SurfaceKHR& surface, vk::Extent2D& extent2D, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::RenderPass& renderPass);
};

} // namespace vk_

#endif
