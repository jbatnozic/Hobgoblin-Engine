# Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
# See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

# ===== PROJECT SETUP =====

cmake_minimum_required(VERSION 3.23)

project("Hobgoblin" VERSION 0.1 LANGUAGES CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    # Disable a warning (C4996) that's emitted by MSVC many times for using fmt
    # MSVC spams the build output with many warnings (C4996) because fmt uses some functions
    # which are deprecated in C++20. Silence it with the following define until a version of
    # fmt not using these functions is integrated.
    add_compile_definitions("_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING")

    # It is also possible (but not recommended) to silence this warning from all sources.
    # add_compile_options("/wd4996")

    add_compile_options("/Zc:preprocessor")
endif()

# ===== IMPORT CUSTOM FUNCTIONS =====

include("CMake/hg_copy_test_assets.cmake")

# ===== ADD ALL THE PARTS =====

# In Debug builds, define "UHOBGOBLIN_DEBUG" macro for all targets.
add_compile_definitions($<$<CONFIG:Debug>:UHOBGOBLIN_DEBUG>)

# This will make the modules which compile to Windows DLLs
# export their symbols properly.
add_compile_definitions("UHOBGOBLIN_EXPORT")

add_subdirectory("EngineCore")
add_subdirectory("Overlays")

# add_subdirectory("ProofOfConceptProjects")
add_subdirectory("Tools")
