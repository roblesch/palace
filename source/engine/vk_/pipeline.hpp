#pragma once

#include "include.hpp"

namespace vk_ {

class Pipeline {
private:
    vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
    vk::UniqueDescriptorPool m_descriptorPool;
    std::vector<vk::UniqueDescriptorSet> m_descriptorSets;

    vk::UniqueRenderPass m_renderPass;
    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipeline m_pipeline;

    std::vector<char> readSpirVFile(const std::string& spirVFile);

public:
    Pipeline() = default;
    Pipeline(vk::Device& device, vk::Extent2D& extent2D, uint32_t concurrentFrames);

    vk::RenderPass& renderPass();
    vk::Pipeline& pipeline();
    vk::PipelineLayout& pipelineLayout();
    vk::DescriptorSetLayout& descriptorSetLayout();
    vk::DescriptorSet& descriptorSet(size_t frame);

    void setDescriptorSets(vk::Device& device, std::vector<vk::UniqueBuffer>& uniformBuffers, vk::Sampler& sampler, vk::ImageView& imageView, uint32_t concurrentFrames);
};

}
