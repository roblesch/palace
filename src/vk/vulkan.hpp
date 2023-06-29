#pragma once

#include "camera.hpp"
#include "gltf.hpp"
#include "memory.hpp"
#include "pipelines.hpp"
#include "types.hpp"
#include <functional>
#include <string>

namespace pl {

class Vulkan {
public:
    explicit Vulkan(bool enableValidation = true);
    ~Vulkan();

    void loadGltfModel(const char* path);
    void run();

private:
    void createSwapchain(vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void recreateSwapchain();

    void updateUniformBuffers(int dx);
    void drawNode(vk::CommandBuffer& commandBuffer, pl::Node* node);
    void drawFrame();

    static constexpr int sWidth_ = 1800;
    static constexpr int sHeight_ = 600;
    static constexpr const std::string_view sSpirVDir_ = "/shaders";
    static constexpr uint32_t sConcurrentFrames_ = 2;

    bool isValidationEnabled_ = true;
    bool isInitialized_ = false;
    bool isSceneLoaded_ = false;
    bool isResized_ = false;

    vk::Extent2D extent_;
    size_t currentFrame_ = 0;
    size_t indicesCount_ = 0;

    // instance
    SDL_Window* window_;
    vk::DynamicLoader dynamicLoader_;
    vk::UniqueInstance instance_;
    vk::UniqueSurfaceKHR surface_;

    // device
    struct
    {
        uint32_t graphics;
    } queueFamilyIndices_{};
    vk::PhysicalDevice physicalDevice_;
    vk::UniqueDevice device_;
    vk::Queue graphicsQueue_;
    vk::UniqueCommandPool commandPool_;
    std::vector<vk::UniqueCommandBuffer> commandBuffers_;

    // pipelines
    vk::UniqueRenderPass renderPass_;
    pl::UniquePipelineHelper pipelineHelper_;
    pl::UniqueHelperPipeline colorPipeline_;
    pl::UniqueHelperPipeline texturePipeline_;
    std::vector<vk::UniqueDescriptorSet> colorPipelineDescriptorSets_;

    // memory
    pl::UniqueMemoryHelper memoryHelper_;

    // swapchain
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchainImages_;
    std::vector<vk::UniqueImageView> swapchainImageViews_;
    std::vector<vk::UniqueFramebuffer> swapchainFramebuffers_;
    pl::VmaImage* depthImage_;
    vk::UniqueImageView depthView_;

    // sync
    std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;
    std::vector<vk::UniqueFence> inFlightFences_;

    // buffers
    std::vector<VmaBuffer*> uniformBuffers_;

    // ubo
    struct UniformBuffer {
        glm::mat4 model { 1.0f };
        glm::mat4 view { 1.0f };
        glm::mat4 proj { 1.0f };
    } ubo_;

    // scene
    pl::UniqueGltfModel model_;

    // imgui
    vk::UniqueDescriptorPool imguiDescriptorPool_;

    // camera
    Camera camera_;

    // time
    Uint64 ticks { 0 };
    Uint64 dt { 0 };
};

}