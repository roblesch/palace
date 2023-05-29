#include "engine/vulkan.hpp"

int main(const int argc, const char* argv[])
{
    engine::Vulkan vulkan(true);
    vulkan.run();

    return 0;
}
