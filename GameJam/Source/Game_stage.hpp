#pragma once

#include <array>

enum GameStage : int {
    GAME_STAGE_UNKNOWN,

    GAME_STAGE_INITIAL_COUNTDOWN, //!< Countdown until the game begins
    GAME_STAGE_RUNNING,           //!< Shark is immobile, hunters are running away
    GAME_STAGE_HIDE_N_SEEK,       //!< Shark wakes up, looks for divers
    GAME_STAGE_PEARL_HUNTING,     //!< Pearls have spawned, the hunt is underway
    GAME_STAGE_FINAL,             //!< Oxygen has started disappearing
    GAME_STAGE_FINISHED,          //!< Game is over (show victory screen)

    GAME_STAGE_COUNT //!< Total number of game states (always keep last)
};

inline constexpr std::array<int, GAME_STAGE_COUNT> GetStageDurations() {
    class Units {
    public:
        static constexpr int seconds(int aX) {
            return aX * 60;
        }
        static constexpr int minutes(int aX) {
            return aX * 60 * 60;
        }
    };
#define Seconds(_x_) Units::seconds(_x_)
#define Minutes(_x_) Units::minutes(_x_)
    std::array<int, GAME_STAGE_COUNT> arr;
    arr[GAME_STAGE_UNKNOWN]           = -1;
    arr[GAME_STAGE_INITIAL_COUNTDOWN] = Seconds(3);
    arr[GAME_STAGE_RUNNING]           = Seconds(10);
    arr[GAME_STAGE_HIDE_N_SEEK]       = Seconds(60);
    arr[GAME_STAGE_PEARL_HUNTING]     = Minutes(5);
    arr[GAME_STAGE_FINAL]             = Minutes(1);
    arr[GAME_STAGE_FINISHED]          = -1; // infinite
#undef Minutes
#undef Seconds
    return arr;
}

constexpr std::array<int, GAME_STAGE_COUNT> GAME_STAGE_DURATIONS = GetStageDurations();
