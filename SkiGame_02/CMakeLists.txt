cmake_minimum_required(VERSION 3.19)
project(skiing LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)

qt_standard_project_setup()

qt_add_executable(skiing
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    gamewidget.cpp
    gamewidget.h
    player.cpp
    player.h
    powerup.cpp
    powerup.h
    penguinpowerup.cpp
    penguinpowerup.h
    obstacle.cpp
    obstacle.h
    snowmanhunter.cpp
    snowmanhunter.h
    avalanche.cpp
    avalanche.h
)

target_link_libraries(skiing
    PRIVATE
        Qt::Core
        Qt::Multimedia
        Qt::Widgets
)

target_compile_definitions(skiing
    PRIVATE
        SKIING_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

install(TARGETS skiing
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET skiing
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
