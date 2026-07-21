cmake_minimum_required(VERSION 3.21)
project(AtlasRuntime VERSION 0.5.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED XNVSE_SDK)
  message(FATAL_ERROR "Set XNVSE_SDK to the xNVSE plugin SDK root")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ATLAS_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(ATLAS_GENERATED_SOURCE "${ATLAS_GENERATED_DIR}/Plugin.Stage23.cpp")

add_custom_command(
  OUTPUT "${ATLAS_GENERATED_SOURCE}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${ATLAS_GENERATED_DIR}"
  COMMAND "${Python3_EXECUTABLE}"
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_stage23.py"
          "${CMAKE_CURRENT_SOURCE_DIR}/src/Plugin.cpp"
          "${ATLAS_GENERATED_SOURCE}"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Plugin.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/apply_stage23.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_stage23.py"
  VERBATIM
)

add_library(AtlasRuntime SHARED "${ATLAS_GENERATED_SOURCE}")

target_include_directories(AtlasRuntime PRIVATE "${XNVSE_SDK}")
target_compile_definitions(AtlasRuntime PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(AtlasRuntime PRIVATE shell32)

set_target_properties(AtlasRuntime PROPERTIES
  OUTPUT_NAME "AtlasRuntime"
  PREFIX ""
)
