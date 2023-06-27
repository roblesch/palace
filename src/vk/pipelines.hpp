#pragma once

#include "types.hpp"
#include <string>

namespace pl {

struct PipelineHelperCreateInfo {
    vk::Device device;
    vk::Extent2D extent;
    uint32_t descriptorCount;
    vk::DescriptorSetLayout descriptorLayout;
};

class PipelineHelper {
public:
    PipelineHelper(const PipelineHelperCreateInfo& createInfo);

    vk::UniquePipeline createPipelineUnique(vk::RenderPass renderPass);

private:
    vk::Device device_;
    vk::Extent2D extent_;
    uint32_t descriptorCount_;
    vk::DescriptorSetLayout descriptorLayout_;
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
    vk::UniquePipelineLayout pipelineLayout_;
};

using UniquePipelineHelper = std::unique_ptr<PipelineHelper>;

UniquePipelineHelper createPipelineHelperUnique(const PipelineHelperCreateInfo& createInfo);

}
