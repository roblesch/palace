add_library(gltf "scene.cpp")
target_link_libraries(gltf
        util
        glm::glm
        tinygltf
        tinyobjloader)
