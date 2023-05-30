#ifndef PALACE_ENGINE_VK_PIPELINE_HPP
#define PALACE_ENGINE_VK_PIPELINE_HPP

#include <string>
#include "include.hpp"

namespace vk_ {

class Pipeline {
private:
    const vk::Device* m_device;

    //vk::UniqueShaderModule m_vertexShaderModule;
    //vk::UniqueShaderModule m_fragmentShaderModule;

    vk::UniqueShaderModule loadShaderModule(const std::string& spirVFile);

public:
    Pipeline() = default;
    Pipeline(const vk::Device& device, const std::string& spirVDir);
};

} // namespace vk_

#endif
