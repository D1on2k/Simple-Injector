cmake_minimum_required(VERSION 3.10)
project(DInjector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB IMGUI_CORE 
    "ImGui/*.cpp"
)

file(GLOB IMGUI_BACKEND 
    "ImGui/backends/imgui_impl_win32.cpp"
)

add_library(InjectorDLL SHARED
    dllTest.cpp
    ${IMGUI_CORE}
    ${IMGUI_BACKEND}
)

set_target_properties(InjectorDLL PROPERTIES
    OUTPUT_NAME "Injector"
    PREFIX ""
)

target_include_directories(InjectorDLL PRIVATE
    ${CMAKE_SOURCE_DIR}/ImGui
    ${CMAKE_SOURCE_DIR}/ImGui/backends
)

add_executable(injector
    Injector.cpp
)

target_link_libraries(injector PRIVATE InjectorDLL d3d11 d3dcompiler dwmapi)

target_include_directories(injector PRIVATE
    ${CMAKE_SOURCE_DIR}/ImGui
    ${CMAKE_SOURCE_DIR}/ImGui/backends
)