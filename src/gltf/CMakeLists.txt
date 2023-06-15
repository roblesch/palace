add_library(gltf "scene.cpp")
add_library(pl::gltf ALIAS gltf)
target_link_libraries(gltf glm::glm pl::util tinygltf)
