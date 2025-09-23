cpmaddpackage(
    NAME
    protobuf
    GITHUB_REPOSITORY
    protocolbuffers/protobuf
    VERSION
    32.1
    OPTIONS
    # needed to build abseil - https://github.com/abseil/abseil-cpp/issues/1091
    "CMAKE_INCLUDE_CURRENT_DIR FALSE"
    # it has a bunch of warnings, at least with MSVC - make sure it can still build
    "CMAKE_COMPILE_WARNING_AS_ERROR FALSE"
    "protobuf_BUILD_PROTOBUF_BINARIES ON"
    "protobuf_BUILD_TESTS OFF"
    "protobuf_MSVC_STATIC_RUNTIME OFF"
    "protobuf_INSTALL OFF"
)

add_library(protos STATIC)

include("${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake")

set(PROTO_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/protos")
file(GLOB_RECURSE PROTO_FILES "${PROTO_FILES_DIR}/*.proto")
message(STATUS "Detected proto files: ${PROTO_FILES}")

set(PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
protobuf_generate(
    TARGET
    protos
    PROTOC_OUT_DIR
    ${PROTOC_OUT_DIR}
    PROTOS
    ${PROTO_FILES}
    PROTOC_EXE
    ${HOST_PROTOC}
    IMPORT_DIRS
    ${PROTO_FILES_DIR}
)

target_include_directories(protos PUBLIC "${PROTOC_OUT_DIR}")
target_link_libraries(protos PUBLIC protobuf::libprotobuf)
