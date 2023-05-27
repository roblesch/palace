#ifndef PALACE_GRAPHICS_VULKANDEBUG_HPP
#define PALACE_GRAPHICS_VULKANDEBUG_HPP

#include "include.hpp"

namespace vk_::debug {

vk::DebugUtilsMessengerCreateInfoEXT createInfo();

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

}

#endif
