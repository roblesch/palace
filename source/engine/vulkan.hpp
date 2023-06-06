#ifndef PALACE_ENGINE_VULKAN_HPP
#define PALACE_ENGINE_VULKAN_HPP

#include <string>

#include "vk_/buffer.hpp"
#include "vk_/debug.hpp"
#include "vk_/device.hpp"
#include "vk_/include.hpp"
#include "vk_/pipeline.hpp"
#include "vk_/primitive.hpp"
#include "vk_/swapchain.hpp"

namespace engine {

class Vulkan {
private:
    static constexpr int s_windowWidth = 800;
    static constexpr int s_windowHeight = 600;
    static constexpr const std::string_view s_spirVDir = "/shaders";
    static constexpr uint32_t s_concurrentFrames = 2;

    SDL_Window* m_window;
    vk::DynamicLoader m_dynamicLoader;
    vk::UniqueInstance m_uniqueInstance;
    vk::UniqueSurfaceKHR m_uniqueSurface;

    vk_::Device m_device;
    vk_::Pipeline m_pipeline;
    vk_::Swapchain m_swapchain;
    vk_::Buffer m_buffer;

    bool m_isValidationEnabled = true;
    bool m_isVerticesBound = false;
    bool m_isResized = false;
    bool m_isInitialized = false;

    vk::Extent2D m_extent2D;
    size_t m_currentFrame = 0;
    size_t m_vertexCount = 0;

public:
    explicit Vulkan(bool enableValidation = true);
    ~Vulkan();

    void bindVertexBuffer(std::vector<vk_::Vertex>& vertices);
    void recreateSwapchain();
    void recordCommandBuffer(vk::CommandBuffer& commandBuffer, uint32_t imageIndex);
    void drawFrame();
    void run();
};

} // namespace engine

#endif
