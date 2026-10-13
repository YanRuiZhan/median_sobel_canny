cmake_minimum_required(VERSION 3.16)

project(assignment_ws
        VERSION 0.1.0
        LANGUAGES CXX)

message(STATUS "项目根目录: ${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 未指定构建类型时默认 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "构建类型" FORCE)
endif()
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")

# OpenCV
#find_package(OpenCV 4 REQUIRED)
#message(STATUS "OpenCV 版本: ${OpenCV_VERSION}")
#message(STATUS "OpenCV 头文件: ${OpenCV_INCLUDE_DIRS}")

# 图像处理实现由命令行程序和 Qt 界面共享。
add_library(icm
    src/convolution.cpp
    src/kernels.cpp
    src/image_io.cpp
    src/median_blur.cpp
    src/sobel.cpp
    src/canny.cpp
)

target_include_directories(icm PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(icm SYSTEM PRIVATE
    ${PROJECT_SOURCE_DIR}/third_party
)

target_compile_options(icm PRIVATE
    -Wall
    -Wextra
)

# 保留现有命令行程序。
add_executable(final src/main.cpp)
target_link_libraries(final PRIVATE icm)

# Qt 界面作为独立可选目标，命令行构建无需安装 Qt。
option(ICM_BUILD_GUI "Build the Qt Widgets interface when Qt is available" ON)
if(ICM_BUILD_GUI)
    find_package(Qt5 COMPONENTS Widgets Concurrent QUIET)
endif()
if(ICM_BUILD_GUI AND Qt5Widgets_FOUND AND Qt5Concurrent_FOUND)
    set(CMAKE_AUTOMOC ON)
    add_executable(icm_gui src/gui_main.cpp src/main_window.cpp src/main_window.hpp)
    target_link_libraries(icm_gui PRIVATE icm Qt5::Widgets Qt5::Concurrent)
    set_target_properties(icm_gui PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
else()
    message(STATUS "Qt5 Widgets/Concurrent not found or GUI disabled; skipping icm_gui target")
endif()

foreach(target icm final)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
endforeach()
