#ifndef PALACE_VK_HPP
#define PALACE_VK_HPP

#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

namespace pl {

class Vulkan {
public:
    void init(SDL_Window* window);
    void cleanup();

private:
    SDL_Window* window;

#ifdef NDEBUG
    bool enableValidation = false;
#else
    bool enableValidation = true;
#endif

    vk::DynamicLoader dl;
    vk::UniqueInstance instance;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

}

#endif
