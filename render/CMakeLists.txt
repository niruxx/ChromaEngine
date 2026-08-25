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
endif()
