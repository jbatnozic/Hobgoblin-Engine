// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <GridGoblin/Model/Shape.hpp>
#include <GridGoblin/Model/Sprites.hpp>

#include <Hobgoblin/Math/Vector.hpp>

#include <cassert>
#include <cstdint>
#include <tuple>

namespace jbatnozic {
namespace gridgoblin {

namespace hg = jbatnozic::hobgoblin;

//! A single cell of a GridGoblin World.
class CellModel {
public:
    struct Floor {
        SpriteId spriteId; // TBD: spriteId to 16 bit?
    };

    struct Wall {
        SpriteId spriteId;
        SpriteId spriteId_reduced;
        Shape    shape;
    };

    enum Flags : std::uint16_t {
        FLOOR_INITIALIZED = 1 << 1,
        WALL_INITIALIZED  = 1 << 2,

        // Obstructed-by flags
        //     bits  8..11 -> any
        //     bits 12..15 -> full
        OBSTRUCTED_BY_NORTH_NEIGHBOR       = (1 << 8),
        OBSTRUCTED_FULLY_BY_NORTH_NEIGHBOR = (1 << 12) | OBSTRUCTED_BY_NORTH_NEIGHBOR,
        OBSTRUCTED_BY_WEST_NEIGHBOR        = (1 << 9),
        OBSTRUCTED_FULLY_BY_WEST_NEIGHBOR  = (1 << 13) | OBSTRUCTED_BY_WEST_NEIGHBOR,
        OBSTRUCTED_BY_EAST_NEIGHBOR        = (1 << 10),
        OBSTRUCTED_FULLY_BY_EAST_NEIGHBOR  = (1 << 14) | OBSTRUCTED_BY_EAST_NEIGHBOR,
        OBSTRUCTED_BY_SOUTH_NEIGHBOR       = (1 << 11),
        OBSTRUCTED_FULLY_BY_SOUTH_NEIGHBOR = (1 << 15) | OBSTRUCTED_BY_SOUTH_NEIGHBOR,
    };

    std::uint16_t getFlags() const;

    //! Sets the any OBSTRUCTED_* flags according to the provided bitmask.
    //! Other flags are guaranteed to remain unchaged.
    void setObstructedByFlags(std::uint16_t aFlags);

    //! Returns true if the Floor structure of this Cell is initialized.
    //! \note equivalent to `(getFlags() & FLOOR_INITIALIZED) != 0`.
    bool isFloorInitialized() const;

    //! Returns true if the Floor structure of this Cell is initialized.
    //! \note equivalent to `(getFlags() & FLOOR_INITIALIZED) != 0`.
    bool isWallInitialized() const;

    //! Returns the Floor structure of this Cell.
    //! \warning it is undefined what is returned if `isFloorInitialized() == false`!
    const Floor& getFloor() const;

    //! Returns the Wall structure of this Cell.
    //! \warning it is undefined what is returned if `isWallInitialized() == false`!
    const Wall& getWall() const;

    //! Sets a new value for the Floor structure of this Cell
    //! and marks it as initialized.
    void setFloor(Floor aFloor);

    //! Uninitializes the Floor structure of this Cell.
    void resetFloor();

    //! Sets a new value for the Wall structure of this Cell
    //! and marks it as initialized.
    void setWall(Wall aWall);

    //! Uninitializes the Wall structure of this Cell.
    void resetWall();

    //! Returns the openness value of the cell.
    //! \see description of `WorldConfig` to see how this value is defined.
    //! \warning the returned value can be inaccurate if the cell is very close to an unloaded (or
    //! not loaded) chunk.
    std::uint8_t getOpenness() const;

    //! Set the openness value.
    void setOpenness(std::uint8_t aOpenness);

private:
    // TODO(future): more efficient packed layout
    Floor         _floor;
    Wall          _wall;
    std::uint16_t _flags    = 0;
    std::uint8_t  _openness = 0;
};

//! CellModel::Floor equality operator.
inline bool operator==(const CellModel::Floor& aLhs, const CellModel::Floor& aRhs) {
    return aLhs.spriteId == aRhs.spriteId;
}

//! CellModel::Floor inequality operator.
inline bool operator!=(const CellModel::Floor& aLhs, const CellModel::Floor& aRhs) {
    return !(aLhs == aRhs);
}

//! CellModel::Wall equality operator.
inline bool operator==(const CellModel::Wall& aLhs, const CellModel::Wall& aRhs) {
    return std::tie(aLhs.spriteId, aLhs.spriteId_reduced, aLhs.shape) ==
           std::tie(aRhs.spriteId, aRhs.spriteId_reduced, aRhs.shape);
}

//! CellModel::Wall inequality operator.
inline bool operator!=(const CellModel::Wall& aLhs, const CellModel::Wall& aRhs) {
    return !(aLhs == aRhs);
}

//! CellModel equality operator.
inline bool operator==(const CellModel& aLhs, const CellModel& aRhs) {
    if (aLhs.getFlags() != aRhs.getFlags()) {
        return false;
    }

    if (aLhs.isFloorInitialized() && (aLhs.getFloor() != aRhs.getFloor())) {
        return false;
    }

    if (aLhs.isWallInitialized() && (aLhs.getWall() != aRhs.getWall())) {
        return false;
    }

    return true;
}

//! CellModel inequality operator.
inline bool operator!=(const CellModel& aLhs, const CellModel& aRhs) {
    return !(aLhs == aRhs);
}

inline std::uint16_t CellModel::getFlags() const {
    return _flags;
}

inline void CellModel::setObstructedByFlags(std::uint16_t aFlags) {
    // clang-format off
    static constexpr auto RELEVANT_FLAGS =
        OBSTRUCTED_BY_NORTH_NEIGHBOR       |
        OBSTRUCTED_FULLY_BY_NORTH_NEIGHBOR |
        OBSTRUCTED_BY_WEST_NEIGHBOR        |
        OBSTRUCTED_FULLY_BY_WEST_NEIGHBOR  |
        OBSTRUCTED_BY_EAST_NEIGHBOR        |
        OBSTRUCTED_FULLY_BY_EAST_NEIGHBOR  |
        OBSTRUCTED_BY_SOUTH_NEIGHBOR       |
        OBSTRUCTED_FULLY_BY_SOUTH_NEIGHBOR ;
    // clang-format on

    _flags &= ~RELEVANT_FLAGS;
    _flags |= (aFlags & RELEVANT_FLAGS);
}

inline bool CellModel::isFloorInitialized() const {
    return (_flags & FLOOR_INITIALIZED) != 0;
}

inline bool CellModel::isWallInitialized() const {
    return (_flags & WALL_INITIALIZED) != 0;
}

inline const CellModel::Floor& CellModel::getFloor() const {
    assert(isFloorInitialized());
    return _floor;
}

inline const CellModel::Wall& CellModel::getWall() const {
    assert(isWallInitialized());
    return _wall;
}

inline void CellModel::setFloor(Floor aFloor) {
    _floor = aFloor;
    _flags |= FLOOR_INITIALIZED;
}

inline void CellModel::resetFloor() {
    _flags &= ~FLOOR_INITIALIZED;
}

inline void CellModel::setWall(Wall aWall) {
    _wall = aWall;
    _flags |= WALL_INITIALIZED;
}

inline void CellModel::resetWall() {
    _flags &= ~WALL_INITIALIZED;
}

inline std::uint8_t CellModel::getOpenness() const {
    return _openness;
}

inline void CellModel::setOpenness(std::uint8_t aOpenness) {
    _openness = aOpenness;
}

} // namespace gridgoblin
} // namespace jbatnozic
