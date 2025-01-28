find_package(SFML CONFIG REQUIRED)
find_package(ztcpp CONFIG REQUIRED)

project("DjoxTrauma")

add_executable(${PROJECT_NAME}
    "Collisions.cpp"
    "Context_factory.cpp" 
    "Game_stage_controller.cpp"
    "Main.cpp"
    "Simple_zerotier.cpp"

    "Managers/Source/Environment_manager.cpp"
    "Managers/Source/Host_menu_manager.cpp"
    "Managers/Source/Join_menu_manager.cpp"
    "Managers/Source/Lobby_frontend_manager.cpp"
    "Managers/Source/Main_gameplay_manager.cpp"
    "Managers/Source/Main_menu_manager.cpp"
    "Managers/Source/Resource_manager.cpp"

    "GameObjects/Source/Bubble.cpp"
    "GameObjects/Source/Diver.cpp"
    "GameObjects/Source/Pearl.cpp"
    "GameObjects/Source/Shark.cpp"
    "GameObjects/Source/Shell.cpp"
    "GameObjects/Source/Sponge.cpp"
)

target_include_directories(${PROJECT_NAME} PRIVATE ".")
target_include_directories(${PROJECT_NAME} PRIVATE "Managers/Include")
target_include_directories(${PROJECT_NAME} PRIVATE "GameObjects/Include")

target_link_libraries(${PROJECT_NAME}
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

    # Principal
    "Hobgoblin.ChipmunkPhysics"
    "Hobgoblin.HGConfig"
    "Hobgoblin.Input"
    "Hobgoblin.QAO"
    "Hobgoblin.RigelNet"
    "Hobgoblin.Window"
    "Hobgoblin.Alvin"
    "Hobgoblin.Graphics"
    "Hobgoblin.RmlUi"

    # Overlay
    "SPeMPE"
    "GridGoblin"

    # External
    "sfml::sfml"
    "ztcpp::ztcpp"
)
