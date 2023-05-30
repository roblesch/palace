#include "pipeline.hpp"

#include <fstream>

namespace vk_ {

Pipeline::Pipeline(const vk::Device& device, const std::string& spirVDir)
    : m_device(&device)
{
    vk::UniqueShaderModule vertexShaderModule = loadShaderModule("shaders/vertex.spv");
    vk::UniqueShaderModule fragmentShaderModule = loadShaderModule("shaders/fragment.spv");

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos {
        { .stage = vk::ShaderStageFlagBits::eVertex,
            .module = vertexShaderModule.get(),
            .pName = "main" },
        { .stage = vk::ShaderStageFlagBits::eFragment,
            .module = vertexShaderModule.get(),
            .pName = "main" }
    };
}

vk::UniqueShaderModule Pipeline::loadShaderModule(const std::string& spirVFile)
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

    vk::ShaderModuleCreateInfo shaderModuleInfo {
        .codeSize = spirVBytes.size(),
        .pCode = reinterpret_cast<const uint32_t*>(spirVBytes.data())
    };

    return m_device->createShaderModuleUnique(shaderModuleInfo);
}

}
