find_package(SFML CONFIG REQUIRED)
find_package(ztcpp CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    "Collisions.cpp"
    "Context_factory.cpp"
    "Diver.cpp"
    "Environment_manager.cpp"
    "Game_stage_controller.cpp"
    "Host_menu_manager.cpp"
    "Lobby_frontend_manager.cpp"
    "Loot.cpp"
    "Main_gameplay_manager.cpp"
    "Main_menu_manager.cpp"
    "Join_menu_manager.cpp"
    "Main.cpp"
    "Resource_manager.cpp"
    "Shark.cpp"
    "Simple_zerotier.cpp"
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
