// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Model/Shape.hpp>

#include <gtest/gtest.h>

namespace jbatnozic {
namespace gridgoblin {
namespace test {

TEST(GridGoblinShapeConversionsTest, ConvertToStringAndBack) {
    const auto limit = static_cast<unsigned>(Shape::HALF_SQUARE_VER | Shape::HVFLIP);

    for (unsigned i = 0; i <= limit; i += 1) {
        const auto shape  = static_cast<Shape>(i);
        const auto string = ShapeToString(shape);
        const auto cshape = StringToShape(string);
        EXPECT_EQ(shape, cshape);
    }
}

} // namespace test
} // namespace gridgoblin
} // namespace jbatnozic
