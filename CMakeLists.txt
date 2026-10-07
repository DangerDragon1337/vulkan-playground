cmake_minimum_required(VERSION 3.20)
project(MyVulkanProject LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(SLANGC_EXECUTABLE slangc HINTS "$ENV{VULKAN_SDK}/bin" REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

function(add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})

    # Put compiled binaries in the build output directory, NOT the source folder
    set(SHADER_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

    # Ensure output directory exists before running slangc
    file(MAKE_DIRECTORY "${SHADER_OUT_DIR}")

    set(COMPILED_SPV "${SHADER_OUT_DIR}/${TARGET}.spv")

    add_custom_command(
        OUTPUT "${COMPILED_SPV}"
        COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            -entry vertMain
            -entry fragMain
            -o "${COMPILED_SPV}"
        DEPENDS ${SHADER_SOURCES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling Slang Shader Target: ${TARGET}"
        VERBATIM
    )

    # Create the custom target that your executable can depend on
    add_custom_target(${TARGET} DEPENDS "${COMPILED_SPV}")
endfunction()

add_slang_shader_target(
    Shaders SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/shader.slang"
)

add_executable(my_vulkan_app
    src/main.cpp
    src/MakeTriangle.cpp
    src/functions.cpp
)

add_dependencies(my_vulkan_app
    Shaders
)

target_include_directories(my_vulkan_app PRIVATE include)

target_link_libraries(my_vulkan_app PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)
