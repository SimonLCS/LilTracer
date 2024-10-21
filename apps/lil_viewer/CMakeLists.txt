set(PROGRAM_NAME lil_viewer)

add_executable(${PROGRAM_NAME} main.cpp lil_gl.h)

target_link_libraries(${PROGRAM_NAME} PRIVATE lil_tracer_lib)

find_package(OpenMP REQUIRED)
target_link_libraries(${PROGRAM_NAME} PRIVATE OpenMP::OpenMP_CXX)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROGRAM_NAME} PRIVATE glfw)

find_package(GLEW CONFIG REQUIRED)
target_link_libraries(${PROGRAM_NAME} PRIVATE GLEW::GLEW)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${PROGRAM_NAME} PRIVATE imgui::imgui)

find_package(implot CONFIG REQUIRED)
target_link_libraries(${PROGRAM_NAME} PRIVATE implot::implot)

include_directories( ../../3rd_party/imgui_style )