find_package(X11 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS DBus)

add_executable(ChromaEngine
    main_linux.cpp
    X11WallpaperWindow.cpp
    DesktopOverlayWindow.cpp
    ThumbnailGenerator.cpp
    StartupManager.cpp
    MemoryLimiter.cpp
    BluetoothBatteryReader.cpp
)

target_include_directories(ChromaEngine PRIVATE ${X11_INCLUDE_DIR})

target_link_libraries(ChromaEngine PRIVATE
    colorfy_engine
    colorfy_render
    colorfy_ui
    Qt6::Widgets
    Qt6::DBus
    ${X11_LIBRARIES}
)

install(TARGETS ChromaEngine RUNTIME DESTINATION bin)
install(FILES chroma-engine.desktop DESTINATION share/applications)
