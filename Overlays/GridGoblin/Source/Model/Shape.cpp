// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Model/Shape.hpp>

#include <Hobgoblin/HGExcept.hpp>

#include <unordered_map>

namespace jbatnozic {
namespace gridgoblin {

const std::string& ShapeToString(Shape aShape) {
    // clang-format off
    static const std::unordered_map<Shape, std::string> shapeToStringMap = {
        // Full square
        { Shape::FULL_SQUARE,                             "FULL_SQUARE()" },
        { Shape::FULL_SQUARE | Shape::HFLIP,              "FULL_SQUARE(HFLIP)" },
        { Shape::FULL_SQUARE | Shape::VFLIP,              "FULL_SQUARE(VFLIP)" },
        { Shape::FULL_SQUARE | Shape::HVFLIP,             "FULL_SQUARE(HFLIP,VFLIP)" },
        // Large triangle
        { Shape::LARGE_TRIANGLE,                          "LARGE_TRIANGLE()" },
        { Shape::LARGE_TRIANGLE | Shape::HFLIP,           "LARGE_TRIANGLE(HFLIP)" },
        { Shape::LARGE_TRIANGLE | Shape::VFLIP,           "LARGE_TRIANGLE(VFLIP)" },
        { Shape::LARGE_TRIANGLE | Shape::HVFLIP,          "LARGE_TRIANGLE(HFLIP,VFLIP)" },
        // Small triangle (horizontal)
        { Shape::SMALL_TRIANGLE_HOR,                      "SMALL_TRIANGLE_HOR()" },
        { Shape::SMALL_TRIANGLE_HOR | Shape::HFLIP,       "SMALL_TRIANGLE_HOR(HFLIP)" },
        { Shape::SMALL_TRIANGLE_HOR | Shape::VFLIP,       "SMALL_TRIANGLE_HOR(VFLIP)" },
        { Shape::SMALL_TRIANGLE_HOR | Shape::HVFLIP,      "SMALL_TRIANGLE_HOR(HFLIP,VFLIP)" },
        // Tall small triangle (horizontal)
        { Shape::TALL_SMALL_TRIANGLE_HOR,                 "TALL_SMALL_TRIANGLE_HOR()" },
        { Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HFLIP,  "TALL_SMALL_TRIANGLE_HOR(HFLIP)" },
        { Shape::TALL_SMALL_TRIANGLE_HOR | Shape::VFLIP,  "TALL_SMALL_TRIANGLE_HOR(VFLIP)" },
        { Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HVFLIP, "TALL_SMALL_TRIANGLE_HOR(HFLIP,VFLIP)" },
        // Half square (horizontal)
        { Shape::HALF_SQUARE_HOR,                         "HALF_SQUARE_HOR()" },
        { Shape::HALF_SQUARE_HOR | Shape::HFLIP,          "HALF_SQUARE_HOR(HFLIP)" },
        { Shape::HALF_SQUARE_HOR | Shape::VFLIP,          "HALF_SQUARE_HOR(VFLIP)" },
        { Shape::HALF_SQUARE_HOR | Shape::HVFLIP,         "HALF_SQUARE_HOR(HFLIP,VFLIP)" },
        // Small triangle (vertical)
        { Shape::SMALL_TRIANGLE_VER,                      "SMALL_TRIANGLE_VER()" },
        { Shape::SMALL_TRIANGLE_VER | Shape::HFLIP,       "SMALL_TRIANGLE_VER(HFLIP)" },
        { Shape::SMALL_TRIANGLE_VER | Shape::VFLIP,       "SMALL_TRIANGLE_VER(VFLIP)" },
        { Shape::SMALL_TRIANGLE_VER | Shape::HVFLIP,      "SMALL_TRIANGLE_VER(HFLIP,VFLIP)" },
        // Tall small triangle (vertical)
        { Shape::TALL_SMALL_TRIANGLE_VER,                 "TALL_SMALL_TRIANGLE_VER()" },
        { Shape::TALL_SMALL_TRIANGLE_VER | Shape::HFLIP,  "TALL_SMALL_TRIANGLE_VER(HFLIP)" },
        { Shape::TALL_SMALL_TRIANGLE_VER | Shape::VFLIP,  "TALL_SMALL_TRIANGLE_VER(VFLIP)" },
        { Shape::TALL_SMALL_TRIANGLE_VER | Shape::HVFLIP, "TALL_SMALL_TRIANGLE_VER(HFLIP,VFLIP)" },
        // Half square (vertical)
        { Shape::HALF_SQUARE_VER,                         "HALF_SQUARE_VER()" },
        { Shape::HALF_SQUARE_VER | Shape::HFLIP,          "HALF_SQUARE_VER(HFLIP)" },
        { Shape::HALF_SQUARE_VER | Shape::VFLIP,          "HALF_SQUARE_VER(VFLIP)" },
        { Shape::HALF_SQUARE_VER | Shape::HVFLIP,         "HALF_SQUARE_VER(HFLIP,VFLIP)" },
    };
    // clang-format on

    if (const auto it = shapeToStringMap.find(aShape); it != shapeToStringMap.end()) {
        return it->second;
    }

    using hobgoblin::TracedLogicError;
    HG_THROW_TRACED(TracedLogicError, 0, "Invalid value for aShape ({}).", (int)aShape);
}

Shape StringToShape(std::string_view aString) {
    // clang-format off
    static const std::unordered_map<std::string_view, Shape> stringToShapeMap = {
        // Full square
        { "FULL_SQUARE()",                        Shape::FULL_SQUARE },
        { "FULL_SQUARE(HFLIP)",                   Shape::FULL_SQUARE | Shape::HFLIP },
        { "FULL_SQUARE(VFLIP)",                   Shape::FULL_SQUARE | Shape::VFLIP },
        { "FULL_SQUARE(HFLIP,VFLIP)",             Shape::FULL_SQUARE | Shape::HVFLIP },
        // Large triangle
        { "LARGE_TRIANGLE()",                     Shape::LARGE_TRIANGLE },
        { "LARGE_TRIANGLE(HFLIP)",                Shape::LARGE_TRIANGLE | Shape::HFLIP },
        { "LARGE_TRIANGLE(VFLIP)",                Shape::LARGE_TRIANGLE | Shape::VFLIP },
        { "LARGE_TRIANGLE(HFLIP,VFLIP)",          Shape::LARGE_TRIANGLE | Shape::HVFLIP },
        // Small triangle (horizontal)
        { "SMALL_TRIANGLE_HOR()",                 Shape::SMALL_TRIANGLE_HOR },
        { "SMALL_TRIANGLE_HOR(HFLIP)",            Shape::SMALL_TRIANGLE_HOR | Shape::HFLIP },
        { "SMALL_TRIANGLE_HOR(VFLIP)",            Shape::SMALL_TRIANGLE_HOR | Shape::VFLIP },
        { "SMALL_TRIANGLE_HOR(HFLIP,VFLIP)",      Shape::SMALL_TRIANGLE_HOR | Shape::HVFLIP },
        // Tall small triangle (horizontal)
        { "TALL_SMALL_TRIANGLE_HOR()",            Shape::TALL_SMALL_TRIANGLE_HOR },
        { "TALL_SMALL_TRIANGLE_HOR(HFLIP)",       Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HFLIP },
        { "TALL_SMALL_TRIANGLE_HOR(VFLIP)",       Shape::TALL_SMALL_TRIANGLE_HOR | Shape::VFLIP },
        { "TALL_SMALL_TRIANGLE_HOR(HFLIP,VFLIP)", Shape::TALL_SMALL_TRIANGLE_HOR | Shape::HVFLIP },
        // Half square (horizontal)
        { "HALF_SQUARE_HOR()",                    Shape::HALF_SQUARE_HOR },
        { "HALF_SQUARE_HOR(HFLIP)",               Shape::HALF_SQUARE_HOR | Shape::HFLIP },
        { "HALF_SQUARE_HOR(VFLIP)",               Shape::HALF_SQUARE_HOR | Shape::VFLIP },
        { "HALF_SQUARE_HOR(HFLIP,VFLIP)",         Shape::HALF_SQUARE_HOR | Shape::HVFLIP },
        // Small triangle (vertical)
        { "SMALL_TRIANGLE_VER()",                 Shape::SMALL_TRIANGLE_VER },
        { "SMALL_TRIANGLE_VER(HFLIP)",            Shape::SMALL_TRIANGLE_VER | Shape::HFLIP },
        { "SMALL_TRIANGLE_VER(VFLIP)",            Shape::SMALL_TRIANGLE_VER | Shape::VFLIP },
        { "SMALL_TRIANGLE_VER(HFLIP,VFLIP)",      Shape::SMALL_TRIANGLE_VER | Shape::HVFLIP },
        // Tall small triangle (vertical)
        { "TALL_SMALL_TRIANGLE_VER()",            Shape::TALL_SMALL_TRIANGLE_VER },
        { "TALL_SMALL_TRIANGLE_VER(HFLIP)",       Shape::TALL_SMALL_TRIANGLE_VER | Shape::HFLIP },
        { "TALL_SMALL_TRIANGLE_VER(VFLIP)",       Shape::TALL_SMALL_TRIANGLE_VER | Shape::VFLIP },
        { "TALL_SMALL_TRIANGLE_VER(HFLIP,VFLIP)", Shape::TALL_SMALL_TRIANGLE_VER | Shape::HVFLIP },
        // Half square (vertical)
        { "HALF_SQUARE_VER()",                    Shape::HALF_SQUARE_VER },
        { "HALF_SQUARE_VER(HFLIP)",               Shape::HALF_SQUARE_VER | Shape::HFLIP },
        { "HALF_SQUARE_VER(VFLIP)",               Shape::HALF_SQUARE_VER | Shape::VFLIP },
        { "HALF_SQUARE_VER(HFLIP,VFLIP)",         Shape::HALF_SQUARE_VER | Shape::HVFLIP },
    };
    // clang-format on

    if (const auto it = stringToShapeMap.find(aString); it != stringToShapeMap.end()) {
        return it->second;
    }

    using hobgoblin::TracedLogicError;
    HG_THROW_TRACED(TracedLogicError, 0, "Invalid value for aString ({}).", aString);
}

} // namespace gridgoblin
} // namespace jbatnozic
