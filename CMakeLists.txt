cmake_minimum_required(VERSION 3.16)
project(noughtsandcrosses LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

add_executable(noughtsandcrosses
    main.cpp
    board.h board.cpp
    gamewidget.h gamewidget.cpp
    mainwindow.h mainwindow.cpp
    computerworker.h computerworker.cpp
    aboutdialog.h aboutdialog.cpp
    statisticsdialog.h statisticsdialog.cpp
)

target_link_libraries(noughtsandcrosses PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core)



# Install target
install(TARGETS noughtsandcrosses RUNTIME DESTINATION bin)
