#ifndef PALACE_ENGINE_VK_PIPELINE_HPP
#define PALACE_ENGINE_VK_PIPELINE_HPP

#include "include.hpp"

namespace vk_ {

class Pipeline {
private:
    vk::UniqueDescriptorSetLayout m_uniqueDescriptorSetLayout;
    vk::UniqueRenderPass m_uniqueRenderPass;
    vk::UniquePipelineLayout m_uniquePipelineLayout;
    vk::UniquePipeline m_uniquePipeline;

    std::vector<char> readSpirVFile(const std::string& spirVFile);

public:
    Pipeline() = default;
    Pipeline(vk::Device& device, vk::Extent2D& extent2D, uint32_t concurrentFrames);

    vk::RenderPass& renderPass();
    vk::Pipeline& pipeline();
    vk::PipelineLayout& pipelineLayout();
    vk::DescriptorSetLayout& descriptorSetLayout();
};

} // namespace vk_

#endif
