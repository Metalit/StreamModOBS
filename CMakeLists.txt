cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

# needed for protobuf
message("VCPKG $ENV{VCPKG_ROOT}")
string(REPLACE "\\" "/" VCPKG_ROOT_WINDOWS_FIX $ENV{VCPKG_ROOT})
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT_WINDOWS_FIX}/scripts/buildsystems/vcpkg.cmake)

project(${_name} VERSION ${_version})

# obs stuff
include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

# add finders for asio, websocketpp, and ffmpeg
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# find websocketpp
find_package(Asio REQUIRED)
find_package(Websocketpp REQUIRED)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ASIO_STANDALONE)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Asio::Asio Websocketpp::Websocketpp)

# find ffmpeg
find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE FFmpeg::avcodec FFmpeg::avutil)

# find protobuf
find_package(Protobuf REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE protobuf::libprotobuf)

# note the install(IMPORTED_RUNTIME_ARTIFACTS) in cmake/x/helpers.cmake because protobuf seems impossible to statically link

# find protobuf files
file(GLOB PROTO_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/protos/*.proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# generate and add protobuf files (only on build)
protobuf_generate(
    TARGET ${CMAKE_PROJECT_NAME}
    LANGUAGE cpp
    GENERATE_EXTENSIONS .pb.h .pb.cc
    PROTOS ${PROTO_SRC_FILES}
    PROTOC_OUT_DIR ${PROTO_GEN_DIR}
)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${PROTO_GEN_DIR})

# add plugin definitions and sources
add_compile_definitions("PLUGIN_NAME=\"${CMAKE_PROJECT_NAME}\"")
add_compile_definitions("PLUGIN_VERSION=\"${CMAKE_PROJECT_VERSION}\"")

# add all cpp files in src as sources
file(GLOB SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${SRC_FILES})

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
