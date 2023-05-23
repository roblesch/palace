#ifndef PALACE_VK_HPP
#define PALACE_VK_HPP

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

extern SDL_Window* window;

extern vk::Instance instance;

namespace palace::vulkan {
    namespace {
        void createInstance() {
            vk::ApplicationInfo applicationInfo{ .pApplicationName = "Hello Triangle",
                                                 .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                                 .pEngineName = "None",
                                                 .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                                 .apiVersion = VK_API_VERSION_1_0 };

            vk::InstanceCreateInfo instanceInfo{ .pApplicationInfo = &applicationInfo };

            uint32_t extensionCount;
            SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
            const char** extensionNames = 0;
            extensionNames = new const char* [extensionCount];
            SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames);

            instance = vk::createInstance(instanceInfo, nullptr);
        }
    }

    void init() {
        createInstance();
    }
}

#endif
