#pragma once

#include "types.hpp"
#include "vk_mem_alloc.h"

namespace pl {

/*** types ***/

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

/*** mesh ***/

struct Vertex2 {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

struct Mesh2 {
    std::vector<Vertex2> vertices;
    AllocatedBuffer vertexBuffer;
};

/*** vma ***/

// struct VmaAllocatorDeleter {
//     using pointer = VmaAllocator;
//     void operator()(VmaAllocator allocator) {
//         vmaDestroyAllocator(allocator);
//     }
// };
//
// using UniqueAllocator = std::unique_ptr<VmaAllocator, VmaAllocatorDeleter>;

struct MemoryCreateInfo {
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Instance instance;
};

class Memory {
public:
    Memory(const MemoryCreateInfo& createInfo);
    ~Memory();

    void loadMesh();
    vk::Result uploadMesh(Mesh2& mesh);

private:
    VmaAllocator allocator_;
    Mesh2 mesh_;
};

using UniqueMemory = std::unique_ptr<Memory>;

UniqueMemory createMemoryUnique(const MemoryCreateInfo& createInfo);

}
