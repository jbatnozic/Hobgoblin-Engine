project("GridWorld.ManualTest")

add_executable(${PROJECT_NAME}
    "Default_disk_io_test.cpp"
    "GridWorld_manual_test.cpp"
    "Lighting_test.cpp"
    "Openness_test.cpp"
    "Spooling_test.cpp"
    "Storage_handler_test.cpp"
)

target_link_libraries(${PROJECT_NAME}
PUBLIC
    "GridWorld"
)

target_include_directories(${PROJECT_NAME}
PUBLIC
    "../Common"
)

hg_copy_test_assets(${PROJECT_NAME} "Assets")
