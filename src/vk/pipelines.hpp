#pragma once

#include "types.hpp"
#include <string>

namespace pl {

struct PipelinesCreateInfo {
    vk::Device device;
    vk::RenderPass renderPass;
    vk::Extent2D extent;
};

class Pipelines {
public:
    Pipelines(const PipelinesCreateInfo& createInfo);

    vk::UniquePipeline createPipelineUnique(vk::RenderPass renderPass);

private:
    vk::Device device_;
    vk::Extent2D extent_;
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
    vk::PipelineLayout pipelineLayout_;
};

}
