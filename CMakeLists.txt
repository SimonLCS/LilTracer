

cmake_minimum_required(VERSION 3.9)

project(LilTracer 
  VERSION 1.0.0
  LANGUAGES CXX C
)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH           ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(clang_format)

# ----------------------------------------------------------------------------
# LilTracer library
# ----------------------------------------------------------------------------

add_subdirectory(src)
add_clang_format_target(format-liltracer src)

# ----------------------------------------------------------------------------
# Applications
# ----------------------------------------------------------------------------

add_subdirectory(apps/lil_viewer)
add_subdirectory(apps/lil_tracer)
add_subdirectory(apps/envmap_sampling)

# ----------------------------------------------------------------------------
# Documentation
# ----------------------------------------------------------------------------

add_subdirectory(doc)