#ifndef PALACE_ENGINE_VK_DEBUG_HPP
#define PALACE_ENGINE_VK_DEBUG_HPP

#include "include.hpp"

namespace vk_::debug
{

	vk::DebugUtilsMessengerCreateInfoEXT createInfo();

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT													messageType,
		const VkDebugUtilsMessengerCallbackDataEXT*										pCallbackData,
		void*																			pUserData);

} // namespace vk_::debug

#endif
