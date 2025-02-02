# Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
# See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

project(GridGoblin)

find_package(GLEW CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

add_library(${PROJECT_NAME}
    # Model
    "Source/Model/Chunk.cpp"
    "Source/Model/Shape_visibility_vertices.cpp"
    "Source/Model/Shape.cpp"

    # Private
    "Source/Private/Cell_model_ext.cpp"
    "Source/Private/Chunk_disk_io_handler_default.cpp"
    "Source/Private/Chunk_spooler_default.cpp"
    "Source/Private/Chunk_storage_handler.cpp"
    "Source/Private/Model_conversions.cpp"

    # Rendering
    "Source/Rendering/Dimetric_renderer.cpp"
    "Source/Rendering/Drawing_order.cpp"
    "Source/Rendering/Top_down_los_renderer.cpp"
    "Source/Rendering/OpenGL_helpers.cpp"
    "Source/Rendering/Visibility_calculator.cpp"

    # Spatial
    "Source/Spatial/Position_conversions.cpp"
    "Source/Spatial/Spatial_info.cpp"

    # World
    "Source/World/Active_area.cpp"
    "Source/World/World.cpp"

    # Other
    "Source/Detail_access.hpp"
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
    # Foundation
    "Hobgoblin.Common"
    "Hobgoblin.Format"
    "Hobgoblin.Preprocessor"
    "Hobgoblin.HGExcept"
    "Hobgoblin.Logging"
    "Hobgoblin.Unicode"

    # Utilities
    "Hobgoblin.Math"
    "Hobgoblin.Utility"

    # Principals
    "Hobgoblin.Window"
    "Hobgoblin.Graphics"

    # External
    "GLEW::GLEW"
    "rapidjson"
)

# TODO: Chunk conversions to and from json strings could be even faster if std::string is not used
#       (not a big concern for now; it's a possible micro-optimization)
target_compile_definitions(${PROJECT_NAME}
PRIVATE
    "RAPIDJSON_HAS_STDSTRING=1"
)

target_include_directories(${PROJECT_NAME} PUBLIC "Include/")

add_subdirectory("Test")
