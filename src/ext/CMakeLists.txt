find_package(OpenImageIO CONFIG REQUIRED)
add_library(oiio_config INTERFACE)
target_link_libraries(oiio_config INTERFACE OpenImageIO::OpenImageIO)

find_package(fmt CONFIG REQUIRED)
add_library(fmt_config INTERFACE)
target_link_libraries(fmt_config INTERFACE fmt::fmt)

find_package(spdlog CONFIG REQUIRED)
add_library(spdlog_config INTERFACE)
target_link_libraries(spdlog_config INTERFACE spdlog::spdlog)

add_library(jitify_config INTERFACE)
target_include_directories(jitify_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(cxxopts CONFIG REQUIRED)
add_library(cxxopts_config INTERFACE)
target_link_libraries(cxxopts_config INTERFACE cxxopts::cxxopts)

find_package(assimp CONFIG REQUIRED)
add_library(assimp_config INTERFACE)
target_link_libraries(assimp_config INTERFACE assimp::assimp)

add_library(tinyexr_config INTERFACE)
target_include_directories(tinyexr_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stb_config INTERFACE)
target_include_directories(stb_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (LUISA_RENDER_ENABLE_OPENCL)
    find_package(OpenCL REQUIRED)
    add_library(opencl_config INTERFACE)
    target_compile_definitions(opencl_config INTERFACE
                               CL_HPP_TARGET_OPENCL_VERSION=120
                               CL_HPP_MINIMUM_OPENCL_VERSION=120
                               CL_HPP_CL_1_2_DEFAULT_BUILD
                               CL_HPP_ENABLE_EXCEPTIONS)
    target_include_directories(opencl_config INTERFACE OpenCL-CLHPP/include)
    target_link_libraries(opencl_config INTERFACE OpenCL::OpenCL)
endif ()
