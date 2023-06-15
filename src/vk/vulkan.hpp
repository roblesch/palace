#pragma once

#include "gltf/mesh.hpp"
#include "types.hpp"
#include <string>

namespace pl {

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class Vulkan {
public:
    static constexpr int sWidth_ = 800;
    static constexpr int sHeight_ = 600;
    static constexpr const std::string_view sSpirVDir_ = "/shaders";
    static constexpr uint32_t sConcurrentFrames_ = 2;

    explicit Vulkan(bool enableValidation = true);
    ~Vulkan();

    // void loadMesh(Mesh& mesh);
    void bindVertexBuffer(const std::vector<pl::Vertex>& vertices, const std::vector<uint32_t>& indices);
    void loadTextureImage(const unsigned char* data, uint32_t width, uint32_t height);
    void run();

private:
    void modelViewProj();
    void drawFrame();

    vk::UniqueCommandBuffer beginSingleUseCommandBuffer();
    void endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer);

    vk::UniqueBuffer createBufferUnique(vk::DeviceSize& size, const vk::BufferUsageFlags usage);
    vk::UniqueBuffer createStagingBufferUnique(vk::DeviceSize& size);
    vk::UniqueDeviceMemory createDeviceMemoryUnique(const vk::MemoryRequirements requirements, const vk::MemoryPropertyFlags memoryFlags);
    vk::UniqueDeviceMemory createStagingMemoryUnique(vk::Buffer& buffer, vk::DeviceSize& size);
    vk::UniqueImage createImageUnique(vk::Extent2D& extent, const vk::Format format, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage);
    vk::UniqueImageView createImageViewUnique(vk::Image& image, const vk::Format format, vk::ImageAspectFlagBits aspectMask);
    void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
    void transitionImageLayout(vk::Image& image, const vk::Format format, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout);

    void createSwapchain(vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void recreateSwapchain();

    bool isValidationEnabled_ = true;
    bool isInitialized_ = false;
    bool isTextureLoaded_ = false;
    bool isVerticesBound_ = false;
    bool isResized_ = false;

    vk::Extent2D extents_;
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

    // pipeline
    vk::UniqueDescriptorSetLayout pipelineDescriptorLayout_;
    vk::UniqueDescriptorPool pipelineDescriptorPool_;
    std::vector<vk::UniqueDescriptorSet> pipelineDescriptorSets_;
    vk::UniqueRenderPass renderPass_;
    vk::UniquePipelineLayout pipelineLayout_;
    vk::UniquePipeline pipeline_;

    // swapchain
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchainImages_;
    std::vector<vk::UniqueImageView> swapchainImageViews_;
    std::vector<vk::UniqueFramebuffer> swapchainFramebuffers_;
    vk::UniqueImage depthImage_;
    vk::UniqueDeviceMemory depthMemory_;
    vk::UniqueImageView depthView_;

    // buffers
    vk::UniqueBuffer vertexBuffer_;
    vk::UniqueBuffer indexBuffer_;
    vk::UniqueDeviceMemory vertexMemory_;
    vk::UniqueDeviceMemory indexMemory_;
    std::vector<vk::UniqueBuffer> uniformBuffers_;
    std::vector<vk::UniqueDeviceMemory> uniformMemories_;
    std::vector<void*> uniformPtrs_;

    // texture
    vk::UniqueImage texture_;
    vk::UniqueDeviceMemory textureMemory_;
    vk::UniqueImageView textureView_;
    vk::UniqueSampler textureSampler_;

    // imgui
    vk::UniqueDescriptorPool imguiDescriptorPool_;
};

} // namespace pl
