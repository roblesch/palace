#pragma once

#include "types.hpp"
#include "vktypes.hpp"
#include <array>
#include <vector>

class GraphicsContext {
    Application vk_application_;
    Window sdl_window_;
    DebugMessenger vk_debug_messenger_;
    Instance vk_instance_;
    Surface vk_surface_khr_;
    PhysicalDevice vk_physical_device_;
    GraphicsQueue vk_graphics_queue_;
    Device vk_device_;
    Swapchain vk_swapchain_khr_;
    CommandPool vk_command_pool_;
    CommandBuffer vk_command_buffer_;

    struct {
        VkDescriptorPool descriptorPool;
    } imgui_;

public:
    GraphicsContext();
    ~GraphicsContext();

    void init();
    void cleanup();

private:
    void createWindow();
    VkResult createInstance();
    SDL_bool createSurface();
    VkResult createDevice();
    VkResult createSwapchain(VkSwapchainKHR oldSwapchain);
    VkResult createCommandPool();
    VkResult createCommandBuffer();
    void createImGui();
};

class Graphics {
    static GraphicsContext* graphics_context_;

public:
    static void bind(GraphicsContext* graphics_context);
    static void unbind();
    static bool ready();
};
