#include "engine/vk_/primitive.hpp"
#include "engine/vulkan.hpp"

std::vector<vk_::Vertex> vertices = {
    { { 0.0f, -0.5f }, { 1.0f, 1.0f, 1.0f } },
    { { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
    { { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } }
};

int main(const int argc, const char* argv[])
{
#ifdef NDEBUG
    engine::Vulkan vulkan(false);
#else
    engine::Vulkan vulkan;
#endif
    vulkan.bindVertexBuffer(vertices);
    vulkan.run();

    return 0;
}
