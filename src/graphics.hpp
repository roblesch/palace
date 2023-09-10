#pragma once

#include "camera.hpp"
#include "types.hpp"
#include "vk_mem_alloc.h"

struct VmaBuffer {
    size_t size;
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct VmaImage {
    VkImage image;
    VmaAllocation allocation;
    uint32_t mipLevels;
};

class VulkanContext {
    static VulkanContext* singleton;

    SDL_Window* window;
    vk::UniqueInstance instance;
    vk::UniqueSurfaceKHR surface;
    vk::Extent2D extent;
    vk::PhysicalDevice physicalDevice;
    uint32_t graphicsQueueIndex;
    vk::UniqueDevice device;
    vk::Queue graphicsQueue;
    vk::UniqueCommandPool commandPool;
    vk::Format colorFormat = vk::Format::eB8G8R8A8Unorm;
    vk::Format depthFormat = vk::Format::eD32Sfloat;
    vk::UniqueSwapchainKHR swapchain;
    bool resized = false;

    /*TODO
     */

    vk::SampleCountFlagBits msaaSampleCount = vk::SampleCountFlagBits::e4;

    /*
        Descriptors
    */
    struct DescriptorPools {
        vk::UniqueDescriptorPool ubo;
        vk::UniqueDescriptorPool material;
    } descriptorPools;

    struct DescriptorSetLayouts {
        vk::UniqueDescriptorSetLayout ubo;
        vk::UniqueDescriptorSetLayout material;
    } descriptorLayouts;

    /*
        Vma
    */
    struct Vma {
        VmaAllocator allocator;
        std::vector<VmaBuffer*> buffers;
        std::vector<VmaImage*> images;
    } vma;

    /*
        Off-screen resources
    */
    struct OffScreenResources {
        uint32_t shadowResolution = 2048;
        vk::UniqueFramebuffer frameBuffer;
        VmaImage* depthImage;
        VmaBuffer* buffer;
        vk::UniqueImageView depthView;
        vk::UniqueSampler depthSampler;
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
    } offScreen;

    /*
        On-screen resources
    */
    struct OnScreenResources {
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
        VmaImage* depthImage;
        vk::UniqueImageView depthView;
        VmaImage colorImage;
        vk::UniqueImageView colorImageView;
    } onScreen;

    /*
        Per-frame resources
    */
    struct FrameResources {
        vk::UniqueCommandBuffer commandBuffer;
        struct {
            VmaBuffer* buffer;
            vk::UniqueDescriptorSet descriptorSet;
        } uniformBuffer;
        struct {
            vk::Image image;
            vk::UniqueImageView imageView;
            vk::UniqueFramebuffer frameBuffer;
        } swapchainResources;
        vk::UniqueFence inFlight;
        vk::UniqueSemaphore renderFinished;
        vk::UniqueSemaphore imageAvailable;
    };

    std::vector<FrameResources> frames;
    uint32_t frame = 0;

    struct UniformBuffer {
        glm::mat4 cameraView { 1.0f };
        glm::mat4 cameraProj { 1.0f };
        glm::mat4 lightView { 1.0f };
        glm::mat4 lightProj { 1.0f };
        glm::vec4 lightPos { -50.0f, 50.0f, 50.0f, 1.0f };
    } ubo;

    /*
        Push constants
    */
    struct PushConstants {
        glm::mat4 meshTransform { 1.0f };
        float useNormalTexture { 0.0f };
    } pushConstants;

    /*
        Camera
    */
    Camera camera;

public:
    static void init();
    static void cleanup();
    static VulkanContext* get();

    VulkanContext();
    ~VulkanContext();

private:
    void createWindow();
    void createInstance();
    void createSurface();
    void createDevice();
    void createCommandPool();
    void createDescriptorLayouts();
    void createVmaAllocator();
    void createSwapchain(vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    void createOffScreenResources();
    void createOnScreenResources();
    void createFrameResources();

    // void createOnScreenResources();
    // void createOffScreenResources();
    // void createFrameResources();

    vk::UniqueCommandBuffer beginOneTimeCommandBuffer();
    void endOneTimeCommandBuffer(vk::CommandBuffer& commandBuffer);

    VmaBuffer* createBuffer(size_t size, vk::BufferUsageFlags usage, VmaAllocationCreateFlags flags);
    void uploadToBuffer(VmaBuffer* buffer, void* src);
    void uploadToBufferDirect(VmaBuffer* buffer, void* src);
    VmaImage* createImage(vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage, uint32_t mipLevels, vk::SampleCountFlagBits samples);
    VmaImage* createTextureImage(const void* src, size_t size, vk::Extent3D extent, uint32_t mipLevels);
    vk::UniqueImageView createImageViewUnique(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectMask, uint32_t);
    vk::UniqueSampler createTextureSamplerUnique(uint32_t mipLevels);
    VmaBuffer* createStagingBuffer(size_t size);
    vk::ImageMemoryBarrier imageTransitionBarrier(vk::Image image, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels = 1);
};

namespace gfx {

void init();

void cleanup();

VulkanContext* get();

}
