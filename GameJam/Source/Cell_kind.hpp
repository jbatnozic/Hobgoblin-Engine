#pragma once

#include <cstdint>

enum class CellKind : std::int8_t {
    EMPTY,
    ROCK_1,
    ROCK_2,
    ROCK_3,
    ROCK_T_1,
    ROCK_T_2,
    ROCK_T_3,
    ROCK_MT_1,
    ROCK_MT_2,
    ROCK_MT_3,
    SCALE,
};
