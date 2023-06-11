#include "pipeline.hpp"

#include "buffer.hpp"
#include "vertex.hpp"
#include <fstream>

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
    // uniform modelview transforms
    vk::DescriptorSetLayoutBinding uboLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    };

    // image sampler
    vk::DescriptorSetLayoutBinding samplerLayoutBinding {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    // layout
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings { uboLayoutBinding, samplerLayoutBinding };
    vk::DescriptorSetLayoutCreateInfo descriptorLayoutInfo {
        .bindingCount = 2,
        .pBindings = bindings.data()
    };

    m_descriptorSetLayout = device.createDescriptorSetLayoutUnique(descriptorLayoutInfo);

    // descriptor pools
    vk::DescriptorPoolSize uboSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = concurrentFrames
    };

    vk::DescriptorPoolSize samplerSize {
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = concurrentFrames
    };

    std::array<vk::DescriptorPoolSize, 2> poolSizes { uboSize, samplerSize };

    vk::DescriptorPoolCreateInfo descriptorPoolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = concurrentFrames,
        .poolSizeCount = 2,
        .pPoolSizes = poolSizes.data()
    };

    m_descriptorPool = device.createDescriptorPoolUnique(descriptorPoolInfo);

    // descriptor sets
    std::vector<vk::DescriptorSetLayout> layouts(concurrentFrames, *m_descriptorSetLayout);

    vk::DescriptorSetAllocateInfo descriptorSetInfo {
        .descriptorPool = *m_descriptorPool,
        .descriptorSetCount = concurrentFrames,
        .pSetLayouts = layouts.data()
    };

    m_descriptorSets = device.allocateDescriptorSetsUnique(descriptorSetInfo);

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

    // depth state
    vk::PipelineDepthStencilStateCreateInfo depthStateInfo {
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

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &(*m_descriptorSetLayout)
    };
    m_pipelineLayout = device.createPipelineLayoutUnique(pipelineLayoutInfo);

    // attachments
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

    vk::AttachmentDescription depthAttachment {
        .format = vk::Format::eD32Sfloat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::AttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    // subpass
    vk::SubpassDescription subpass {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    vk::SubpassDependency dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
    };

    // renderpass
    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    vk::RenderPassCreateInfo renderPassInfo {
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    m_renderPass = device.createRenderPassUnique(renderPassInfo);

    // pipeline
    vk::GraphicsPipelineCreateInfo graphicsPipelineInfo {
        .stageCount = static_cast<uint32_t>(shaderStageInfos.size()),
        .pStages = shaderStageInfos.data(),
        .pVertexInputState = &vertexStateInfo,
        .pInputAssemblyState = &assemblyStateInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterStateInfo,
        .pMultisampleState = &multisampleStateInfo,
        .pDepthStencilState = &depthStateInfo,
        .pColorBlendState = &colorBlendStateInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = *m_pipelineLayout,
        .renderPass = *m_renderPass,
        .subpass = 0
    };

    m_pipeline = device.createGraphicsPipelineUnique(nullptr, graphicsPipelineInfo).value;
}

vk::RenderPass& Pipeline::renderPass()
{
    return *m_renderPass;
}

vk::Pipeline& Pipeline::pipeline()
{
    return *m_pipeline;
}

vk::PipelineLayout& Pipeline::pipelineLayout()
{
    return *m_pipelineLayout;
}

vk::DescriptorSetLayout& Pipeline::descriptorSetLayout()
{
    return *m_descriptorSetLayout;
}

vk::DescriptorSet& Pipeline::descriptorSet(size_t frame)
{
    return *m_descriptorSets[frame];
}

void Pipeline::setDescriptorSets(vk::Device& device, std::vector<vk::UniqueBuffer>& uniformBuffers, vk::Sampler& sampler, vk::ImageView& imageView, uint32_t concurrentFrames)
{
    m_descriptorSets.resize(concurrentFrames);

    for (size_t i = 0; i < concurrentFrames; i++) {
        vk::DescriptorBufferInfo uboInfo {
            .buffer = *uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        vk::DescriptorImageInfo samplerInfo {
            .sampler = sampler,
            .imageView = imageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet uboWriteDescriptor {
            .dstSet = *m_descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &uboInfo
        };

        vk::WriteDescriptorSet samplerWriteDescriptor {
            .dstSet = *m_descriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &samplerInfo
        };

        std::array<vk::WriteDescriptorSet, 2> writeDescriptors { uboWriteDescriptor, samplerWriteDescriptor };
        device.updateDescriptorSets(writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
    }
}

}
