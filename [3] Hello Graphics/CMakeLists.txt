cmake_minimum_required(VERSION 3.22)
project(HelloVulkan)

find_package(Vulkan REQUIRED)

if (UNIX)
    find_package(glfw3 REQUIRED)
    if(NOT glfw3_FOUND)
        message(FATAL_ERROR "GLFW3 not found. Install on ubuntu: sudo apt-get install libglfw3-dev")
    endif()
    set(GLFW3_LIBRARY glfw)
elseif(WIN32)
    set(GLFW3_LIBRARY glfw3)
    set(GLFW3_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/glfw3/include)
endif()



add_custom_target(CompileShaders
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
        COMMAND glslc ${CMAKE_SOURCE_DIR}/shaders/triangle.vert -o ${CMAKE_BINARY_DIR}/shaders/vert.spv
        COMMAND glslc ${CMAKE_SOURCE_DIR}/shaders/triangle.frag -o ${CMAKE_BINARY_DIR}/shaders/frag.spv
        )


link_directories(${CMAKE_SOURCE_DIR}/external/glfw3/lib-mingw-w64)

add_executable(HelloVulkan main.cpp)
include_directories(
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW3_INCLUDE_DIRS}
)
target_link_libraries(HelloVulkan
        ${Vulkan_LIBRARIES}
        ${GLFW3_LIBRARY}
        )
add_dependencies(HelloVulkan CompileShaders)

