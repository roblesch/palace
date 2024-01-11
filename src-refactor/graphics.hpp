#pragma once

#include "types.hpp"
#include "vktypes.hpp"
#include <array>
#include <vector>

class GraphicsContext {
    Application application;
    Window window;
    DebugMessenger debug;
    Instance instance;
    Surface surface;
    PhysicalDevice physicalDevice;
    GraphicsQueue graphicsQueue;
    Device device;

    Swapchain swapchain;
    Viewport viewport;
    ShaderModule triangleVert;
    ShaderModule triangleFrag;
    PipelineLayout pipelineLayout;
    Pipeline pipeline;

    CommandPool commandPool;
    CommandBuffer commandBuffer;
    Fence renderFence;
    Semaphore renderSemaphore;
    Semaphore presentSemaphore;

    uint32_t frame = 0;

public:
    GraphicsContext();
    ~GraphicsContext();

    void create();
    void destroy();

    void draw();
};

class Graphics {
    static GraphicsContext* graphics_context_;

public:
    static void bind(GraphicsContext* graphics_context);
    static void unbind();
    static bool ready();
};
