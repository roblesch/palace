#include "gltf.hpp"

#include "util/log.hpp"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#include <filesystem>

namespace fs = std::filesystem;

namespace pl {

void GltfModel::loadImages(const char* path, tinygltf::Model& model)
{
    for (const auto& _image : model.images) {
        auto texture = std::make_shared<Texture>();
        texture->name = (fs::path(path).parent_path() / _image.uri).string();
        texture->extent = vk::Extent3D {
            .width = static_cast<uint32_t>(_image.width),
            .height = static_cast<uint32_t>(_image.height),
            .depth = 1
        };
        auto size = texture->extent.width * texture->extent.height * 4 * sizeof(unsigned char);
        auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(_image.width, _image.height)))) + 1;
        texture->image = memoryHelper->createTextureImage(_image.image.data(), size, texture->extent, mipLevels);
        texture->view = memoryHelper->createImageViewUnique(texture->image->image, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, mipLevels);
        texture->sampler = memoryHelper->createTextureSamplerUnique(mipLevels);
        textures.push_back(texture);
    }
}

void GltfModel::loadMaterials(tinygltf::Model& model)
{
    for (const auto& _material : model.materials) {
        auto material = std::make_shared<Material>();
        materials.push_back(material);
        material->name = _material.name;

        // base color
        if (_material.pbrMetallicRoughness.baseColorTexture.index > -1) {
            material->baseColor = textures[model.textures[_material.pbrMetallicRoughness.baseColorTexture.index].source].get();
        } else {
            auto texture = std::make_shared<Texture>();
            texture->name = _material.name + "_BASE_COLOR";
            texture->extent = vk::Extent3D {
                .width = 32,
                .height = 32,
                .depth = 1
            };
            auto size = texture->extent.width * texture->extent.height * 4 * sizeof(unsigned char);
            uint32_t px = 32 * 32;
            uint32_t mipLevels = 1;
            std::vector<unsigned char> baseColor;
            for (uint32_t i = 0; i < px; i++) {
                baseColor.push_back(_material.pbrMetallicRoughness.baseColorFactor[0]);
                baseColor.push_back(_material.pbrMetallicRoughness.baseColorFactor[1]);
                baseColor.push_back(_material.pbrMetallicRoughness.baseColorFactor[2]);
                baseColor.push_back(_material.pbrMetallicRoughness.baseColorFactor[3]);
            }
            texture->image = memoryHelper->createTextureImage(baseColor.data(), size, texture->extent, mipLevels);
            texture->view = memoryHelper->createImageViewUnique(texture->image->image, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, mipLevels);
            texture->sampler = memoryHelper->createTextureSamplerUnique(mipLevels);
            textures.push_back(texture);
            material->baseColor = texture.get();
        }
        if (_material.normalTexture.index > -1) {
            material->useNormalTexture = 1.0f;
            material->normal = textures[model.textures[_material.normalTexture.index].source].get();
        }
    }
}

