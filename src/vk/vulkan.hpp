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
    void updateUniformBuffers(int dx);
    void drawNode(vk::CommandBuffer& commandBuffer, pl::Node* node);
    void drawFrame();

    vk::UniqueBuffer createBufferUnique(vk::DeviceSize& size, const vk::BufferUsageFlags usage);
    vk::UniqueDeviceMemory createDeviceMemoryUnique(const vk::MemoryRequirements requirements, const vk::MemoryPropertyFlags memoryFlags);
    vk::UniqueImage createImageUnique(vk::Extent2D& extent, const vk::Format format, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage);
    vk::UniqueImageView createImageViewUnique(vk::Image& image, const vk::Format format, vk::ImageAspectFlagBits aspectMask);

    void createSwapchain(vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void recreateSwapchain();

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
    } queueFamilyIndices_;
    vk::PhysicalDevice physicalDevice_;
    vk::UniqueDevice device_;
    vk::Queue graphicsQueue_;
    vk::UniqueCommandPool commandPool_;
    std::vector<vk::UniqueCommandBuffer> commandBuffers_;

    // sync
    std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;
    std::vector<vk::UniqueFence> inFlightFences_;

    // pipelines
    pl::UniquePipelineHelper pipelineHelper_;
    vk::UniqueDescriptorSetLayout pipelineDescriptorLayout_;
    vk::UniqueDescriptorPool pipelineDescriptorPool_;
    std::vector<vk::UniqueDescriptorSet> pipelineDescriptorSets_;
    vk::UniqueRenderPass renderPass_;
    vk::UniquePipelineLayout pipelineLayout_;
    vk::UniquePipeline vertColorPipeline_;
    vk::UniquePipeline texturePipeline_;

    // swapchain
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchainImages_;
    std::vector<vk::UniqueImageView> swapchainImageViews_;
    std::vector<vk::UniqueFramebuffer> swapchainFramebuffers_;
    vk::UniqueImage depthImage_;
    vk::UniqueDeviceMemory depthMemory_;
    vk::UniqueImageView depthView_;

    // buffers
    std::vector<vk::UniqueBuffer> uniformBuffers_;
    std::vector<vk::UniqueDeviceMemory> uniformMemories_;
    std::vector<void*> uniformPtrs_;

    // ubo
    struct UniformBuffer {
        glm::mat4 model { 1.0f };
        glm::mat4 view { 1.0f };
        glm::mat4 proj { 1.0f };
    } ubo_;

    // memory
    pl::UniqueMemoryHelper memoryHelper_;

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

} // namespace pl
