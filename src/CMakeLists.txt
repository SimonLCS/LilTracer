set(PROGRAM_NAME lil_tracer_lib)

set(LIL_TRACER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lt" PARENT_SCOPE)

include_directories(${LIL_TRACER_INCLUDE_DIR})

file(GLOB cpp_h_files
     "lt/*.h"
     "lt/*.cpp"
     "lt/brdf/*.h"
     "lt/brdf/*.cpp"
)


add_library(${PROGRAM_NAME}  STATIC ${cpp_h_files} )
target_include_directories(${PROGRAM_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" ../3rd_party)


add_library(fast_obj_lib STATIC ../3rd_party/fast_obj/fast_obj.c ../3rd_party/fast_obj/fast_obj.h)
target_link_libraries(${PROGRAM_NAME}  PRIVATE fast_obj_lib)

add_library(tiny_exr_lib STATIC ../3rd_party/tiny_exr/tinyexr.cc ../3rd_party/tiny_exr/tinyexr.h)
find_package(miniz CONFIG REQUIRED)
target_link_libraries(tiny_exr_lib PRIVATE miniz::miniz)
target_link_libraries(${PROGRAM_NAME}  PRIVATE tiny_exr_lib)

find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROGRAM_NAME} PUBLIC glm::glm)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROGRAM_NAME}  PRIVATE nlohmann_json nlohmann_json::nlohmann_json)

find_package(embree 3 CONFIG REQUIRED)
target_link_libraries(${PROGRAM_NAME} PRIVATE embree)

add_library(stb_image_lib STATIC ../3rd_party/stb_image/stb_image.cpp ../3rd_party/stb_image/stb_image.h)
target_link_libraries(${PROGRAM_NAME} PRIVATE stb_image_lib)
