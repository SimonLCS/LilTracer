vcpkg install^
  glm:x64-windows^
  nlohmann-json:x64-windows^
  embree3:x64-windows^
  glfw3:x64-windows^
  imgui[core,opengl3-binding,glfw-binding]:x64-windows^
  implot:x64-windows^
  miniz:x64-windows^
  glew:x64-windows

vcpkg integrate install

pause