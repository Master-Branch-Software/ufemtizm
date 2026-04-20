cmake_minimum_required(VERSION 3.16)
project(
  UnfuckMyTimeZoneMath
  VERSION 2025.1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Svg)
qt_standard_project_setup()

configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_BINARY_DIR}/version.h
               @ONLY)

# Package metadata
set(COMPONENT_NAME_MAIN "unfuckmytimezonemath")
set(CONF_DISPLAY_NAME "Unfuck My TimeZone Math")
set(CONF_PUBLISHER "Ray Parker & Vojtěch Gistr")
set(CONF_DESCRIPTION "A Qt-based timezone conversion utility")

# Create executable with platform-specific flags
if(WIN32)
  add_executable(
    UnfuckMyTimeZoneMath WIN32
    main.cpp
    mainwindow.hpp
    mainwindow.cpp
    timezonewidget.hpp
    timezonewidget.cpp
    settingsdialog.hpp
    settingsdialog.cpp
    copydialog.hpp
    copydialog.cpp
    resources.qrc)
elseif(APPLE)
  add_executable(
    UnfuckMyTimeZoneMath MACOSX_BUNDLE
    main.cpp
    mainwindow.hpp
    mainwindow.cpp
    timezonewidget.hpp
    timezonewidget.cpp
    settingsdialog.hpp
    settingsdialog.cpp
    copydialog.hpp
    copydialog.cpp
    macos_helper.hpp
    macos_helper.mm
    resources.qrc)
  set_target_properties(
    UnfuckMyTimeZoneMath
    PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "Unfuck My Time Zone Math"
               OUTPUT_NAME "Unfuck My Time Zone Math"
               MACOSX_BUNDLE_GUI_IDENTIFIER "com.ows.unfuckmytimezonemath"
               MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
               MACOSX_BUNDLE_SHORT_VERSION_STRING
               "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
               MACOSX_BUNDLE_INFO_STRING "${CONF_DESCRIPTION}"
               MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist.in")
else()
  add_executable(
    UnfuckMyTimeZoneMath
    main.cpp
    mainwindow.hpp
    mainwindow.cpp
    timezonewidget.hpp
    timezonewidget.cpp
    settingsdialog.hpp
    settingsdialog.cpp
    copydialog.hpp
    copydialog.cpp
    resources.qrc)
endif()

target_include_directories(UnfuckMyTimeZoneMath PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(UnfuckMyTimeZoneMath PRIVATE Qt6::Core Qt6::Widgets
                                                   Qt6::Network Qt6::Svg)

if(APPLE)
  target_link_libraries(UnfuckMyTimeZoneMath PRIVATE "-framework AppKit")
endif()

if(UNIX AND NOT APPLE)
  set_target_properties(
    UnfuckMyTimeZoneMath PROPERTIES INSTALL_RPATH
                                    "$ORIGIN/../lib/UnfuckMyTimeZoneMath")
endif()

include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)

# Platform-specific installation
if(UNIX AND NOT APPLE)
  set(CMAKE_INSTALL_PREFIX
      "/usr"
      CACHE PATH "Install path prefix" FORCE)

  install(
    FILES ${CMAKE_SOURCE_DIR}/unfuck-my-timezone-math.desktop
    DESTINATION share/applications
    COMPONENT ${COMPONENT_NAME_MAIN})

  install(
    FILES ${CMAKE_SOURCE_DIR}/icon.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME unfuck-my-timezone-math.svg
    COMPONENT ${COMPONENT_NAME_MAIN})
elseif(APPLE)
  if(EXISTS "${CMAKE_SOURCE_DIR}/icon.icns")
    set_target_properties(UnfuckMyTimeZoneMath
                          PROPERTIES MACOSX_BUNDLE_ICON_FILE "icon.icns")

    install(
      FILES ${CMAKE_SOURCE_DIR}/icon.icns
      DESTINATION UnfuckMyTimeZoneMath.app/Contents/Resources
      COMPONENT unfuckmytimezonemath)
  endif()
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "UnfuckMyTimeZoneMath")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "${CONF_PUBLISHER}")
set(CPACK_PACKAGE_DESCRIPTION "${CONF_DESCRIPTION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "UnfuckMyTimeZoneMath")
set(CPACK_COMPONENTS_ALL ${COMPONENT_NAME_MAIN})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Binary installation
if(UNIX AND NOT APPLE)
  install(TARGETS UnfuckMyTimeZoneMath
          RUNTIME DESTINATION lib/UnfuckMyTimeZoneMath
                  COMPONENT ${COMPONENT_NAME_MAIN})

  install(
    PROGRAMS unfuckmytimezonemath-wrapper.sh
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RENAME UnfuckMyTimeZoneMath
    COMPONENT ${COMPONENT_NAME_MAIN})
else()
  install(
    TARGETS UnfuckMyTimeZoneMath
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION . COMPONENT ${COMPONENT_NAME_MAIN})
endif()

install(
  FILES ${CMAKE_SOURCE_DIR}/LICENSE
  DESTINATION share/doc/${CMAKE_PROJECT_NAME}/licenses
  COMPONENT ${COMPONENT_NAME_MAIN})

# Platform-specific packaging
if(APPLE)
  # macOS: DMG is created via macdeployqt in build_and_deploy.sh

elseif(WIN32)
  # Windows: Qt Installer Framework
  file(
    GLOB_RECURSE
    _BINARYCREATOR_CANDIDATES
    "C:/Qt/Tools/QtInstallerFramework/*/bin/binarycreator.exe"
    "$ENV{HOME}/Qt/Tools/QtInstallerFramework/*/bin/binarycreator"
    "/opt/Qt/Tools/QtInstallerFramework/*/bin/binarycreator"
    "$ENV{QTIFW_DIR}/*/bin/binarycreator"
    "$ENV{QTDIR}/*/bin/binarycreator")

  list(LENGTH _BINARYCREATOR_CANDIDATES _NUM_FOUND)
  if(_NUM_FOUND GREATER 0)
    list(GET _BINARYCREATOR_CANDIDATES 0 _IFW_BINARYCREATOR)
    get_filename_component(_IFW_BIN_DIR "${_IFW_BINARYCREATOR}" DIRECTORY)
    get_filename_component(CPACK_IFW_ROOT "${_IFW_BIN_DIR}" DIRECTORY)
    set(CPACK_IFW_ROOT
        "${CPACK_IFW_ROOT}"
        CACHE PATH "Path to Qt Installer Framework")
  endif()

  install(
    FILES ${CMAKE_SOURCE_DIR}/icon.ico
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RENAME UnfuckMyTimeZoneMath.ico
    COMPONENT ${COMPONENT_NAME_MAIN})

  find_program(
    WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt6
    HINTS "${Qt6_DIR}/../../../bin")

  if(WINDEPLOYQT_EXECUTABLE)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/win-deploy.cmake.in"
                   "${CMAKE_CURRENT_SOURCE_DIR}/win-deploy.cmake" @ONLY)
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

  cpack_add_component(
    ${COMPONENT_NAME_MAIN}
    DISPLAY_NAME "${CONF_DISPLAY_NAME}"
    DESCRIPTION "${CONF_DESCRIPTION}"
    REQUIRED)

  cpack_ifw_configure_component(
    ${COMPONENT_NAME_MAIN} ESSENTIAL FORCED_INSTALLATION
    NAME "${COMPONENT_NAME_MAIN}"
    DESCRIPTION "${CONF_DESCRIPTION}"
    SCRIPT "${CMAKE_SOURCE_DIR}/installer/installscript.qs"
    LICENSES "License" ${CPACK_RESOURCE_FILE_LICENSE}
    DEFAULT TRUE)

