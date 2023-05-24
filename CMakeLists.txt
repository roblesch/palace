cmake_minimum_required(VERSION 3.21)
project(palace CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

add_subdirectory(ext)
add_subdirectory(graphics)

add_executable(palace "palace.cpp")
target_link_libraries(palace
		graphics
		Vulkan::Vulkan
		SDL2::SDL2)

if(WIN32)
add_custom_command(TARGET palace POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:palace> $<TARGET_FILE_DIR:palace>
	COMMAND_EXPAND_LISTS)
endif()
