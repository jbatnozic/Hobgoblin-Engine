find_package(SFML CONFIG REQUIRED)
find_package(ztcpp CONFIG REQUIRED)

project("DjoxTrauma")

add_executable(${PROJECT_NAME}
    "Collisions.cpp"
    "Context_factory.cpp" 
    "Game_stage_controller.cpp"
    "Main.cpp"
    "Simple_zerotier.cpp"
    
    "Managers/Environment_manager.cpp"
    "Managers/Host_menu_manager.cpp"
    "Managers/Join_menu_manager.cpp"
    "Managers/Lobby_frontend_manager.cpp"
    "Managers/Main_gameplay_manager.cpp"
    "Managers/Main_menu_manager.cpp"
    "Managers/Resource_manager.cpp"

    "GameObjects/Bubble.cpp"
    "GameObjects/Diver.cpp"
    "GameObjects/Loot.cpp"
    "GameObjects/Pearl.cpp"
    "GameObjects/Shark.cpp"
    "GameObjects/Shell.cpp"
    "GameObjects/Sponge.cpp"
 )

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

target_include_directories(${PROJECT_NAME} PRIVATE .)