else()
  # Linux: bundle Qt libraries by deriving paths from linked Qt targets.
  # Any Qt module added to target_link_libraries is picked up automatically.
  get_target_property(_qt_core_loc Qt6::Core LOCATION)
  get_filename_component(_qt_lib_dir "${_qt_core_loc}" DIRECTORY)
  get_filename_component(_qt_root "${_qt_lib_dir}" DIRECTORY)
  set(_qt_plugins_dir "${_qt_root}/plugins")

  foreach(_qt_target Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::DBus
                     Qt6::Svg Qt6::XcbQpa Qt6::OpenGL)
    if(TARGET ${_qt_target})
      get_target_property(_lib_loc ${_qt_target} LOCATION)
      get_filename_component(_lib_dir "${_lib_loc}" DIRECTORY)
      get_filename_component(_lib_name "${_lib_loc}" NAME_WE)
      file(GLOB _lib_files "${_lib_dir}/${_lib_name}.so*")
      install(FILES ${_lib_files}
              DESTINATION "lib/UnfuckMyTimeZoneMath"
              COMPONENT ${COMPONENT_NAME_MAIN})
    endif()
  endforeach()

  # ICU is a hard dependency of Qt Core
  file(GLOB _icu_files "${_qt_lib_dir}/libicu*.so*")
  install(FILES ${_icu_files}
          DESTINATION "lib/UnfuckMyTimeZoneMath"
          COMPONENT ${COMPONENT_NAME_MAIN}
          OPTIONAL)

  # Bundle the xcb/xkb helper libraries that the Qt xcb platform plugin
  # needs at runtime. This keeps the .deb self-contained so users do not
  # have to apt-install libxcb-cursor0 et al on a fresh machine
  # (libxcb-cursor0 became a hard runtime requirement in Qt 6.5).
  #
  # Libraries are searched on the build host in the usual multi-arch paths.
  # install(... OPTIONAL) means missing libs are tolerated silently; at least
  # libxcb-cursor is required for the plugin to load.
  set(_bundled_xcb_libs
      libxcb-cursor.so.0
      libxkbcommon-x11.so.0
      libxkbcommon.so.0
      libxcb-icccm.so.4
      libxcb-image.so.0
      libxcb-keysyms.so.1
      libxcb-randr.so.0
      libxcb-render-util.so.0
      libxcb-render.so.0
      libxcb-shape.so.0
      libxcb-shm.so.0
      libxcb-sync.so.1
      libxcb-util.so.1
      libxcb-xfixes.so.0
      libxcb-xinerama.so.0
      libxcb-xkb.so.1)

  set(_xcb_search_paths
      "/usr/lib/x86_64-linux-gnu"
      "/lib/x86_64-linux-gnu"
      "/usr/lib64"
      "/usr/lib")

  foreach(_lib_name ${_bundled_xcb_libs})
    set(_resolved_lib "")
    foreach(_search_path ${_xcb_search_paths})
      if(EXISTS "${_search_path}/${_lib_name}")
        set(_resolved_lib "${_search_path}/${_lib_name}")
        break()
      endif()
    endforeach()

    if(_resolved_lib)
      # Follow symlinks so the real .so file ends up in the package.
      get_filename_component(_real_lib "${_resolved_lib}" REALPATH)
      install(FILES "${_real_lib}"
              DESTINATION "lib/UnfuckMyTimeZoneMath"
              RENAME "${_lib_name}"
              COMPONENT ${COMPONENT_NAME_MAIN})
    else()
      message(WARNING "Could not locate ${_lib_name} to bundle; the installed "
                      ".deb may fail to start on systems lacking it.")
    endif()
  endforeach()

  # Essential Qt plugins
  install(FILES "${_qt_plugins_dir}/platforms/libqxcb.so"
          DESTINATION "lib/UnfuckMyTimeZoneMath/plugins/platforms"
          COMPONENT ${COMPONENT_NAME_MAIN} OPTIONAL)

  install(FILES "${_qt_plugins_dir}/imageformats/libqsvg.so"
          DESTINATION "lib/UnfuckMyTimeZoneMath/plugins/imageformats"
          COMPONENT ${COMPONENT_NAME_MAIN} OPTIONAL)

  install(FILES "${_qt_plugins_dir}/iconengines/libqsvgicon.so"
          DESTINATION "lib/UnfuckMyTimeZoneMath/plugins/iconengines"
          COMPONENT ${COMPONENT_NAME_MAIN} OPTIONAL)

  install(FILES "${_qt_plugins_dir}/platformthemes/libqxdgdesktopportal.so"
          DESTINATION "lib/UnfuckMyTimeZoneMath/plugins/platformthemes"
          COMPONENT ${COMPONENT_NAME_MAIN} OPTIONAL)

  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ray")
  # Runtime dependencies. Qt and the xcb helper libs the Qt xcb plugin needs
  # are bundled under /usr/lib/UnfuckMyTimeZoneMath (see above), so only the
  # core system libraries are declared here.
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6, libgcc-s1")
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
  set(CPACK_SET_DESTDIR ON)
  set(CPACK_INSTALL_PREFIX "/usr")

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/postinst")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_CURRENT_SOURCE_DIR}/postinst")
  endif()

  include(CPack)
endif()
