#include "util/parser.hpp"
#include "vk/vulkan.hpp"

/* === TODO ===
* descriptor init and organization
* vma mapping pointers each frame
* color image/swapchain image size mismatch
* position light at scene bounding box
* scale scene with bounding box
* move single use command buffer out of memory
* matrix transform hierarchy computed each frame
* less reliant on push constants
*/

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
