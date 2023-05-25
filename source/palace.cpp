#include "graphics/vulkan.hpp"

int WIDTH = 800;
int HEIGHT = 600;

int main()
{
    graphics::Vulkan vulkan(true);
    vulkan.run();

    return 0;
}
