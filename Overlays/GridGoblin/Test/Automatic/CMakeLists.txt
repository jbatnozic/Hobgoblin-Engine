# Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
# See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

project("GridGoblin.AutomaticTest")

find_package(GTest CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    "Active_area_test.cpp"
    "Model_conversions_test.cpp"
    "Shape_conversions_test.cpp"
    "Spatial_info_test.cpp"
    "World_test.cpp"
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
    "GridGoblin"
    "gtest::gtest"
)

target_include_directories(${PROJECT_NAME}
PUBLIC
    "../Common"
)
