#include "memory.hpp"

#define VMA_IMPLEMENTATION

namespace pl {

Memory::Memory(const MemoryCreateInfo& createInfo)
{
    VmaAllocatorCreateInfo allocatorInfo {
        .physicalDevice = createInfo.physicalDevice,
        .device = createInfo.device,
        .instance = createInfo.instance
    };

    vmaCreateAllocator(&allocatorInfo, &allocator_);
}

Memory::~Memory()
{
    vmaDestroyBuffer(allocator_, mesh_.vertexBuffer.buffer, mesh_.vertexBuffer.allocation);
    vmaDestroyAllocator(allocator_);
}

void Memory::loadMesh()
{
    // make the array 3 vertices long
    mesh_.vertices.resize(3);

    // vertex positions
    mesh_.vertices[0].position = { 1.f, 1.f, 0.0f };
    mesh_.vertices[1].position = { -1.f, 1.f, 0.0f };
    mesh_.vertices[2].position = { 0.f, -1.f, 0.0f };

    // vertex colors, all green
    mesh_.vertices[0].color = { 0.f, 1.f, 0.0f }; // pure green
    mesh_.vertices[1].color = { 0.f, 1.f, 0.0f }; // pure green
    mesh_.vertices[2].color = { 0.f, 1.f, 0.0f }; // pure green

    uploadMesh(mesh_);
}

vk::Result Memory::uploadMesh(Mesh2& mesh)
{
    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = mesh.vertices.size() * sizeof(Vertex2),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    };

    VmaAllocationCreateInfo allocationInfo {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };

    auto result = vmaCreateBuffer(allocator_, &bufferInfo, &allocationInfo, &mesh.vertexBuffer.buffer, &mesh.vertexBuffer.allocation, VK_NULL_HANDLE);

    void* data;
    vmaMapMemory(allocator_, mesh.vertexBuffer.allocation, &data);
    memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex2));
    vmaUnmapMemory(allocator_, mesh.vertexBuffer.allocation);

    return static_cast<vk::Result>(result);
}

UniqueMemory createMemoryUnique(const MemoryCreateInfo& createInfo)
{
    return std::make_unique<Memory>(createInfo);
}

}
