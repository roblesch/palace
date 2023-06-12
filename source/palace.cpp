#include <string>
#include "graphics/vk_/scene.hpp"
#include "graphics/vulkan.hpp"
#include "util/parser.hpp"

int main(const int argc, const char* argv[])
{
    Parser args(argc, argv);

    vk_::Scene scene = vk_::Scene::fromObj(args.obj_path());
    vk_::Scene scene2 = vk_::Scene::fromGltf(args.gltf_path());

#ifdef NDEBUG
    gfx::Vulkan vulkan(false);
#else
    gfx::Vulkan vulkan;
#endif

    vulkan.bindVertexBuffer(scene2.meshes[0].vertices, scene2.meshes[0].indices);
    vulkan.loadTextureImage(args.texture_path());

    vulkan.run();
    return 0;
}
