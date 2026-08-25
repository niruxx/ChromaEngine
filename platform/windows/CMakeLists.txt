add_executable(ColorfyEngine WIN32
    main_windows.cpp
    WorkerWHost.cpp
    WallpaperWindow.cpp
    ThumbnailGenerator.cpp
    app.rc
)

target_link_libraries(ColorfyEngine PRIVATE
    colorfy_engine
    colorfy_render
    colorfy_ui
    Qt6::Widgets
    user32
    shell32
)

if(MSVC)
    # app.rc supplies our own DPI-awareness manifest at resource ID 1;
    # without this, MSVC's linker embeds a second, conflicting one at the
    # same ID and the build fails with "duplicate resource".
    set_target_properties(ColorfyEngine PROPERTIES LINK_FLAGS "/MANIFEST:NO")
endif()

if(EXISTS "${MPV_DIR}/mpv-2.dll")
    set(MPV_RUNTIME_DLL "${MPV_DIR}/mpv-2.dll")
elseif(EXISTS "${MPV_DIR}/libmpv-2.dll")
    set(MPV_RUNTIME_DLL "${MPV_DIR}/libmpv-2.dll")
endif()

if(MPV_RUNTIME_DLL)
    add_custom_command(TARGET ColorfyEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MPV_RUNTIME_DLL}" $<TARGET_FILE_DIR:ColorfyEngine>
        COMMENT "Copying ${MPV_RUNTIME_DLL} next to ColorfyEngine.exe"
    )
endif()
