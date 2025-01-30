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

// MARK: Blockage

using BlockageFlags = std::uint8_t;

enum BlockageFlagsEnum : BlockageFlags {
    BLOCKS_NORTH       = 0x01,
    BLOCKS_NORTH_FULLY = 0x03,
    BLOCKS_WEST        = 0x04,
    BLOCKS_WEST_FULLY  = 0x0C,
    BLOCKS_EAST        = 0x10,
    BLOCKS_EAST_FULLY  = 0x30,
    BLOCKS_SOUTH       = 0x40,
    BLOCKS_SOUTH_FULLY = 0xC0,

    BLOCKS_ALL       = 0xAA,
    BLOCKS_ALL_FULLY = 0xFF
};

inline constexpr std::array<BlockageFlags, 8 * 4> GetBlockageFlagsForAllShapes() {
    std::array<BlockageFlags, 8 * 4> arr;

    using hobgoblin::ToSz;

    // clang-format off
    arr[ToSz(Shape::FULL_SQUARE)]                 = BLOCKS_ALL_FULLY;
    arr[ToSz(Shape::FULL_SQUARE | Shape::HFLIP)]  = BLOCKS_ALL_FULLY;
    arr[ToSz(Shape::FULL_SQUARE | Shape::VFLIP)]  = BLOCKS_ALL_FULLY;
    arr[ToSz(Shape::FULL_SQUARE | Shape::HVFLIP)] = BLOCKS_ALL_FULLY;

    arr[ToSz(Shape::LARGE_TRIANGLE)]                 = BLOCKS_ALL | BLOCKS_NORTH_FULLY | BLOCKS_WEST_FULLY;
    arr[ToSz(Shape::LARGE_TRIANGLE | Shape::HFLIP)]  = BLOCKS_ALL | BLOCKS_NORTH_FULLY | BLOCKS_EAST_FULLY;
    arr[ToSz(Shape::LARGE_TRIANGLE | Shape::VFLIP)]  = BLOCKS_ALL | BLOCKS_SOUTH_FULLY | BLOCKS_WEST_FULLY;
    arr[ToSz(Shape::LARGE_TRIANGLE | Shape::HVFLIP)] = BLOCKS_ALL | BLOCKS_SOUTH_FULLY | BLOCKS_EAST_FULLY;

    arr[ToSz(Shape::SMALL_TRIANGLE_HOR)]                 = BLOCKS_NORTH_FULLY | BLOCKS_SOUTH;
    arr[ToSz(Shape::SMALL_TRIANGLE_HOR | Shape::HFLIP)]  = BLOCKS_NORTH_FULLY | BLOCKS_SOUTH;
    arr[ToSz(Shape::SMALL_TRIANGLE_HOR | Shape::VFLIP)]  = BLOCKS_SOUTH_FULLY | BLOCKS_NORTH;
    arr[ToSz(Shape::SMALL_TRIANGLE_HOR | Shape::HVFLIP)] = BLOCKS_SOUTH_FULLY | BLOCKS_NORTH;

    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_HOR)]                 = BLOCKS_ALL | BLOCKS_NORTH_FULLY | BLOCKS_WEST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HFLIP)]  = BLOCKS_ALL | BLOCKS_NORTH_FULLY | BLOCKS_EAST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_HOR | Shape::VFLIP)]  = BLOCKS_ALL | BLOCKS_SOUTH_FULLY | BLOCKS_WEST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HVFLIP)] = BLOCKS_ALL | BLOCKS_SOUTH_FULLY | BLOCKS_EAST_FULLY;

    arr[ToSz(Shape::HALF_SQUARE_HOR)]                 = BLOCKS_NORTH_FULLY | BLOCKS_SOUTH;
    arr[ToSz(Shape::HALF_SQUARE_HOR | Shape::HFLIP)]  = BLOCKS_NORTH_FULLY | BLOCKS_SOUTH;
    arr[ToSz(Shape::HALF_SQUARE_HOR | Shape::VFLIP)]  = BLOCKS_SOUTH_FULLY | BLOCKS_NORTH;
    arr[ToSz(Shape::HALF_SQUARE_HOR | Shape::HVFLIP)] = BLOCKS_SOUTH_FULLY | BLOCKS_NORTH;

    arr[ToSz(Shape::SMALL_TRIANGLE_VER)]                 = BLOCKS_WEST_FULLY | BLOCKS_EAST;
    arr[ToSz(Shape::SMALL_TRIANGLE_VER | Shape::HFLIP)]  = BLOCKS_EAST_FULLY | BLOCKS_WEST;
    arr[ToSz(Shape::SMALL_TRIANGLE_VER | Shape::VFLIP)]  = BLOCKS_WEST_FULLY | BLOCKS_EAST;
    arr[ToSz(Shape::SMALL_TRIANGLE_VER | Shape::HVFLIP)] = BLOCKS_EAST_FULLY | BLOCKS_WEST;

    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_VER)]                 = BLOCKS_ALL | BLOCKS_NORTH_FULLY | BLOCKS_WEST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_VER | Shape::HFLIP)]  = BLOCKS_ALL | BLOCKS_NORTH_FULLY | BLOCKS_EAST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_VER | Shape::VFLIP)]  = BLOCKS_ALL | BLOCKS_SOUTH_FULLY | BLOCKS_WEST_FULLY;
    arr[ToSz(Shape::TALL_SMALL_TRIANGLE_VER | Shape::HVFLIP)] = BLOCKS_ALL | BLOCKS_SOUTH_FULLY | BLOCKS_EAST_FULLY;

    arr[ToSz(Shape::HALF_SQUARE_VER)]                 = BLOCKS_WEST_FULLY | BLOCKS_EAST;
    arr[ToSz(Shape::HALF_SQUARE_VER | Shape::HFLIP)]  = BLOCKS_EAST_FULLY | BLOCKS_WEST;
    arr[ToSz(Shape::HALF_SQUARE_VER | Shape::VFLIP)]  = BLOCKS_WEST_FULLY | BLOCKS_EAST;
    arr[ToSz(Shape::HALF_SQUARE_VER | Shape::HVFLIP)] = BLOCKS_EAST_FULLY | BLOCKS_WEST;
    // clang-format on

    return arr;
}

// Fast lookup table
inline constexpr auto SHAPE_BLOCKAGE_FLAGS = GetBlockageFlagsForAllShapes();

// MARK: String conversions

const std::string& ShapeToString(Shape aShape);
Shape StringToShape(std::string_view aString);

} // namespace gridgoblin
} // namespace jbatnozic
