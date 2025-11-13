cmake_minimum_required(VERSION 3.16)
project(UnfuckMyTimeZoneMath VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()
configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_BINARY_DIR}/version.h
               @ONLY)
if(WIN32)
  add_executable(
    UnfuckMyTimeZoneMath WIN32 main.cpp mainwindow.h mainwindow.cpp
                               timezonewidget.h timezonewidget.cpp
                               resources.qrc)
elseif(APPLE)
  add_executable(
    UnfuckMyTimeZoneMath MACOSX_BUNDLE main.cpp mainwindow.h mainwindow.cpp
                                       timezonewidget.h timezonewidget.cpp)
  set_target_properties(UnfuckMyTimeZoneMath
    PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Unfuck My Time Zone Math"
               OUTPUT_NAME "Unfuck My Time Zone Math"
               MACOSX_BUNDLE_GUI_IDENTIFIER "com.ows.unfuckmytimezonemath"
               MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
               MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
               MACOSX_BUNDLE_INFO_STRING
               "A Qt-based timezone conversion utility")
else()
  add_executable(UnfuckMyTimeZoneMath main.cpp mainwindow.h mainwindow.cpp
                                      timezonewidget.h timezonewidget.cpp
                                      resources.qrc)
endif()


target_include_directories(UnfuckMyTimeZoneMath PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(UnfuckMyTimeZoneMath PRIVATE Qt6::Core Qt6::Widgets)

if(UNIX AND NOT APPLE)
  set_target_properties(UnfuckMyTimeZoneMath PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib/UnfuckMyTimeZoneMath")
endif()


if(UNIX AND NOT APPLE)
  install(FILES ${CMAKE_SOURCE_DIR}/unfuck-my-timezone-math.desktop
          DESTINATION share/applications
          COMPONENT ${COMPONENT_NAME_MAIN})
  install(FILES ${CMAKE_SOURCE_DIR}/icon.svg
          DESTINATION share/icons/hicolor/scalable/apps
          RENAME unfuck-my-timezone-math.svg
          COMPONENT ${COMPONENT_NAME_MAIN})
endif()
if(APPLE)
  if(EXISTS "${CMAKE_SOURCE_DIR}/icon.icns")
    set_target_properties(UnfuckMyTimeZoneMath PROPERTIES MACOSX_BUNDLE_ICON_FILE "icon.icns")
    install(FILES ${CMAKE_SOURCE_DIR}/icon.icns DESTINATION UnfuckMyTimeZoneMath.app/Contents/Resources COMPONENT unfuckmytimezonemath)
  endif()
endif()


include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)

set(COMPONENT_NAME_MAIN "unfuckmytimezonemath")
set(CONF_DISPLAY_NAME "Unfuck My TimeZone Math")
set(CONF_PUBLISHER "Ray Parker & Vojtěch Gistr")
set(CONF_DESCRIPTION "A Qt-based timezone conversion utility")

set(CPACK_PACKAGE_NAME "UnfuckMyTimeZoneMath")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "${CONF_PUBLISHER}")
set(CPACK_PACKAGE_DESCRIPTION "${CONF_DESCRIPTION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "UnfuckMyTimeZoneMath")
set(CPACK_COMPONENTS_ALL ${COMPONENT_NAME_MAIN})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if(UNIX AND NOT APPLE)
  # On Linux: install binary to lib directory and wrapper to bin
  install(TARGETS UnfuckMyTimeZoneMath
      RUNTIME DESTINATION lib/UnfuckMyTimeZoneMath
      COMPONENT ${COMPONENT_NAME_MAIN})
  install(PROGRAMS unfuckmytimezonemath-wrapper.sh
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      RENAME UnfuckMyTimeZoneMath
      COMPONENT ${COMPONENT_NAME_MAIN})
else()
  install(TARGETS UnfuckMyTimeZoneMath
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      BUNDLE DESTINATION .
      COMPONENT ${COMPONENT_NAME_MAIN})
endif()


install(FILES ${CMAKE_SOURCE_DIR}/LICENSE
    DESTINATION share/doc/${CMAKE_PROJECT_NAME}/licenses
    COMPONENT ${COMPONENT_NAME_MAIN})

if(APPLE)
  # macOS DMG is created via macdeployqt in build_and_deploy.sh; CPack is not used on macOS.

elseif(WIN32)
  file(GLOB_RECURSE _BINARYCREATOR_CANDIDATES
    "C:/Qt/Tools/QtInstallerFramework/*/bin/binarycreator.exe"
    "$ENV{HOME}/Qt/Tools/QtInstallerFramework/*/bin/binarycreator"
    "/opt/Qt/Tools/QtInstallerFramework/*/bin/binarycreator"
    "$ENV{QTIFW_DIR}/*/bin/binarycreator"
    "$ENV{QTDIR}/*/bin/binarycreator"
  )

  list(LENGTH _BINARYCREATOR_CANDIDATES _NUM_FOUND)

  if(_NUM_FOUND GREATER 0)
    list(GET _BINARYCREATOR_CANDIDATES 0 _IFW_BINARYCREATOR)
    get_filename_component(_IFW_BIN_DIR "${_IFW_BINARYCREATOR}" DIRECTORY)
    get_filename_component(CPACK_IFW_ROOT "${_IFW_BIN_DIR}" DIRECTORY)
    set(CPACK_IFW_ROOT "${CPACK_IFW_ROOT}" CACHE PATH "Path to Qt Installer Framework")
  endif()

  install(FILES ${CMAKE_SOURCE_DIR}/icon.ico
              DESTINATION ${CMAKE_INSTALL_BINDIR}
              RENAME UnfuckMyTimeZoneMath.ico
              COMPONENT ${COMPONENT_NAME_MAIN})

  find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt6
    HINTS "${Qt6_DIR}/../../../bin"
  )

  if(WINDEPLOYQT_EXECUTABLE)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/win-deploy.cmake.in" "${CMAKE_CURRENT_SOURCE_DIR}/win-deploy.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/win-deploy.cmake")
  endif()

  set(CPACK_GENERATOR "IFW")

  set(CPACK_IFW_PACKAGE_NAME "${CONF_DISPLAY_NAME}")
  set(CPACK_IFW_PACKAGE_TITLE "${CONF_DISPLAY_NAME}")
  set(CPACK_IFW_PACKAGE_PUBLISHER "${CONF_PUBLISHER}")
  set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Classic")
  set(CPACK_IFW_PACKAGE_WIZARD_SHOW_PAGE_LIST OFF)
  set(CPACK_IFW_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/icon.png")
  set(CPACK_IFW_DOWNLOAD_ALL OFF)

  include(CPack)
  include(CPackIFW)

  cpack_add_component(${COMPONENT_NAME_MAIN}
    DISPLAY_NAME "${CONF_DISPLAY_NAME}"
    DESCRIPTION "${CONF_DESCRIPTION}"
    REQUIRED
  )

  cpack_ifw_configure_component(${COMPONENT_NAME_MAIN}
    ESSENTIAL
    FORCED_INSTALLATION
    NAME "${COMPONENT_NAME_MAIN}"
    DESCRIPTION "${CONF_DESCRIPTION}"
    SCRIPT "${CMAKE_SOURCE_DIR}/installer/installscript.qs"
    LICENSES "License" ${CPACK_RESOURCE_FILE_LICENSE}
    DEFAULT TRUE
  )


else()
  # Linux: Use CPack to create DEB package with bundled Qt libraries
  
  # Install Qt libraries from qt_deploy directory (created by build script)
  install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_deploy/lib/"
          DESTINATION "lib/UnfuckMyTimeZoneMath"
          COMPONENT ${COMPONENT_NAME_MAIN}
          OPTIONAL
          FILES_MATCHING PATTERN "*.so*")
  install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_deploy/plugins/"
          DESTINATION "lib/UnfuckMyTimeZoneMath/plugins"
          COMPONENT ${COMPONENT_NAME_MAIN}
          OPTIONAL
          FILES_MATCHING PATTERN "*.so")
  
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ray")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libgcc-s1")
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
  set(CPACK_SET_DESTDIR ON)
  set(CPACK_INSTALL_PREFIX "/usr")
  
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/postinst")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst")
  endif()

  include(CPack)
endif()
