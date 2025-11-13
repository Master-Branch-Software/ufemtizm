cmake_minimum_required(VERSION 3.16)

project(
  UnfuckMyTimeZoneMath
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Configure version header
configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_BINARY_DIR}/version.h
               @ONLY)

if(WIN32)
  add_executable(
    UnfuckMyTimeZoneMath WIN32 main.cpp mainwindow.h mainwindow.cpp
                               timezonewidget.h timezonewidget.cpp)
elseif(APPLE)
  add_executable(
    UnfuckMyTimeZoneMath MACOSX_BUNDLE main.cpp mainwindow.h mainwindow.cpp
                                       timezonewidget.h timezonewidget.cpp)

  set_target_properties(
    UnfuckMyTimeZoneMath
    PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Unfuck My TimeZone Math"
               MACOSX_BUNDLE_GUI_IDENTIFIER "com.ows.unfuckmytimezonemath"
               MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
               MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
               MACOSX_BUNDLE_INFO_STRING
               "A Qt-based timezone conversion utility")
else()
  add_executable(UnfuckMyTimeZoneMath main.cpp mainwindow.h mainwindow.cpp
                                      timezonewidget.h timezonewidget.cpp)
endif()

target_include_directories(UnfuckMyTimeZoneMath PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(UnfuckMyTimeZoneMath PRIVATE Qt6::Core Qt6::Widgets)

# Install target
install(
  TARGETS UnfuckMyTimeZoneMath
  RUNTIME DESTINATION bin
  BUNDLE DESTINATION .)

# Bundle Qt runtime into the install tree on Windows so CPack includes it
if(WIN32)
  # Try to locate windeployqt (relative to Qt6_DIR is typical)
  find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt6
    HINTS "${Qt6_DIR}/../../../bin"
  )
  if(WINDEPLOYQT_EXECUTABLE)
    # Run windeployqt at install time to deploy Qt DLLs/plugins into bin directory
    install(CODE "
      set(WINDEPLOYQT_EXECUTABLE \"${WINDEPLOYQT_EXECUTABLE}\")
      execute_process(
        COMMAND \"\${WINDEPLOYQT_EXECUTABLE}\" --release --dir \"\${CMAKE_INSTALL_PREFIX}/bin\" \"\${CMAKE_INSTALL_PREFIX}/bin/UnfuckMyTimeZoneMath.exe\"
        RESULT_VARIABLE WINDEPLOYQT_RESULT
      )
      if(NOT WINDEPLOYQT_RESULT EQUAL 0)
        message(WARNING \"windeployqt failed with result: \${WINDEPLOYQT_RESULT}\")
      endif()
    ")
  else()
    message(WARNING "windeployqt not found. Qt runtime will not be bundled by CPack.")
  endif()
endif()

# Install desktop file and icon for Linux
if(UNIX AND NOT APPLE)
  install(FILES ${CMAKE_SOURCE_DIR}/unfuck-my-timezone-math.desktop
          DESTINATION share/applications)
  install(
    FILES ${CMAKE_SOURCE_DIR}/icon.png
    DESTINATION share/icons/hicolor/256x256/apps
    RENAME unfuck-my-timezone-math.png)
  # Install smaller icon sizes for better system integration
  install(
    FILES ${CMAKE_SOURCE_DIR}/icon.png
    DESTINATION share/icons/hicolor/128x128/apps
    RENAME unfuck-my-timezone-math.png)
  install(
    FILES ${CMAKE_SOURCE_DIR}/icon.png
    DESTINATION share/icons/hicolor/48x48/apps
    RENAME unfuck-my-timezone-math.png)
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "unfuck-my-timezone-math")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "Ray")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A Qt-based timezone conversion utility")
set(CPACK_PACKAGE_DESCRIPTION
    "UnfuckMyTimeZoneMath is a Qt6-based application for converting times between different timezones."
)
set(CPACK_PACKAGE_CONTACT "ray@example.com")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "UnfuckMyTimeZoneMath")

if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()

if(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
  set(CPACK_DMG_VOLUME_NAME "Unfuck My TimeZone Math")
  set(CPACK_DMG_FORMAT "UDBZ")
  set(CPACK_DMG_BACKGROUND_IMAGE "")
elseif(WIN32)
  # Use Qt Installer Framework as primary installer, with NSIS and ZIP as fallbacks
  set(CPACK_GENERATOR "IFW;NSIS;ZIP")
  
  # Qt Installer Framework configuration
  set(CPACK_IFW_PACKAGE_NAME "Unfuck My TimeZone Math")
  set(CPACK_IFW_PACKAGE_TITLE "Unfuck My TimeZone Math Installer")
  set(CPACK_IFW_PACKAGE_PUBLISHER "Ray Parker")
  set(CPACK_IFW_PRODUCT_URL "https://github.com/RayParkerBassPlayer/UnfuckMyTimeZoneMath")
  set(CPACK_IFW_PACKAGE_START_MENU_DIRECTORY "Unfuck My TimeZone Math")
  set(CPACK_IFW_TARGET_DIRECTORY "@ApplicationsDir@/UnfuckMyTimeZoneMath")
  set(CPACK_IFW_PACKAGE_RUN_PROGRAM "@TargetDir@/bin/UnfuckMyTimeZoneMath.exe")
  set(CPACK_IFW_PACKAGE_RUN_PROGRAM_DESCRIPTION "Launch Unfuck My TimeZone Math")
  set(CPACK_IFW_VERBOSE ON)
  
  # NSIS configuration (fallback)
  set(CPACK_NSIS_DISPLAY_NAME "Unfuck My TimeZone Math")
  set(CPACK_NSIS_PACKAGE_NAME "UnfuckMyTimeZoneMath")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\UnfuckMyTimeZoneMath.exe")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
  set(CPACK_NSIS_MODIFY_PATH OFF)
  set(CPACK_NSIS_MUI_FINISHPAGE_RUN "UnfuckMyTimeZoneMath.exe")
  set(CPACK_NSIS_CREATE_ICONS_EXTRA
      "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Unfuck My TimeZone Math.lnk' '$INSTDIR\\\\bin\\\\UnfuckMyTimeZoneMath.exe'")
  set(CPACK_NSIS_DELETE_ICONS_EXTRA
      "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Unfuck My TimeZone Math.lnk'")
  set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/RayParkerBassPlayer/UnfuckMyTimeZoneMath")
  set(CPACK_NSIS_CONTACT "ray@example.com")
  set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
  set(CPACK_NSIS_MUI_ICON "")
  set(CPACK_NSIS_MUI_UNIICON "")
else()
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ray")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base, libc6")
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
  set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst")
endif()

include(CPack)
