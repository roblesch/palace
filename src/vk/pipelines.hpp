#pragma once

#include "types.hpp"
#include <string>

namespace pl {

struct HelperPipeline {
    vk::UniqueDescriptorSetLayout descriptorLayout;
    vk::UniqueDescriptorPool descriptorPool;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;
};

using UniqueHelperPipeline = std::unique_ptr<HelperPipeline>;

struct PipelineHelperCreateInfo {
    vk::Device device;
    vk::Extent2D extent;
    uint32_t descriptorCount{};
};

class PipelineHelper {
public:
    explicit PipelineHelper(const PipelineHelperCreateInfo& createInfo);
    // concurrent frames, renderpass, vec<DescriptorType>, vec<ShaderStageFlags>,
    UniqueHelperPipeline createPipeline(uint32_t frames, vk::RenderPass renderPass, std::vector<vk::DescriptorType> descriptorTypes, std::vector<vk::ShaderStageFlags> shaderStageFlags);

private:
    vk::DescriptorSetLayoutBinding createDescriptorSetLayoutBinding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags);
    vk::UniqueDescriptorSetLayout createDescriptorSetLayoutUnique(std::vector<vk::DescriptorSetLayoutBinding> bindings);
    vk::DescriptorPoolSize createDescriptorPoolSize(vk::DescriptorType type, uint32_t descriptorCount);
    vk::UniqueDescriptorPool createDescriptorPoolUnique(uint32_t maxSets, std::vector<vk::DescriptorPoolSize> poolSizes);
    vk::UniquePipelineLayout createPipelineLayoutUnique(vk::DescriptorSetLayout descriptorLayout);
    vk::UniquePipeline createPipelineUnique(vk::RenderPass renderPass, vk::PipelineLayout layout);

    vk::Device device_;
    vk::Extent2D extent_;
    vk::UniqueShaderModule vertexShaderModule_;
    vk::UniqueShaderModule fragmentShaderModule_;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos_;
    vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo_;
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo_;
    vk::Viewport viewport_;
    vk::Rect2D scissor_;
    vk::PipelineViewportStateCreateInfo viewportStateInfo_;
    vk::PipelineRasterizationStateCreateInfo rasterStateInfo_;
    vk::PipelineMultisampleStateCreateInfo multisampleStateInfo_;
    vk::PipelineColorBlendAttachmentState colorBlendAttachment_;
    vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo_;
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateInfo_;
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo_;
};

using UniquePipelineHelper = std::unique_ptr<PipelineHelper>;

UniquePipelineHelper createPipelineHelperUnique(const PipelineHelperCreateInfo& createInfo);

}
