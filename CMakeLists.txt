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

set(PROGEN3D_KOKORO_BUNDLE "${CMAKE_CURRENT_SOURCE_DIR}/build/kokoro-windows" CACHE PATH
    "Directory containing the extracted Windows Kokoro model and Sherpa-ONNX runtime")
set(kokoro_runtime "${PROGEN3D_KOKORO_BUNDLE}/sherpa-onnx-v2.0.3-win-x64-shared-MT-Release")
set(kokoro_model "${PROGEN3D_KOKORO_BUNDLE}/kokoro-int8-multi-lang-v1_0")
if(EXISTS "${kokoro_runtime}/include/sherpa-onnx/c-api/c-api.h" AND
   EXISTS "${kokoro_model}/model.int8.onnx")
    set_source_files_properties(src/audio/service/SpeechPlaybackService.cpp
        src/audio/service/KokoroSpeechEngine.cpp PROPERTIES
        COMPILE_DEFINITIONS PROGEN3D_KOKORO_ENABLED=1
        INCLUDE_DIRECTORIES "${kokoro_runtime}/include"
    )
    target_link_libraries(progen3d PRIVATE winmm)
else()
    message(STATUS "Kokoro bundle absent. Grammar audio events can be parsed without speech playback.")
    set(kokoro_runtime "")
    set(kokoro_model "")
endif()

get_filename_component(ucrt_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
add_custom_target(stage_progen3d ALL
    COMMAND "${CMAKE_COMMAND}"
        "-DAPP_EXE=$<TARGET_FILE:progen3d>"
        "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DUCRT_BIN_DIR=${ucrt_bin_dir}"
        "-DOBJDUMP=${CMAKE_OBJDUMP}"
        "-DKOKORO_RUNTIME_DIR=${kokoro_runtime}"
        "-DKOKORO_MODEL_DIR=${kokoro_model}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageWindows.cmake"
    DEPENDS progen3d
    VERBATIM
)

# Reuse the editor's existing engine object files while omitting the UI entry points
# and presentation objects. The static library only links objects used by the CLI.
add_library(progen3d_cli_engine STATIC
    "$<FILTER:$<TARGET_OBJECTS:progen3d>,EXCLUDE,/(src/(main|imgui_main|imgui_stb_impl)[.]cpp[.]obj$|third_party/vendor/imgui-|src/editor/presentation/)>"
)
set_target_properties(progen3d_cli_engine PROPERTIES LINKER_LANGUAGE CXX)
add_executable(progen3d-cli
    src/cli/main.cpp
    src/cli/CalibratedCamera.cpp
    src/cli/ExperimentParameters.cpp
    src/cli/ExperimentImages.cpp
    src/cli/GrammarImageFit.cpp
    src/cli/RenderSweep.cpp
    src/cli/ShotCamera.cpp
    src/cli/ShotCameraRail.cpp
    src/cli/FlowOverlay.cpp
    src/cli/RenderEffectInputs.cpp
    src/cli/SceneLayerCapture.cpp
    src/cli/DepthOfField.cpp
    src/cli/FrameOutput.cpp
    src/cli/RenderSceneTimeline.cpp
    src/cli/SequenceOutput.cpp
    src/cli/SequenceAudioCues.cpp
    src/cli/ShutterExposure.cpp
    src/cli/TimelineSettings.cpp
    src/cli/ManifestInput.cpp
    src/cli/RenderFiles.cpp
    src/cli/RenderCheckpoint.cpp
    src/cli/RenderQueue.cpp
    src/cli/VideoEdit.cpp
    src/cli/VideoComposition.cpp
    src/cli/FfmpegProcess.cpp
)
set_target_properties(progen3d-cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/headless"
)
target_include_directories(progen3d-cli PRIVATE include . ${glad_dir}/include "${STB_INCLUDE_DIR}")
target_compile_definitions(progen3d-cli PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_options(progen3d-cli PRIVATE -municode)
target_link_libraries(progen3d-cli PRIVATE
    progen3d_cli_engine glfw OpenSSL::Crypto PkgConfig::CURL PkgConfig::JSONCPP PkgConfig::XML
    PkgConfig::EIGEN comdlg32 shell32 ws2_32 gdi32 user32
)

add_custom_target(stage_progen3d_cli ALL
    COMMAND "${CMAKE_COMMAND}"
        "-DAPP_EXE=$<TARGET_FILE:progen3d-cli>"
        "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DUCRT_BIN_DIR=${ucrt_bin_dir}"
        "-DOBJDUMP=${CMAKE_OBJDUMP}"
        "-DKOKORO_RUNTIME_DIR=${kokoro_runtime}"
        "-DKOKORO_MODEL_DIR=${kokoro_model}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageWindows.cmake"
    DEPENDS progen3d-cli
    VERBATIM
)

option(BUILD_TESTING "Build native regression tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_executable(lyra_greeting_harness tests/lyra_greeting_harness.cpp
        "$<FILTER:$<TARGET_OBJECTS:progen3d>,EXCLUDE,/src/main[.]cpp[.]obj$>")
    target_include_directories(lyra_greeting_harness PRIVATE include . ${glad_dir}/include)
    target_compile_definitions(lyra_greeting_harness PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    get_target_property(lyra_editor_libraries progen3d LINK_LIBRARIES)
    target_link_libraries(lyra_greeting_harness PRIVATE ${lyra_editor_libraries})
    add_dependencies(lyra_greeting_harness stage_progen3d)
    add_test(NAME bundled_lyra_greeting COMMAND lyra_greeting_harness
        "$<TARGET_FILE_DIR:progen3d>/examples/Characters/Lyra/Lyra_Greeting.p3d")
    set_tests_properties(bundled_lyra_greeting PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${ucrt_bin_dir}")
endif()
