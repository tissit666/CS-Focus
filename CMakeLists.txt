cmake_minimum_required(VERSION 3.14)
project(CS2FocusGuard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep EUI-NEO as the UI/runtime dependency; its demos are not part of this app.
set(EUI_BUILD_APPS OFF CACHE BOOL "" FORCE)
set(EUI_BUILD_USER_APPS OFF CACHE BOOL "" FORCE)
set(EUI_BUILD_TEST_FIXTURES OFF CACHE BOOL "" FORCE)
set(EUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(EUI_ENABLE_TRAY OFF CACHE BOOL "" FORCE)
set(EUI_WINDOW_BACKEND "glfw" CACHE STRING "" FORCE)
set(EUI_RENDER_BACKEND "opengl" CACHE STRING "" FORCE)
add_subdirectory(third_party/EUI-NEO EXCLUDE_FROM_ALL)

add_executable(cs2_focus_guard
    src/main.cpp
    src/process_monitor.cpp
)
target_include_directories(cs2_focus_guard PRIVATE src)
target_compile_definitions(cs2_focus_guard PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(cs2_focus_guard PRIVATE $<$<CXX_COMPILER_ID:GNU>:-finput-charset=UTF-8>)
eui_neo_configure_app(cs2_focus_guard)