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

struct Vertex {
    glm::vec3 pos { 0.0, 0.0, 0.0 };
    glm::vec3 normal { 0.0, 1.0, 0.0 };
    glm::vec4 color { 1.0, 1.0, 1.0, 1.0 };
    glm::vec2 uv { 0.0, 0.0 };

    static vk::VertexInputBindingDescription* bindingDescription()
    {
        return new vk::VertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions()
    {
        vk::VertexInputAttributeDescription posDescription {
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, pos)
        };

        vk::VertexInputAttributeDescription normalDescription {
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, normal)
        };

        vk::VertexInputAttributeDescription colorDescription {
            .location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32B32A32Sfloat,
            .offset = offsetof(Vertex, color)
        };

        vk::VertexInputAttributeDescription uvDescription {
            .location = 3,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, uv)
        };

        return { posDescription, normalDescription, colorDescription, uvDescription };
    }
};

class VulkanContext {
    static VulkanContext* singleton;

    SDL_Window* window;
    vk::UniqueInstance instance;
    vk::UniqueSurfaceKHR surface;
    vk::Extent2D windowExtent;
    vk::PhysicalDevice physicalDevice;
    uint32_t graphicsQueueIndex;
    vk::UniqueDevice device;
    vk::Queue graphicsQueue;
    vk::UniqueCommandPool commandPool;
    vk::Format colorFormat = vk::Format::eB8G8R8A8Unorm;
    vk::Format depthFormat = vk::Format::eD32Sfloat;
    vk::UniqueSwapchainKHR swapchain;
    bool resized = false;
    vk::SampleCountFlagBits msaaSampleCount = vk::SampleCountFlagBits::e4;
    uint32_t frameCount = 3;
    uint64_t frameTimeout = UINT64_MAX;
    bool initialized = false;

    /*
        Vma
    */
    struct Vma {
        VmaAllocator allocator;
        std::vector<VmaBuffer*> buffers;
        std::vector<VmaImage*> images;
    } vma;

    /*
        Descriptors
    */
    struct DescriptorPools {
        vk::UniqueDescriptorPool ubo;
        vk::UniqueDescriptorPool material;
        vk::UniqueDescriptorPool imGui;
    } descriptorPools;

    struct DescriptorSetLayouts {
        vk::UniqueDescriptorSetLayout ubo;
        vk::UniqueDescriptorSetLayout material;
    } descriptorLayouts;

    /*
        Off-screen resources
    */
    struct OffScreenRender {
        uint32_t shadowResolution = 2048;
        vk::UniqueFramebuffer framebuffer;
        VmaImage* depthImage;
        vk::UniqueImageView depthView;
        vk::UniqueSampler depthSampler;
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
    } offScreen;

    /*
        On-screen resources
    */
    struct OnScreenRender {
        VmaImage* depthImage;
        vk::UniqueImageView depthView;
        VmaImage* msaaImage;
        vk::UniqueImageView msaaView;
        vk::UniqueRenderPass renderPass;
        vk::UniquePipelineLayout pipelineLayout;
        vk::UniquePipeline pipeline;
    } onScreen;

    /*
        Per-frame resources
    */
    struct Frame {
        vk::UniqueCommandBuffer commandBuffer;
        VmaBuffer* uniformBuffer;
        vk::UniqueDescriptorSet uniformDescriptorSet;
        vk::UniqueFramebuffer framebuffer;
        vk::Image image;
        vk::UniqueImageView imageView;
        vk::UniqueFence inFlight;
        vk::UniqueSemaphore renderFinished;
        vk::UniqueSemaphore imageAvailable;
    };

    std::vector<Frame> frames;
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

public:
    static void init();
    static void cleanup();
    static bool ready();
    static VulkanContext* get();

    VulkanContext();
    ~VulkanContext();

    vk::Extent2D extents();
    SDL_Window* sdlWindow();
    void resize();
    void drawFrame();

private:
    void createContextResources();
    void createDescriptorLayouts();
    void createOffScreenResources();
    void createOnScreenResources();
    void createFrameResources();
    void createImGuiResources();

    static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    std::vector<char> readSpirVFile(const std::string& spirVFile);
    vk::UniqueCommandBuffer beginOneTimeCommandBuffer();
    void endOneTimeCommandBuffer(vk::CommandBuffer& commandBuffer);
    vk::UniqueSwapchainKHR createSwapchain(vk::Extent2D extent, vk::SwapchainKHR oldSwapchain);
    VmaBuffer* createBuffer(size_t size, vk::BufferUsageFlags usage, VmaAllocationCreateFlags flags);
    void uploadToBuffer(VmaBuffer* buffer, void* src);
    void uploadToBufferDirect(VmaBuffer* buffer, void* src);
    VmaImage* createImage(vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage, uint32_t mipLevels, vk::SampleCountFlagBits samples);
    VmaImage* createTextureImage(const void* src, size_t size, vk::Extent3D extent, uint32_t mipLevels);
    vk::UniqueImageView createImageViewUnique(vk::Image image, vk::Format format, vk::ImageAspectFlagBits aspectMask, uint32_t);
    vk::UniqueSampler createTextureSamplerUnique(uint32_t mipLevels);
    VmaBuffer* createStagingBuffer(size_t size);
    vk::ImageMemoryBarrier imageTransitionBarrier(vk::Image image, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels = 1);

    void recreateSwapchain();
};

namespace gfx {

VulkanContext* get();
void init();
void cleanup();
bool ready();
void render();

}
