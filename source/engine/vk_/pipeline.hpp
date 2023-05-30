#ifndef PALACE_ENGINE_VK_PIPELINE_HPP
#define PALACE_ENGINE_VK_PIPELINE_HPP

#include "include.hpp"

namespace vk_ {

class Pipeline {
private:
    const vk::Device* m_device;
    vk::Extent2D m_extent2D;
    vk::Format m_imageFormat;

    vk::UniqueRenderPass m_renderPass;
    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipeline m_pipeline;

    // vk::UniqueShaderModule m_vertexShaderModule;
    // vk::UniqueShaderModule m_fragmentShaderModule;

    std::vector<char> readSpirVFile(const std::string& spirVFile);

public:
    Pipeline() = default;
    Pipeline(const vk::Device* device, const std::string& spirVDir, const vk::Extent2D& extent2D, const vk::Format& imageFormat);
};

} // namespace vk_

#endif
