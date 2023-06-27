#include "util/parser.hpp"
#include "vk/vulkan.hpp"

using namespace pl;

int main(const int argc, const char* argv[])
{
    Parser args(argc, argv);

#ifdef NDEBUG
    Vulkan vulkan(false);
#else
    Vulkan vulkan;
#endif

    vulkan.loadGltfModel(args.gltf_path());
    vulkan.run();

    return 0;
}
