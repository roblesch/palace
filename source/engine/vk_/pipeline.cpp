#include "pipeline.hpp"

#include "vulkan/vulkan.h"

#include <fstream>

namespace vk_ {

Pipeline::Pipeline(const vk::Device* device, const std::string& spirVDir, const vk::Extent2D& extent2D, const vk::Format& imageFormat)
    : m_device(device)
    , m_extent2D(extent2D)
    , m_imageFormat(imageFormat)
{
    // shaders
    std::vector<char> vertexShaderBytes = readSpirVFile("shaders/vertex.spv");
    std::vector<char> fragmentShaderBytes = readSpirVFile("shaders/fragment.spv");

    auto vertexShaderModule = device->createShaderModuleUnique({ .codeSize = vertexShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(vertexShaderBytes.data()) });

    auto fragmentShaderModule = device->createShaderModuleUnique({ .codeSize = fragmentShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(fragmentShaderBytes.data()) });

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos {
        { .stage = vk::ShaderStageFlagBits::eVertex,
            .module = vertexShaderModule.get(),
            .pName = "main" },
        { .stage = vk::ShaderStageFlagBits::eFragment,
            .module = fragmentShaderModule.get(),
            .pName = "main" }
    };

    // vertex
    vk::PipelineVertexInputStateCreateInfo vertexStateInfo {};

    // assembly
    vk::PipelineInputAssemblyStateCreateInfo assemblyStateInfo {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = VK_FALSE
    };

    // viewport
    vk::Viewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_extent2D.width),
        .height = static_cast<float>(m_extent2D.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vk::Rect2D scissor {
        .offset = { 0, 0 },
        .extent = m_extent2D
    };

    vk::PipelineViewportStateCreateInfo viewportStateInfo {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // raster
    // TODO: debugging toggle mode
    vk::PipelineRasterizationStateCreateInfo rasterStateInfo {
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    // multisampling
    vk::PipelineMultisampleStateCreateInfo multisampleStateInfo {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = VK_FALSE
    };

    // color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo {
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    // dynamic
    std::vector<vk::DynamicState> dynamicStates {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {};

    m_pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutInfo);

    // render pass
    vk::AttachmentDescription colorAttachment {
        .format = m_imageFormat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR
    };

    vk::AttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    // subpass
    vk::SubpassDescription subpass {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    vk::RenderPassCreateInfo renderPassInfo {
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    m_renderPass = m_device->createRenderPassUnique(renderPassInfo);

    // pipeline
    vk::GraphicsPipelineCreateInfo graphicsPipelineInfo {
        .stageCount = static_cast<uint32_t>(shaderStageInfos.size()),
        .pStages = shaderStageInfos.data(),
        .pVertexInputState = &vertexStateInfo,
        .pInputAssemblyState = &assemblyStateInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterStateInfo,
        .pMultisampleState = &multisampleStateInfo,
        .pColorBlendState = &colorBlendStateInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = m_pipelineLayout.get(),
        .renderPass = m_renderPass.get(),
        .subpass = 0
    };

    // TODO: creating a graphics pipeline requires binding
    auto [result, m_pipeline] = m_device->createGraphicsPipelineUnique(nullptr, graphicsPipelineInfo);
}

std::vector<char> Pipeline::readSpirVFile(const std::string& spirVFile)
{
    std::ifstream file(spirVFile, std::ios::binary | std::ios::in | std::ios::ate);

    if (!file.is_open()) {
        printf("(VK_:ERROR) Pipeline failed to open spir-v file for reading: %s", spirVFile.c_str());
        exit(1);
    }

    size_t fileSize = file.tellg();
    file.seekg(std::ios::beg);
    std::vector<char> spirVBytes(fileSize);
    file.read(spirVBytes.data(), fileSize);
    file.close();

    return spirVBytes;
}

}
