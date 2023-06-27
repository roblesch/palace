#include "pipelines.hpp"

#include <fstream>
#include "gltf.hpp"

namespace pl {

std::vector<char> readSpirVFile(const std::string& spirVFile)
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

PipelineHelper::PipelineHelper(const PipelineHelperCreateInfo& createInfo)
    : device_(createInfo.device)
    , extent_(createInfo.extent)
    , descriptorCount_(createInfo.descriptorCount)
    , descriptorLayout_(createInfo.descriptorLayout)
{
    // shaders
    std::vector<char> vertexShaderBytes = readSpirVFile("shaders/vertex.spv");
    std::vector<char> fragmentShaderBytes = readSpirVFile("shaders/fragment.spv");

    auto vertexShaderModule = device_.createShaderModuleUnique({ .codeSize = vertexShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(vertexShaderBytes.data()) });

    auto fragmentShaderModule = device_.createShaderModuleUnique({ .codeSize = fragmentShaderBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(fragmentShaderBytes.data()) });

    shaderStageInfos_ = {
        { .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *vertexShaderModule,
            .pName = "main" },
        { .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *fragmentShaderModule,
            .pName = "main" }
    };

    // vertex input
    vk::VertexInputBindingDescription vertexBindingDescription {
        .binding = 0,
        .stride = sizeof(pl::Vertex),
        .inputRate = vk::VertexInputRate::eVertex
    };

    vk::VertexInputAttributeDescription posDescription {
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(pl::Vertex, pos)
    };
    vk::VertexInputAttributeDescription normalDescription {
        .location = 1,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(pl::Vertex, normal)
    };
    vk::VertexInputAttributeDescription colorDescription {
        .location = 2,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(pl::Vertex, color)
    };
    vk::VertexInputAttributeDescription uvDescription {
        .location = 3,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(pl::Vertex, uv)
    };
    std::array<vk::VertexInputAttributeDescription, 4> vertexAttributeDescriptions { posDescription, normalDescription, colorDescription, uvDescription };

    vertexInputStateInfo_ = {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBindingDescription,
        .vertexAttributeDescriptionCount = vertexAttributeDescriptions.size(),
        .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
    };

    // input assembly
    inputAssemblyStateInfo_ = {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = VK_FALSE
    };

    // viewport state
    viewport_ = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent_.width),
        .height = static_cast<float>(extent_.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    scissor_ = {
        .offset = { 0, 0 },
        .extent = extent_
    };

    viewportStateInfo_ = {
        .viewportCount = 1,
        .pViewports = &viewport_,
        .scissorCount = 1,
        .pScissors = &scissor_
    };

    // rasterization state
    rasterStateInfo_ = {
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    // multisample state
    multisampleStateInfo_ = {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = VK_FALSE
    };

    // color blend state
    colorBlendAttachment_ = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    colorBlendStateInfo_ = {
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment_
    };

    // depth state
    depthStencilStateInfo_ = {
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    // dynamic state
    std::vector<vk::DynamicState> dynamicStates {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    dynamicStateInfo_ = {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // pipeline layout
    vk::PushConstantRange pushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(glm::mat4)
    };
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorLayout_,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    pipelineLayout_ = device_.createPipelineLayoutUnique(pipelineLayoutInfo);
}

vk::UniquePipeline PipelineHelper::createPipelineUnique(vk::RenderPass renderPass)
{
    vk::GraphicsPipelineCreateInfo pipelineInfo {
        .stageCount = static_cast<uint32_t>(shaderStageInfos_.size()),
        .pVertexInputState = &vertexInputStateInfo_,
        .pInputAssemblyState = &inputAssemblyStateInfo_,
        .pViewportState = &viewportStateInfo_,
        .pRasterizationState = &rasterStateInfo_,
        .pMultisampleState = &multisampleStateInfo_,
        .pColorBlendState = &colorBlendStateInfo_,
        .layout = *pipelineLayout_,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE
    };

    return device_.createGraphicsPipelineUnique(nullptr, pipelineInfo).value;
}

UniquePipelineHelper createPipelineHelperUnique(const PipelineHelperCreateInfo& createInfo)
{
    return std::make_unique<PipelineHelper>(createInfo);
}

}