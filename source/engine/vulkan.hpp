#ifndef PALACE_ENGINE_VULKAN_HPP
#define PALACE_ENGINE_VULKAN_HPP

#include <string>

#include "vk_/debug.hpp"
#include "vk_/device.hpp"
#include "vk_/include.hpp"
#include "vk_/pipeline.hpp"
#include "vk_/swapchain.hpp"

namespace engine {

class Vulkan {
private:
    static constexpr int s_windowWidth = 800;
    static constexpr int s_windowHeight = 600;
    static constexpr const std::string_view s_spirVDir = "/shaders";
    static constexpr vk::Format s_imageFormat = vk::Format::eB8G8R8A8Srgb;

    bool m_isValidationEnabled;
    bool m_isInitialized {};

    SDL_Window* m_window;
    vk::DynamicLoader m_dynamicLoader;
    vk::UniqueInstance m_uniqueInstance;
    vk_::Device m_device;
    vk::UniqueSurfaceKHR m_uniqueSurface;
    vk_::Swapchain m_swapchain;
    vk_::Pipeline m_pipeline;

public:
    explicit Vulkan(bool enableValidation);
    ~Vulkan();

    void run() const;
};

} // namespace engine

#endif
