#include "pipeline.hpp"

#include <fstream>
#include "primitive.hpp"

namespace vk_ {

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

Pipeline::Pipeline(vk::Device& device, vk::Extent2D& extent2D, uint32_t concurrentFrames)
{
    // descriptor set layout
    vk::DescriptorSetLayoutBinding uboLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    };

    vk::DescriptorSetLayoutCreateInfo descriptorLayoutInfo {
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding
    };

    m_uniqueDescriptorSetLayout = device.createDescriptorSetLayoutUnique(descriptorLayoutInfo);

    // shaders
    std::vector<char> vertexShaderBytes = readSpirVFile("shaders/vertex.spv");
    std::vector<char> fragmentShaderBytes = readSpirVFile("shaders/fragment.spv");

    auto vertexShaderModule = device.createShaderModuleUnique({ .codeSize = vertexShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(vertexShaderBytes.data()) });

    auto fragmentShaderModule = device.createShaderModuleUnique({ .codeSize = fragmentShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(fragmentShaderBytes.data()) });

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos {
        { .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *vertexShaderModule,
            .pName = "main" },
        { .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *fragmentShaderModule,
            .pName = "main" }
    };

    // vertex input
    auto bindingDescription = vk_::Vertex::bindingDescription();
    auto attributeDescriptions = vk_::Vertex::attributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexStateInfo {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    // input assembly
    vk::PipelineInputAssemblyStateCreateInfo assemblyStateInfo {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = VK_FALSE
    };

    // viewport state
    vk::Viewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent2D.width),
        .height = static_cast<float>(extent2D.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vk::Rect2D scissor {
        .offset = { 0, 0 },
        .extent = extent2D
    };

    vk::PipelineViewportStateCreateInfo viewportStateInfo {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // rasterization state
    // TODO: debugging toggle mode
    vk::PipelineRasterizationStateCreateInfo rasterStateInfo {
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    // multisample state
    vk::PipelineMultisampleStateCreateInfo multisampleStateInfo {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = VK_FALSE
    };

    // color blend state
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

    // dynamic state
    std::vector<vk::DynamicState> dynamicStates {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &(*m_uniqueDescriptorSetLayout)
    };
    m_uniquePipelineLayout = device.createPipelineLayoutUnique(pipelineLayoutInfo);

    // render pass
    vk::AttachmentDescription colorAttachment {
        .format = vk::Format::eB8G8R8A8Srgb,
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

    m_uniqueRenderPass = device.createRenderPassUnique(renderPassInfo);

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
        .layout = *m_uniquePipelineLayout,
        .renderPass = *m_uniqueRenderPass,
        .subpass = 0
    };

    m_uniquePipeline = device.createGraphicsPipelineUnique(nullptr, graphicsPipelineInfo).value;
}

vk::RenderPass& Pipeline::renderPass()
{
    return *m_uniqueRenderPass;
}

vk::Pipeline& Pipeline::pipeline()
{
    return *m_uniquePipeline;
}

vk::PipelineLayout& Pipeline::pipelineLayout()
{
    return *m_uniquePipelineLayout;
}

vk::DescriptorSetLayout& Pipeline::descriptorSetLayout()
{
    return *m_uniqueDescriptorSetLayout;
}

}
