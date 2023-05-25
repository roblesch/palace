#ifndef PALACE_GRAPHICS_VULKANDEBUG_HPP
#define PALACE_GRAPHICS_VULKANDEBUG_HPP

#include "vulkaninclude.hpp"

namespace graphics::vulkan::debug {

vk::DebugUtilsMessengerCreateInfoEXT createInfo();

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

}

#endif
