#ifndef PALACE_ENGINE_VK_PIPELINE_HPP
#define PALACE_ENGINE_VK_PIPELINE_HPP

#include "include.hpp"

namespace vk_
{

	class Pipeline
	{
	private:
		vk::UniqueShaderModule m_vertex;
		vk::UniqueShaderModule m_fragment;

	public:
		Pipeline() = default;
	};

} // namespace vk_

#endif
