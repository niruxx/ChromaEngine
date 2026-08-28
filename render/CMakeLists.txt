add_library(colorfy_render STATIC
    MpvSurface.cpp
)

target_include_directories(colorfy_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(colorfy_render PUBLIC
    colorfy_engine
    Qt6::Core
)

if(WIN32)
    # PUBLIC: platform/windows/ThumbnailGenerator.cpp also talks to libmpv
    # directly (for hidden-window frame grabs) and needs these transitively.
    target_include_directories(colorfy_render PUBLIC "${MPV_DIR}/include")

    if(EXISTS "${MPV_DIR}/mpv.lib")
        target_link_libraries(colorfy_render PUBLIC "${MPV_DIR}/mpv.lib")
    elseif(EXISTS "${MPV_DIR}/libmpv.dll.a")
        target_link_libraries(colorfy_render PUBLIC "${MPV_DIR}/libmpv.dll.a")
    else()
        message(FATAL_ERROR "Could not find mpv.lib or libmpv.dll.a in ${MPV_DIR} - see third_party/README.md")
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux distros package libmpv's headers/pkg-config file directly (e.g.
    # libmpv-dev on Debian/Ubuntu, mpv-libs-devel on Fedora, mpv on Arch) -
    # no manual download step needed like on Windows.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

    # PUBLIC: platform/linux/ThumbnailGenerator.cpp also talks to libmpv
    # directly, same as the Windows equivalent above.
    target_link_libraries(colorfy_render PUBLIC PkgConfig::MPV)
endif()
