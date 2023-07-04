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
    void createPipelines();
    void createSwapchain(vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void recreateSwapchain();
    void updateUniformBuffers(int dx);
    void drawNode(vk::CommandBuffer& commandBuffer, pl::Node* node);
    void drawFrame();

    static constexpr int sWidth_ = 1600;
    static constexpr int sHeight_ = 900;
    static constexpr const std::string_view sSpirVDir_ = "/shaders";
    static constexpr uint32_t sConcurrentFrames_ = 2;
    static constexpr vk::Format sSwapchainFormat_ = vk::Format::eB8G8R8A8Unorm;
    static constexpr vk::SampleCountFlagBits sMsaaSamples_ = vk::SampleCountFlagBits::e4;

    bool isValidationEnabled_ = true;
    bool isInitialized_ = false;
    bool isSceneLoaded_ = false;
    bool isResized_ = false;

    vk::Extent3D extent_;
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
    } queueFamilyIndices_ {};
    vk::PhysicalDevice physicalDevice_;
    vk::UniqueDevice device_;

    // command buffers
    vk::Queue graphicsQueue_;
    vk::UniqueCommandPool commandPool_;
    std::vector<vk::UniqueCommandBuffer> commandBuffers_;

    // memory
    pl::UniqueMemoryHelper memoryHelper_;

    // descriptors
    struct DescriptorSetLayouts {
        vk::UniqueDescriptorSetLayout ubo;
        vk::UniqueDescriptorSetLayout material;
    } descriptorLayouts_;
    vk::UniqueDescriptorPool descriptorPool_;

    // renderpass
    vk::UniqueRenderPass renderPass_;

    // pipelines
    pl::UniquePipelineHelper pipelineHelper_;
    struct {
        vk::UniquePipelineLayout layout;
        vk::UniquePipeline pipeline;
    } colorPipeline_, texturePipeline_;

    // uniforms
    std::vector<VmaBuffer*> uniformBuffers_;
    std::vector<vk::UniqueDescriptorSet> uboDescriptorSets_;

    // ubo
    struct UniformBuffer {
        glm::mat4 model { 1.0f };
        glm::mat4 view { 1.0f };
        glm::mat4 proj { 1.0f };
    } ubo_;

    // push constants
    struct PushConstants {
        glm::mat4 meshTransform;
        float useNormalTexture;
    } pushConstants_;

    // swapchain
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchainImages_;
    std::vector<vk::UniqueImageView> swapchainImageViews_;
    std::vector<vk::UniqueFramebuffer> swapchainFramebuffers_;
    pl::VmaImage* depthImage_;
    vk::UniqueImageView depthView_;

    // multisampling
    pl::VmaImage* colorImage_;
    vk::UniqueImageView colorImageView_;

    // sync
    std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;
    std::vector<vk::UniqueFence> inFlightFences_;

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