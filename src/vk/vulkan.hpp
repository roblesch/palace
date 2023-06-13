#pragma once

#include "gltf/vertex.hpp"
#define VK_ENABLE_BETA_EXTENSIONS
#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_SETTERS
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <string>

namespace pl {

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class Vulkan {
private:
    static constexpr int s_windowWidth = 800;
    static constexpr int s_windowHeight = 600;
    static constexpr const std::string_view s_spirVDir = "/shaders";
    static constexpr uint32_t s_concurrentFrames = 2;

    SDL_Window* m_window;
    vk::DynamicLoader m_dynamicLoader;
    vk::UniqueInstance m_instance;
    vk::UniqueSurfaceKHR m_surface;

    // device
    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_device;

    struct {
        uint32_t graphics;
    } m_queueFamilyIndices;

    vk::Queue m_graphicsQueue;
    vk::UniqueCommandPool m_commandPool;
    std::vector<vk::UniqueCommandBuffer> m_commandBuffers;

    std::vector<vk::UniqueSemaphore> m_imageAvailableSemaphores;
    std::vector<vk::UniqueSemaphore> m_renderFinishedSemaphores;
    std::vector<vk::UniqueFence> m_inFlightFences;
    // device

    // pipeline
    vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
    vk::UniqueDescriptorPool m_descriptorPool;
    std::vector<vk::UniqueDescriptorSet> m_descriptorSets;

    vk::UniqueRenderPass m_renderPass;
    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipeline m_pipeline;
    // pipeline

    // swapchain
    vk::UniqueSwapchainKHR m_swapchain;

    std::vector<vk::Image> m_images;
    std::vector<vk::UniqueImageView> m_imageViews;
    std::vector<vk::UniqueFramebuffer> m_framebuffers;

    vk::UniqueImage m_depthImage;
    vk::UniqueDeviceMemory m_depthMemory;
    vk::UniqueImageView m_depthView;
    // swapchain

    // buffer
    vk::UniqueBuffer m_vertexBuffer;
    vk::UniqueBuffer m_indexBuffer;

    vk::UniqueDeviceMemory m_vertexMemory;
    vk::UniqueDeviceMemory m_indexMemory;

    std::vector<vk::UniqueBuffer> m_uniformBuffers;
    std::vector<vk::UniqueDeviceMemory> m_uniformMemorys;
    std::vector<void*> m_uniformPtrs;
    // buffer

    // texture
    vk::UniqueImage m_texture;
    vk::UniqueDeviceMemory m_textureMemory;
    vk::UniqueImageView m_textureView;
    vk::UniqueSampler m_textureSampler;
    // texture

    // imgui
    vk::UniqueDescriptorPool m_imguiDescriptorPool;
    // imgui

    bool m_isValidationEnabled = true;
    bool m_isInitialized = false;
    bool m_isTextureLoaded = false;
    bool m_isVerticesBound = false;
    bool m_isResized = false;

    vk::Extent2D m_extent2D;
    size_t m_currentFrame = 0;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;

    vk::UniqueImage createImageUnique(vk::Extent2D& extent, const vk::Format format, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage);
    vk::UniqueImageView createImageViewUnique(vk::Image& image, const vk::Format format, vk::ImageAspectFlagBits aspectMask);
    void transitionImageLayout(vk::Image& image, const vk::Format format, const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout);
    void createSwapchain(vk::SwapchainKHR oldSwapchain = VK_NULL_HANDLE);

    vk::UniqueCommandBuffer beginSingleUseCommandBuffer();
    void endSingleUseCommandBuffer(vk::CommandBuffer& commandBuffer);

    void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, const vk::MemoryPropertyFlags memPropertyFlags);
    vk::UniqueBuffer createBufferUnique(vk::DeviceSize& size, const vk::BufferUsageFlags usage);
    vk::UniqueBuffer createStagingBufferUnique(vk::DeviceSize& size);
    vk::UniqueDeviceMemory createDeviceMemoryUnique(const vk::MemoryRequirements requirements, const vk::MemoryPropertyFlags memoryFlags);
    vk::UniqueDeviceMemory createStagingMemoryUnique(vk::Buffer& buffer, vk::DeviceSize& size);

    void recreateSwapchain();
    void recordCommandBuffer(vk::CommandBuffer& commandBuffer, uint32_t imageIndex);
    void drawFrame();
    void modelViewProj();

public:
    explicit Vulkan(bool enableValidation = true);
    ~Vulkan();

    void loadTextureImage(const char* path);
    void bindVertexBuffer(const std::vector<pl::Vertex>& vertices, const std::vector<uint32_t>& indices);
    void run();
};

}
