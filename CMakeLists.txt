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

# Install target
install(TARGETS UnfuckMyTimeZoneMath
    RUNTIME DESTINATION bin
)

# Install desktop file
install(FILES unfuck-my-timezone-math.desktop
    DESTINATION share/applications
)

# CPack configuration for .deb package
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "unfuck-my-timezone-math")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Ray")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A Qt-based timezone conversion utility")
set(CPACK_PACKAGE_DESCRIPTION "UnfuckMyTimeZoneMath is a Qt6-based application for converting times between different timezones.")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ray")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base, libc6")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_PACKAGE_CONTACT "ray@example.com")

include(CPack)
