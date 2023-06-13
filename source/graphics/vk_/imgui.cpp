#include "imgui.hpp"

#include "device.hpp"

namespace vk_ {

Imgui::Imgui(SDL_Window* window, vk::Instance& instance, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::Queue& graphicsQueue, vk::RenderPass& renderPass, vk::CommandPool& commandPool)
{
    vk::DescriptorPoolSize poolSizes[] = {
        { vk::DescriptorType::eSampler, 1000 },
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampledImage, 1000 },
        { vk::DescriptorType::eStorageImage, 1000 },
        { vk::DescriptorType::eUniformTexelBuffer, 1000 },
        { vk::DescriptorType::eStorageTexelBuffer, 1000 },
        { vk::DescriptorType::eUniformBuffer, 1000 },
        { vk::DescriptorType::eStorageBuffer, 1000 },
        { vk::DescriptorType::eUniformBufferDynamic, 1000 },
        { vk::DescriptorType::eStorageBufferDynamic, 1000 },
        { vk::DescriptorType::eInputAttachment, 1000 }
    };

    vk::DescriptorPoolCreateInfo poolInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1000,
        .poolSizeCount = std::size(poolSizes),
        .pPoolSizes = poolSizes
    };

    m_descriptorPool = device.createDescriptorPoolUnique(poolInfo);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan(window);

    ImGui_ImplVulkan_InitInfo imguiInfo {
        .Instance = instance,
        .PhysicalDevice = physicalDevice,
        .Device = device,
        .Queue = graphicsQueue,
        .DescriptorPool = *m_descriptorPool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    };

    ImGui_ImplVulkan_Init(&imguiInfo, renderPass);
    auto cmd = Device::beginSingleUseCommandBuffer(device, commandPool);
    ImGui_ImplVulkan_CreateFontsTexture(*cmd);
    Device::endSingleUseCommandBuffer(*cmd, graphicsQueue);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Imgui::cleanup()
{
    ImGui_ImplVulkan_Shutdown();
}

}
