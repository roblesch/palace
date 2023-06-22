#include <string>

#include "util/parser.hpp"
#include "vk/gltf.hpp"
#include "vk/vulkan.hpp"

using namespace pl;

int main(const int argc, const char* argv[])
{
    Parser args(argc, argv);
    GltfScene scene = loadGltfScene(args.gltf_path());

#ifdef NDEBUG
    Vulkan vulkan(false);
#else
    Vulkan vulkan;
#endif

    auto primitive = scene.meshes[0].primitives[0];
    auto texture = primitive.texture;

    vulkan.bindVertexBuffer(primitive.vertices, primitive.indices);
    vulkan.loadTextureImage(texture->data.data(), texture->width, texture->height);
    vulkan.run();
}
