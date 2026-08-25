add_library(colorfy_ui STATIC
    SettingsWindow.cpp
    TrayIcon.cpp
    LibraryItemDelegate.cpp
)

target_include_directories(colorfy_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(colorfy_ui PUBLIC
    colorfy_engine
    Qt6::Widgets
)
