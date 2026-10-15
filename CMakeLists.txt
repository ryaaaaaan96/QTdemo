cmake_minimum_required(VERSION 3.25)
project(QTdemo VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Qml Quick QuickWidgets)
qt_standard_project_setup()
include(GNUInstallDirs)
add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(TARGETS QTdemo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
qt_generate_deploy_qml_app_script(
    TARGET QTdemo
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
# Qt 6.8 windeployqt does not collect the LLVM-MinGW runtime automatically.
if(WIN32 AND MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    get_filename_component(llvm_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(runtime libc++.dll libunwind.dll libwinpthread-1.dll)
        find_file(runtime_path_${runtime} NAMES ${runtime}
            PATHS "${llvm_bin}/../x86_64-w64-mingw32/bin" "${llvm_bin}"
            NO_DEFAULT_PATH REQUIRED)
        install(FILES "${runtime_path_${runtime}}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endforeach()
endif()
include(cmake/Packaging.cmake)
