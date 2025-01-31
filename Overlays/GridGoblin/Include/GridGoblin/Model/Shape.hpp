// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#pragma once

#include <Hobgoblin/Common.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace jbatnozic {
namespace gridgoblin {

// MARK: Enum definition

enum class Shape : std::uint8_t {
    HFLIP  = 0x01, //!< Horizontal flip (through the Y axis)
    VFLIP  = 0x02, //!< Vertical flip (through the X axis)
    HVFLIP = HFLIP | VFLIP,

    FULL_SQUARE             = 0x00 << 2,
    LARGE_TRIANGLE          = 0x01 << 2,
    SMALL_TRIANGLE_HOR      = 0x02 << 2,
    TALL_SMALL_TRIANGLE_HOR = 0x03 << 2,
    HALF_SQUARE_HOR         = 0x04 << 2,
    SMALL_TRIANGLE_VER      = 0x05 << 2,
    TALL_SMALL_TRIANGLE_VER = 0x06 << 2,
    HALF_SQUARE_VER         = 0x07 << 2,
    UNUSED_8                = 0x08 << 2,
    UNUSED_9                = 0x09 << 2,
    UNUSED_A                = 0x0A << 2,
    UNUSED_B                = 0x0B << 2,
    UNUSED_C                = 0x0C << 2,
    UNUSED_D                = 0x0D << 2,
    UNUSED_E                = 0x0E << 2,
    UNUSED_F                = 0x0F << 2,

    BIT_6 = 0x40, //!< Reserved for future use
    BIT_7 = 0x80  //!< Reserved for future use
};

// MARK: Operators

#define TO_UNDERLYING(_val_) static_cast<std::underlying_type<decltype(_val_)>::type>(_val_)

inline constexpr Shape operator|(Shape aLhs, Shape aRhs) {
    return static_cast<Shape>(TO_UNDERLYING(aLhs) | TO_UNDERLYING(aRhs));
}

inline constexpr Shape& operator|=(Shape& aLhs, Shape aRhs) {
    return (aLhs = (aLhs | aRhs));
}

inline constexpr Shape operator&(Shape aLhs, Shape aRhs) {
    return static_cast<Shape>(TO_UNDERLYING(aLhs) & TO_UNDERLYING(aRhs));
}

inline constexpr Shape& operator&=(Shape& aLhs, Shape aRhs) {
    return (aLhs = (aLhs & aRhs));
}

#undef TO_UNDERLYING

// MARK: Obstruction

using ObstructionFlags = std::uint8_t;

// Note: north-south and east-west is swapped in regards to CellModel obstructed-by flags
enum ObstructionFlagsEnum : ObstructionFlags {
    OBSTRUCTS_NORTH       = (1 << 3),
    OBSTRUCTS_NORTH_FULLY = (1 << 7) | OBSTRUCTS_NORTH,
    OBSTRUCTS_WEST        = (1 << 2),
    OBSTRUCTS_WEST_FULLY  = (1 << 6) | OBSTRUCTS_WEST,
    OBSTRUCTS_EAST        = (1 << 1),
    OBSTRUCTS_EAST_FULLY  = (1 << 5) | OBSTRUCTS_EAST,
    OBSTRUCTS_SOUTH       = (1 << 0),
    OBSTRUCTS_SOUTH_FULLY = (1 << 4) | OBSTRUCTS_SOUTH,

