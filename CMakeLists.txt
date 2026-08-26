cmake_minimum_required(VERSION 3.21)
project(ChromaEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

if(WIN32)
    set(MPV_DIR "" CACHE PATH "Path to the libmpv dev package (see third_party/README.md)")
    if(MPV_DIR STREQUAL "")
        message(FATAL_ERROR "MPV_DIR is not set. Pass -DMPV_DIR=<path to libmpv dev package>; see third_party/README.md")
    endif()
endif()

add_subdirectory(engine)
add_subdirectory(render)
add_subdirectory(ui)

if(WIN32)
    add_subdirectory(platform/windows)
endif()
