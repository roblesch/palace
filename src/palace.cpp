#include "gltf/scene.hpp"
#include "util/parser.hpp"
#include "vk/vulkan.hpp"
#include <string>

using namespace pl;

int main(const int argc, const char* argv[])
{
    Parser args(argc, argv);
    Scene scene = Scene::fromGltf(args.gltf_path());

#ifdef NDEBUG
    Vulkan vulkan(false);
#else
    Vulkan vulkan;
#endif

    vulkan.bindVertexBuffer(scene.meshes[0].vertices, scene.meshes[0].indices);
    vulkan.loadTextureImage(args.texture_path());

    vulkan.run();
    return 0;
}
