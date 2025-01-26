#pragma once
#include <filesystem>

constexpr float gravity         = 0.5;
constexpr float character_speed = 10;
constexpr float fall_timer      = 40; // in frames
constexpr float fling_timer     = 30; // in frames
constexpr float fling_speed     = 30;

constexpr int   single_terrain_size = 32;
constexpr int   terrain_size        = 40;
constexpr float y_floor             = single_terrain_size * (terrain_size + 5.f);


inline const std::filesystem::path map_path = "Assets/Map/MainMap2.txt";

constexpr float hole_chance      = 0.15f;
constexpr float terrain_1_chance = 0.3f;
constexpr float terrain_1_repeat = 0.5f;
constexpr float terrain_2_chance = 0.5f;
constexpr float terrain_2_repeat = 0.3f;
constexpr int   mountain_height  = terrain_size;
constexpr float slope_chance     = 0.5f;

inline int left_offset  = 0; // max left mountain offset
inline int right_offset = 0; // mar right mountain offset
