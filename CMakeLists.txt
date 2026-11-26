cmake_minimum_required(VERSION 3.25)
project(TrueStep VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Statisk runtime: inga extra DLL-filer behövs på användarens dator
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

# Spärr: MSVC 14.50 (Visual Studio 2026) och nyare kan inte bygga för Windows 7.
if(MSVC AND MSVC_VERSION GREATER_EQUAL 1950)
    message(FATAL_ERROR "Denna kompilator stöder inte Windows 7. Använd Visual Studio 2022 (toolset v143).")
endif()

if(MSVC)
    # Källfilerna är UTF-8 (svenska kommentarer)
    add_compile_options(/utf-8)
endif()

if(WIN32)
    # Kompatibilitet med Windows 7
    add_compile_definitions(_WIN32_WINNT=0x0601 WINVER=0x0601 NOMINMAX _CRT_SECURE_NO_WARNINGS)
    set(TRUESTEP_EDITOR_SOURCES TrueStepEditor.cpp TrueStepGui.h Manual.h)
    set(TRUESTEP_EDITOR_LIBS gdiplus user32 gdi32 comdlg32 shell32)
endif()

# --- VST2-pluginet (bara på Windows) ---------------------------------------
if(WIN32)
    add_library(TrueStep MODULE TrueStep.cpp TrueStepCore.h TrueStepDSP.h vst2.h ${TRUESTEP_EDITOR_SOURCES})
    set_target_properties(TrueStep PROPERTIES PREFIX "" SUFFIX ".dll")
    target_link_libraries(TrueStep PRIVATE ${TRUESTEP_EDITOR_LIBS})
endif()

# --- Automatiska tester ----------------------------------------------------
enable_testing()

add_executable(TrueStepTests TrueStepTests.cpp)
add_test(NAME dsp_and_gui COMMAND TrueStepTests)

add_executable(HostTests HostTests.cpp TrueStep.cpp ${TRUESTEP_EDITOR_SOURCES})
target_link_libraries(HostTests PRIVATE ${TRUESTEP_EDITOR_LIBS})
add_test(NAME host COMMAND HostTests)

# Bilder av gränssnittet (Windows): ritas med samma kod som pluginet
if(WIN32)
    add_executable(RenderGui RenderGui.cpp TrueStepCore.h ${TRUESTEP_EDITOR_SOURCES})
    target_link_libraries(RenderGui PRIVATE ${TRUESTEP_EDITOR_LIBS})
endif()

# --- VST3-pluginet (om Steinbergs VST 3 SDK finns, MIT-licens) --------------
# Bygget hämtar SDK:t (se .github/workflows/build.yml) och anger sökvägen:
#   cmake -DTRUESTEP_VST3_SDK=<sökväg till vst3sdk> ...
set(TRUESTEP_VST3_SDK "" CACHE PATH "Sökväg till Steinbergs VST 3 SDK")
if(TRUESTEP_VST3_SDK)
    set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
    set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
    set(SMTG_USE_STATIC_CRT ON CACHE BOOL "" FORCE)
    set(SMTG_RUN_VST_VALIDATOR ON CACHE BOOL "" FORCE)     # Steinbergs validator körs efter varje bygge
    add_subdirectory(${TRUESTEP_VST3_SDK} ${CMAKE_BINARY_DIR}/vst3sdk)
    set(SDK_ROOT ${TRUESTEP_VST3_SDK})
    set(public_sdk_SOURCE_DIR ${TRUESTEP_VST3_SDK}/public.sdk)   # behövs för pluginets ingångspunkt

    smtg_add_vst3plugin(TrueStepVST3
        PACKAGE_NAME "TrueStep"
        SOURCES_LIST
            TrueStepVST3.cpp TrueStepCore.h TrueStepDSP.h
            ${TRUESTEP_VST3_SDK}/public.sdk/source/vst/vstsinglecomponenteffect.cpp
            ${TRUESTEP_VST3_SDK}/public.sdk/source/vst/vstsinglecomponenteffect.h
            ${TRUESTEP_EDITOR_SOURCES})
    target_link_libraries(TrueStepVST3 PRIVATE sdk ${TRUESTEP_EDITOR_LIBS})
    target_include_directories(TrueStepVST3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
