# CMakeList.txt : CMake project for SDL3 GPU, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (CMakeTarget "main.cpp")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CMakeTarget PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

target_include_directories(CMakeTarget PRIVATE "C:/DEV/SDL/SDL3/SDL3 VC/include")
target_include_directories(CMakeTarget PRIVATE "C:/DEV/SDL/SDL_image/SDL3_image VC/include")
target_include_directories(CMakeTarget PRIVATE "external")
target_link_libraries(CMakeTarget PRIVATE "C:/DEV/SDL/SDL3/SDL3 VC/lib/x64/SDL3.lib")
target_link_libraries(CMakeTarget PRIVATE "C:/DEV/SDL/SDL_image/SDL3_image VC/lib/x64/SDL3_image.lib")