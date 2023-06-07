#include "engine/vk_/primitive.hpp"
#include "engine/vulkan.hpp"

std::vector<vk_::Vertex> vertices = {
    { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
    { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
};

std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

int main(const int argc, const char* argv[])
{
#ifdef NDEBUG
    engine::Vulkan vulkan(false);
#else
    engine::Vulkan vulkan;
#endif
    vulkan.loadTextureImage("textures/texture.jpg");
    vulkan.bindVertexBuffer(vertices, indices);
    vulkan.run();

    return 0;
}
