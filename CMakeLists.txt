cmake_minimum_required(VERSION 3.16)

project(UnfuckMyTimeZoneMath VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(UnfuckMyTimeZoneMath
    main.cpp
    mainwindow.h
    mainwindow.cpp
    timezonewidget.h
    timezonewidget.cpp
)

target_link_libraries(UnfuckMyTimeZoneMath PRIVATE Qt6::Core Qt6::Widgets)
