cmake_minimum_required(VERSION 3.21)
project(AtlasRuntime VERSION 0.3.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED XNVSE_SDK)
  message(FATAL_ERROR "Set XNVSE_SDK to the xNVSE plugin SDK root")
endif()

add_library(AtlasRuntime SHARED src/Plugin.cpp)

target_include_directories(AtlasRuntime PRIVATE "${XNVSE_SDK}")
target_compile_definitions(AtlasRuntime PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(AtlasRuntime PRIVATE shell32)

set_target_properties(AtlasRuntime PROPERTIES
  OUTPUT_NAME "AtlasRuntime"
  PREFIX ""
)
