add_library(colorfy_engine STATIC
    src/ConfigStore.cpp
    src/MonitorManager.cpp
    src/LibraryScanner.cpp
)

target_include_directories(colorfy_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(colorfy_engine PUBLIC
    Qt6::Core
    Qt6::Gui
)