void GltfModel::loadMeshes(tinygltf::Model& model)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& _mesh : model.meshes) {
        auto mesh = std::make_shared<Mesh>();
        mesh->name = _mesh.name;

        for (const auto& _primitive : _mesh.primitives) {
            if (_primitive.indices < 0)
                continue;

            auto primitive = std::make_shared<Primitive>();
            primitive->firstVertex = static_cast<uint32_t>(vertices.size());
            primitive->firstIndex = static_cast<uint32_t>(indices.size());

            const float* positions;
            const float* normals;
            const float* texCoords;

            // positions
            {
                const auto& accessor = model.accessors[_primitive.attributes.at("POSITION")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                primitive->vertexCount = static_cast<uint32_t>(accessor.count);
                positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // normals
            if (_primitive.attributes.find("NORMAL") != _primitive.attributes.end()) {
                const auto& accessor = model.accessors[_primitive.attributes.at("NORMAL")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                normals = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // texCoords
            if (_primitive.attributes.find("TEXCOORD_0") != _primitive.attributes.end()) {
                const auto& accessor = model.accessors[_primitive.attributes.at("TEXCOORD_0")];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                texCoords = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            // material
            auto color = glm::make_vec4(model.materials[_primitive.material].pbrMetallicRoughness.baseColorFactor.data());
            primitive->material = materials[_primitive.material].get();

            // vertices
            for (size_t i = 0; i < primitive->vertexCount; i++) {
                vertices.push_back({ glm::make_vec3(&positions[i * 3]),
                    glm::make_vec3(&normals[i * 3]),
                    color,
                    glm::make_vec2(&texCoords[i * 2]) });
            }

            // indices
            {
                const auto& accessor = model.accessors[_primitive.indices];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];
                primitive->indexCount = accessor.count;

                auto readIndexBuffer = [&]<typename T>(T dummy) {
                    T* buf = new T[accessor.count];
                    memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(T));
                    for (size_t i = 0; i < accessor.count; i++) {
                        indices.push_back(buf[i] + primitive->firstVertex);
                    }
                    delete[] buf;
                };

                switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    readIndexBuffer(uint32_t {});
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    readIndexBuffer(uint16_t {});
                    break;
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    readIndexBuffer(uint8_t {});
                    break;
                }
            }

            primitives.push_back(primitive);
            mesh->primitives.push_back(primitive.get());
        }
        meshes.push_back(mesh);
    }

    // buffers
    vertexBuffer = memoryHelper->createBuffer(vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, {});
    indexBuffer = memoryHelper->createBuffer(indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, {});
    memoryHelper->uploadToBuffer(vertexBuffer, vertices.data());
    memoryHelper->uploadToBuffer(indexBuffer, indices.data());
}

glm::mat4 Node::localMatrix() {
    return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
}

glm::mat4 Node::globalMatrix() {
    glm::mat4 m = localMatrix();
    Node* p = parent;
    while (p) {
        m = p->localMatrix() * m;
        p = p->parent;
    }
    return m;
}

void GltfModel::loadNode(Scene* scene, Node* parent, tinygltf::Node& node, tinygltf::Model& model)
{
    auto newNode = std::make_shared<Node>();
    scene->nodes.push_back(newNode.get());
    nodes.push_back(newNode);
    newNode->parent = parent;
    newNode->name = node.name;
    newNode->matrix = glm::mat4(1.0f);

    // Generate local node matrix
    glm::vec3 translation = glm::vec3(0.0f);
    if (node.translation.size() == 3) {
        translation = glm::make_vec3(node.translation.data());
        newNode->translation = translation;
    }
    glm::mat4 rotation = glm::mat4(1.0f);
    if (node.rotation.size() == 4) {
        glm::quat q = glm::make_quat(node.rotation.data());
        newNode->rotation = glm::mat4(q);
    }
    glm::vec3 scale = glm::vec3(1.0f);
    if (node.scale.size() == 3) {
        scale = glm::make_vec3(node.scale.data());
        newNode->scale = scale;
    }
    if (node.matrix.size() == 16) {
        newNode->matrix = glm::make_mat4x4(node.matrix.data());
    };

    //newNode->matrix = glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * newNode->matrix;

    if (node.children.size() > 0) {
        for (size_t i = 0; i < node.children.size(); i++) {
            loadNode(scene, newNode.get(), model.nodes[node.children[i]], model);
        }
    }

    if (node.mesh > -1) {
        newNode->mesh = meshes[node.mesh].get();
    }
}

GltfModel::GltfModel(const GltfModelCreateInfo& createInfo) : memoryHelper(createInfo.memory)
{
    defaultScene = nullptr;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string warn, err;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, createInfo.path);

    if (!warn.empty()) {
        pl::LOG_WARN(warn.c_str(), "GLTF");
    }

    if (!err.empty()) {
        pl::LOG_ERROR(err.c_str(), "GLTF");
        return;
    }

    if (!ret) {
        pl::LOG_ERROR("Failed to parse gltf file", "GLTF");
        return;
    }

    // textures
    loadImages(createInfo.path, model);

    // materials
    loadMaterials(model);

    // meshes
    loadMeshes(model);

    // scenes
    for (const auto& _scene : model.scenes) {
        auto scene = std::make_shared<Scene>();
        scene->name = _scene.name;
        for (int i : _scene.nodes) {
            loadNode(scene.get(), nullptr, model.nodes[i], model);            
        }
        scenes.push_back(scene);
    }
    defaultScene = scenes[model.defaultScene].get();
}

UniqueGltfModel createGltfModelUnique(const GltfModelCreateInfo& createInfo)
{
    return std::make_unique<GltfModel>(createInfo);
}

}
