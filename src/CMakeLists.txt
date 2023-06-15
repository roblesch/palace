add_subdirectory(gltf)
add_subdirectory(util)
add_subdirectory(vk)

add_executable(palace "palace.cpp")
set_target_properties(palace PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}
		RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR})
target_link_libraries(palace pl::gltf pl::util pl::vk)

# copy dlls
if(WIN32)
add_custom_command(TARGET palace POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:palace> $<TARGET_FILE_DIR:palace>
	COMMAND_EXPAND_LISTS)
endif()

# compile & copy shaders
file(GLOB_RECURSE GLSL_SOURCE_FILES
		"${PROJECT_SOURCE_DIR}/shaders/*.frag"
		"${PROJECT_SOURCE_DIR}/shaders/*.vert")
foreach(GLSL ${GLSL_SOURCE_FILES})
	get_filename_component(FILE_NAME ${GLSL} NAME_WE)
	set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
	add_custom_command(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
			COMMAND glslc ${GLSL} -o ${SPIRV}
			DEPENDS ${GLSL})
	list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(palace shaders)
