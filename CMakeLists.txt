cmake_minimum_required(VERSION 3.24)
project(ProGen3D LANGUAGES C CXX)

if(NOT WIN32 OR NOT MINGW)
    message(FATAL_ERROR "Use MSYS2 UCRT64 for CMake, or the Makefile for Linux.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)
pkg_check_modules(XML REQUIRED IMPORTED_TARGET libxml-2.0)
pkg_check_modules(EIGEN REQUIRED IMPORTED_TARGET eigen3)
find_path(STB_INCLUDE_DIR stb_image.h PATH_SUFFIXES stb REQUIRED)

# Share the Linux Makefile's editor source list.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Makefile")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Makefile" makefile_contents)
string(REGEX MATCHALL "\\$\\(SRC_DIR\\)/[A-Za-z0-9_./-]+\\.cpp" editor_sources "${makefile_contents}")
list(TRANSFORM editor_sources REPLACE "^\\$\\(SRC_DIR\\)" "src")
if(NOT "src/main.cpp" IN_LIST editor_sources OR NOT "src/imgui_render.cpp" IN_LIST editor_sources)
    message(FATAL_ERROR "Could not read the editor source list from Makefile.")
endif()

set(imgui_dir third_party/vendor/imgui-1.90.1)
set(glad_dir third_party/vendor/glad)
add_executable(progen3d
    ${editor_sources}
    stbimage/stb_image_impl.cpp
    ${glad_dir}/src/gl.c
    ${imgui_dir}/imgui.cpp
    ${imgui_dir}/imgui_draw.cpp
    ${imgui_dir}/imgui_tables.cpp
    ${imgui_dir}/imgui_widgets.cpp
    ${imgui_dir}/backends/imgui_impl_glfw.cpp
    ${imgui_dir}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(progen3d PRIVATE
    include . ${imgui_dir} ${imgui_dir}/backends ${glad_dir}/include "${STB_INCLUDE_DIR}"
)
target_compile_definitions(progen3d PRIVATE
    NOMINMAX WIN32_LEAN_AND_MEAN IMGUI_USER_CONFIG="imgui_user_config.h"
)
# Bound MinGW's optimizer memory use for the large editor translation unit.
set_source_files_properties(src/imgui_main.cpp PROPERTIES COMPILE_OPTIONS "$<$<CONFIG:Release>:-O1>")
# The generated catalog's aggregate initializers exhaust GCC's optimizer on low-memory hosts.
set_source_files_properties(src/building/generated/SmallModernBuildingKnowledgeCatalog.cpp
    PROPERTIES COMPILE_OPTIONS -O0
)
target_link_libraries(progen3d PRIVATE
    glfw OpenSSL::Crypto PkgConfig::CURL PkgConfig::JSONCPP PkgConfig::XML PkgConfig::EIGEN
    comdlg32 shell32 ws2_32
)

get_filename_component(ucrt_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
add_custom_target(stage_progen3d ALL
    COMMAND "${CMAKE_COMMAND}"
        "-DAPP_EXE=$<TARGET_FILE:progen3d>"
        "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DUCRT_BIN_DIR=${ucrt_bin_dir}"
        "-DOBJDUMP=${CMAKE_OBJDUMP}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageWindows.cmake"
    DEPENDS progen3d
    VERBATIM
)
