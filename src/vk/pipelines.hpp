#pragma once

#include "types.hpp"
#include <string>

namespace pl {

struct PipelineHelperCreateInfo {
    vk::Device device;
    vk::Extent2D extent;
    uint32_t descriptorCount{};
    vk::DescriptorSetLayout descriptorLayout;
};

class PipelineHelper {
public:
    explicit PipelineHelper(const PipelineHelperCreateInfo& createInfo);

    vk::UniquePipelineLayout createPipelineLayoutUnique();
    vk::UniquePipeline createPipelineUnique(vk::RenderPass renderPass, vk::PipelineLayout layout);

private:
    vk::Device device_;
    vk::Extent2D extent_;
    vk::DescriptorSetLayout descriptorLayout_;
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
