# Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
# See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

project("Hobgoblin.Unicode")

find_package(ICU CONFIG REQUIRED)
find_package(SFML CONFIG REQUIRED)

add_library(${PROJECT_NAME}
STATIC
    "Source/Charset_detection.cpp"
    "Source/File_io.cpp"
    "Source/Regex.cpp"
    "Source/String_conversions.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "Include/")

target_link_libraries(${PROJECT_NAME}
PUBLIC
    # Foundation
    "Hobgoblin_L00_S00_PDef"
    "Hobgoblin_L00_S01_Common"
    "Hobgoblin_L00_S01_Format"
    "Hobgoblin_L00_S02_HGExcept"
    "Hobgoblin_L00_S03_Logging"

    # External
    "icu::icu"
    "sfml::sfml"
)

add_library("Hobgoblin_L00_S04_Unicode" ALIAS ${PROJECT_NAME})

add_subdirectory("Test")
