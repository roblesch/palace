#include "util/parser.hpp"
#include "vk/vulkan.hpp"

/* === TODO ===
* descriptor init and organization
* pipeline init and organization
* vma mapping pointers each frame
* decouple input handling 
* process input on sep thread */

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
