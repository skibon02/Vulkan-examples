cmake_minimum_required(VERSION 3.24)
project(VulkanTriangle)

find_package(Vulkan REQUIRED)

link_directories(${CMAKE_SOURCE_DIR}/external/glfw3/lib-mingw-w64)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")


add_custom_target(CompileShaders
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
        COMMAND glslc ${CMAKE_SOURCE_DIR}/shaders/triangle.vert -o ${CMAKE_BINARY_DIR}/shaders/vert.spv
        COMMAND glslc ${CMAKE_SOURCE_DIR}/shaders/triangle.frag -o ${CMAKE_BINARY_DIR}/shaders/frag.spv
        )

add_executable(VulkanTriangle main.cpp cls.cpp cls.h)
include_directories(
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/external/glfw3/include
)
target_link_libraries(VulkanTriangle
        ${Vulkan_LIBRARIES}
        glfw3
        )
add_dependencies(VulkanTriangle CompileShaders)