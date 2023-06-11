#include <string>
#include "graphics/vk_/scene.hpp"
#include "graphics/vulkan.hpp"

const char* model_path = "models/viking_room.obj";
const char* gltf_path = "models/Avocado.glb";
const char* texture_path = "models/textures/viking_room.png";

int main(const int argc, const char* argv[])
{
    vk_::Scene scene = vk_::Scene::fromObj(model_path);
    vk_::Scene scene2 = vk_::Scene::fromGltf(gltf_path);

#ifdef NDEBUG
    gfx::Vulkan vulkan(false);
#else
    gfx::Vulkan vulkan;
#endif

    vulkan.bindVertexBuffer(scene.vertices, scene.indices);
    vulkan.loadTextureImage(texture_path);
    vulkan.run();

    return 0;
}
