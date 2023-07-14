#pragma once

#include "camera.hpp"
#include "gltf.hpp"
#include "memory.hpp"
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
    void createShadowPassResources();
    void createPipelines();
    void createSwapchain(vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void recreateSwapchain();
    void updateUniformBuffers(int dx);
    void drawNode(vk::CommandBuffer& commandBuffer, pl::Node* node);
    void drawNodeShadow(vk::CommandBuffer& commandBuffer, pl::Node* node);
    void drawFrame();

    static constexpr int sWidth_ = 1600;
    static constexpr int sHeight_ = 900;
    static constexpr int sShadowResolution_ = 2048;
    static constexpr uint32_t sConcurrentFrames_ = 2;
    static constexpr vk::Format sSwapchainFormat_ = vk::Format::eB8G8R8A8Unorm;
    static constexpr vk::Format sDepthAttachmentFormat_ = vk::Format::eD32Sfloat;
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
    vk::UniqueDescriptorPool descriptorPool_;
    struct DescriptorSetLayouts {
        vk::UniqueDescriptorSetLayout ubo;
        vk::UniqueDescriptorSetLayout material;
    } descriptorLayouts_;

    // shadow pass resources
    struct ShadowPassResources {
        uint32_t width {}, height {};
        vk::UniqueFramebuffer frameBuffer;
        pl::VmaImage* depthImage {};
        pl::VmaBuffer* buffer;
        vk::UniqueImageView depthView;
        vk::UniqueSampler depthSampler;
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
    } shadowPass_;

    // renderpass
    vk::UniqueRenderPass renderPass_;

    // pipelines
    struct {
        vk::UniquePipelineLayout layout;
        vk::UniquePipeline pipeline;
    } texturePipeline_;

    // uniforms
    struct CameraUniformBuffer {
        VmaBuffer* buffer;
        vk::UniqueDescriptorSet descriptorSet;
    };
    std::vector<CameraUniformBuffer> uniformBuffers_;

    // ubos
    struct UniformBuffer {
        glm::mat4 cameraView { 1.0f };
        glm::mat4 cameraProj { 1.0f };
        glm::mat4 lightView { 1.0f };
        glm::mat4 lightProj { 1.0f };
        glm::vec3 lightPos { 0.0f };
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