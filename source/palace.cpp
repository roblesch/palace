#include "graphics/vulkan.hpp"

int main(const int argc, const char *argv[])
{
    graphics::Vulkan vulkan(true);
    vulkan.run();

    return 0;
}
