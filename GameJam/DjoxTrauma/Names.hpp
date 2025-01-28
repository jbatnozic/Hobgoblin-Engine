#pragma once

#include "Engine.hpp"

#include <Hobgoblin/Utility/Randomization.hpp>

#include <string>

inline std::string GetRandomGymName() {
    // clang-format off
    static constexpr const char* NAMES[] = {
        "Bulk SquatThrust",
        "Thrust Master",
        "Roid",
        "Manly McMuscleman",
        "Brover",
        "Cousin Arnold",
        "HUGE!!!",
        "The Proteinator",
        "Eggmuncher",
        "Meat Grinder",
        "Wheyfinder",
        "Super(set)man",
        "Lean Beef",
        "Meathead",
        "The Barbarian",
        "Harry Spotter",
        "Throbby",
        "The Gainskeeper",
        "Nameless Peon",
        "Chad PILLZ Enjoyer",
        "Dumbbelldoor",
        "Benchpresser",
        "Ben Swolo",
        "Swoledemort",
        "Weightron",
        "Sir Lift-a-lot",
        "Lord Benchington",
        "Dagooor",
        "Buffy",
        "Serious Back",
        "Severus Shape",
        "Gaindalf"
    };
    // clang-format on

    const std::size_t min = 0;
    const std::size_t max = (sizeof(NAMES) / sizeof(NAMES[0])) - 1;
    const auto        idx = hg::util::GetRandomNumber(min, max);

    return {NAMES[idx]};
}