    OBSTRUCTS_ALL       = OBSTRUCTS_NORTH | OBSTRUCTS_WEST | OBSTRUCTS_EAST | OBSTRUCTS_SOUTH,
    OBSTRUCTS_ALL_FULLY = 0xFF
};

inline constexpr std::array<ObstructionFlags, 8 * 4> GetObstructionFlagsForAllShapes() {
    std::array<ObstructionFlags, 8 * 4> arr;

    using hobgoblin::ToSz;

    // clang-format off
    arr[ToSz(Shape::FULL_SQUARE)]                 = OBSTRUCTS_ALL_FULLY;
    arr[ToSz(Shape::FULL_SQUARE | Shape::HFLIP)]  = OBSTRUCTS_ALL_FULLY;
    arr[ToSz(Shape::FULL_SQUARE | Shape::VFLIP)]  = OBSTRUCTS_ALL_FULLY;
    arr[ToSz(Shape::FULL_SQUARE | Shape::HVFLIP)] = OBSTRUCTS_ALL_FULLY;

    arr[ToSz(Shape::LARGE_TRIANGLE)]                 = OBSTRUCTS_ALL | OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_WEST_FULLY;
    arr[ToSz(Shape::LARGE_TRIANGLE | Shape::HFLIP)]  = OBSTRUCTS_ALL | OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_EAST_FULLY;
    arr[ToSz(Shape::LARGE_TRIANGLE | Shape::VFLIP)]  = OBSTRUCTS_ALL | OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_WEST_FULLY;
    arr[ToSz(Shape::LARGE_TRIANGLE | Shape::HVFLIP)] = OBSTRUCTS_ALL | OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_EAST_FULLY;

    arr[ToSz(Shape::SMALL_TRIANGLE_HOR)]                 = OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_SOUTH;
    arr[ToSz(Shape::SMALL_TRIANGLE_HOR | Shape::HFLIP)]  = OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_SOUTH;
    arr[ToSz(Shape::SMALL_TRIANGLE_HOR | Shape::VFLIP)]  = OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_NORTH;
    arr[ToSz(Shape::SMALL_TRIANGLE_HOR | Shape::HVFLIP)] = OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_NORTH;

    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_HOR)]                 = OBSTRUCTS_ALL | OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_WEST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HFLIP)]  = OBSTRUCTS_ALL | OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_EAST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_HOR | Shape::VFLIP)]  = OBSTRUCTS_ALL | OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_WEST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HVFLIP)] = OBSTRUCTS_ALL | OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_EAST_FULLY;

    arr[ToSz(Shape::HALF_SQUARE_HOR)]                 = OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_SOUTH;
    arr[ToSz(Shape::HALF_SQUARE_HOR | Shape::HFLIP)]  = OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_SOUTH;
    arr[ToSz(Shape::HALF_SQUARE_HOR | Shape::VFLIP)]  = OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_NORTH;
    arr[ToSz(Shape::HALF_SQUARE_HOR | Shape::HVFLIP)] = OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_NORTH;

    arr[ToSz(Shape::SMALL_TRIANGLE_VER)]                 = OBSTRUCTS_WEST_FULLY | OBSTRUCTS_EAST;
    arr[ToSz(Shape::SMALL_TRIANGLE_VER | Shape::HFLIP)]  = OBSTRUCTS_EAST_FULLY | OBSTRUCTS_WEST;
    arr[ToSz(Shape::SMALL_TRIANGLE_VER | Shape::VFLIP)]  = OBSTRUCTS_WEST_FULLY | OBSTRUCTS_EAST;
    arr[ToSz(Shape::SMALL_TRIANGLE_VER | Shape::HVFLIP)] = OBSTRUCTS_EAST_FULLY | OBSTRUCTS_WEST;

    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_VER)]                 = OBSTRUCTS_ALL | OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_WEST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_VER | Shape::HFLIP)]  = OBSTRUCTS_ALL | OBSTRUCTS_NORTH_FULLY | OBSTRUCTS_EAST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_VER | Shape::VFLIP)]  = OBSTRUCTS_ALL | OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_WEST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_VER | Shape::HVFLIP)] = OBSTRUCTS_ALL | OBSTRUCTS_SOUTH_FULLY | OBSTRUCTS_EAST_FULLY;

    arr[ToSz(Shape::HALF_SQUARE_VER)]                 = OBSTRUCTS_WEST_FULLY | OBSTRUCTS_EAST;
    arr[ToSz(Shape::HALF_SQUARE_VER | Shape::HFLIP)]  = OBSTRUCTS_EAST_FULLY | OBSTRUCTS_WEST;
    arr[ToSz(Shape::HALF_SQUARE_VER | Shape::VFLIP)]  = OBSTRUCTS_WEST_FULLY | OBSTRUCTS_EAST;
    arr[ToSz(Shape::HALF_SQUARE_VER | Shape::HVFLIP)] = OBSTRUCTS_EAST_FULLY | OBSTRUCTS_WEST;
    // clang-format on

    return arr;
}

// Fast lookup table
inline constexpr auto SHAPE_OBSTRUCTION_FLAGS = GetObstructionFlagsForAllShapes();

// MARK: String conversions

const std::string& ShapeToString(Shape aShape);

Shape StringToShape(std::string_view aString);

} // namespace gridgoblin
} // namespace jbatnozic
