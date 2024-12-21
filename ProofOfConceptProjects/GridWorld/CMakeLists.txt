
project(GridWorld)

find_package(GLEW CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

add_library(${PROJECT_NAME}
    "Source/Model/Cell.cpp"
    "Source/Model/Chunk.cpp"

    "Source/Private/Chunk_disk_io_handler_default.cpp"
    "Source/Private/Chunk_spooler_default.cpp"
    "Source/Private/Chunk_storage_handler.cpp"
    "Source/Private/Model_conversions.cpp"
    
    # "Source/Rendering/Dimetric_renderer.cpp"
    "Source/Rendering/Lighting_renderer_2d.cpp"
    "Source/Rendering/Line_of_sight_renderer_2d.cpp"
    "Source/Rendering/OpenGL_helpers.cpp"
    "Source/Rendering/OpenGL_helpers.hpp"

    "Source/World/Active_area.cpp"
    "Source/World/World.cpp"
 
    "Source/Coord_conversion.cpp"
    "Source/Detail_access.hpp"
    "Source/Dimetric_transform.cpp"
    "Source/Main.cpp"
    "Source/Spatial_info.cpp"
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
target_compile_definitions(${PROJECT_NAME}
PRIVATE
    "RAPIDJSON_HAS_STDSTRING=1"
)

target_include_directories(${PROJECT_NAME} PUBLIC "Include/")

add_subdirectory("Test")
