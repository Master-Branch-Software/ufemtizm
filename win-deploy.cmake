set(WINDEPLOYQT "C:/Qt/6.9.1/mingw_64/bin/windeployqt.exe")
set(COMPONENT_NAME_MAIN "unfuckmytimezonemath")
set(CMAKE_PROJECT_NAME "UnfuckMyTimeZoneMath")
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_BINARY_DIR "C:/Users/vojti/Documents/UnfuckMyTimeZoneMath/build")
message("Running deploy script...")
set(PACKAGE_BIN_DIR "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/packages/${COMPONENT_NAME_MAIN}/data/bin/")
file(COPY "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.exe" DESTINATION "${PACKAGE_BIN_DIR}")
message(STATUS "Copied EXE to ${PACKAGE_BIN_DIR}")
execute_process(
    COMMAND ${WINDEPLOYQT} --no-translations --compiler-runtime --release "${CMAKE_PROJECT_NAME}.exe"
    WORKING_DIRECTORY "${PACKAGE_BIN_DIR}"
    RESULT_VARIABLE WINDEPLOYQT_RESULT
)
if(NOT WINDEPLOYQT_RESULT EQUAL 0)
    message(WARNING "windeployqt failed with result: ${WINDEPLOYQT_RESULT}")
endif()
