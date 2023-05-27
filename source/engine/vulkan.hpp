#ifndef PALACE_ENGINE_VULKAN_HPP
#define PALACE_ENGINE_VULKAN_HPP

#include "vk_/debug.hpp"
#include "vk_/device.hpp"
#include "vk_/include.hpp"
#include "vk_/pipeline.hpp"
#include "vk_/swapchain.hpp"

namespace engine
{

	class Vulkan
	{
	private:
		bool m_validation;
		bool m_initialized{};

		SDL_Window*			 m_window;
		vk::DynamicLoader	 m_dynamicLoader;
		vk::UniqueInstance	 m_uniqueInstance;
		vk_::Device			 m_device;
		vk::UniqueSurfaceKHR m_uniqueSurface;
		vk_::Swapchain		 m_swapchain;
		vk_::Pipeline		 m_pipeline;

		static constexpr int		 s_windowWidth = 800;
		static constexpr int		 s_windowHeight = 600;
		static constexpr const char* s_spirvDit = "/shaders";

	public:
		explicit Vulkan(bool enableValidation);
		~Vulkan();

		void run() const;
	};

} // namespace engine

#endif
