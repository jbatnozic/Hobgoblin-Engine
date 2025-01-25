# Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
# See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence


add_executable("multiplayer-foundation"
    "_Main.cpp"
    "Lobby_frontend_manager.cpp"
    "Main_gameplay_manager.cpp"
    "Player_character_autodiff.cpp"
    "Player_character_autodiff_alternating.cpp"
    "Player_character_alternating.cpp"
    "Player_character_basic.cpp"
)

target_link_libraries("multiplayer-foundation"
PRIVATE
    "${OVERLAY_NAME}"
)

# https://stackoverflow.com/questions/48468205/cmake-forcing-copy-of-specified-files-in-post-build-even-if-no-change-in-source

# add_custom_command(
#     OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             "${CMAKE_CURRENT_SOURCE_DIR}/assets"
#             "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
#     DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets"
# )

# add_custom_target("copy-assets" DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")

# add_dependencies("multiplayer-foundation" "copy-assets")
