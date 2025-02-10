// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Private/Model_conversions.hpp>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <gtest/gtest.h>

#include <iostream>

namespace jbatnozic {
namespace gridgoblin {
namespace detail {

class GridGoblinConversionsTest : public ::testing::Test {
protected:
    json::Document _document;

    json::Document _jsonDocumentFromString(const char* aJsonString) {
        json::Document doc;
        doc.Parse(aJsonString);
        return doc;
    }

#if 1
    void _print(const json::Document& aDoc) {
        json::StringBuffer                     stringbuf;
        json::PrettyWriter<json::StringBuffer> writer(stringbuf);
        aDoc.Accept(writer);
        std::cerr << stringbuf.GetString() << std::endl;
    }

    void _print(const json::Value& aVal) {
        json::StringBuffer                     stringbuf;
        json::PrettyWriter<json::StringBuffer> writer(stringbuf);
        aVal.Accept(writer);
        std::cerr << stringbuf.GetString() << std::endl;
    }
#endif
};

TEST_F(GridGoblinConversionsTest, EmptyCellConversionToJsonAndBack) {
    CellModel cell;

    EXPECT_EQ(cell, JsonToCell(CellToJson(cell, _document)));
}

TEST_F(GridGoblinConversionsTest, EmptyCellConversionFromJson) {
    static constexpr auto EMPTY_CELL_JSON = R"_({ "flags": 0 })_";

    const auto cell = JsonToCell(_jsonDocumentFromString(EMPTY_CELL_JSON));

    const CellModel emptyCell{};

    EXPECT_EQ(cell, emptyCell);
}

TEST_F(GridGoblinConversionsTest, CellConversionFromInvalidJson) {
    static constexpr auto INVALID_CELL_JSON_1 = R"_({ "flags": "asdf" })_";
    static constexpr auto INVALID_CELL_JSON_2 = R"_({ "flaks":    30  })_";

    EXPECT_THROW(JsonToCell(_jsonDocumentFromString(INVALID_CELL_JSON_1)), JsonParseError);
    EXPECT_THROW(JsonToCell(_jsonDocumentFromString(INVALID_CELL_JSON_2)), JsonParseError);
}

TEST_F(GridGoblinConversionsTest, CellWithFloorConversionToJsonAndBack) {
    CellModel cell;
    cell.setFloor({111});

    EXPECT_EQ(cell, JsonToCell(CellToJson(cell, _document)));
}

TEST_F(GridGoblinConversionsTest, CellWithWallConversionToJsonAndBack) {
    CellModel cell;
    cell.setWall({111, 222, Shape::FULL_SQUARE});

    EXPECT_EQ(cell, JsonToCell(CellToJson(cell, _document)));
}

TEST_F(GridGoblinConversionsTest, CellWithFloorAndWallConversionToJsonAndBack) {
    CellModel cell;
    cell.setFloor({111});
    cell.setWall({111, 222, Shape::SMALL_TRIANGLE_HOR | Shape::HVFLIP});

    EXPECT_EQ(cell, JsonToCell(CellToJson(cell, _document)));
}

TEST_F(GridGoblinConversionsTest, ChunkConversionToJsonAndBack) {
    CellModel cell;
    cell.setFloor({111});
    cell.setWall({111, 222, Shape::FULL_SQUARE});

    Chunk chunk{4, 4};
    chunk.setAll(cell);

    chunk.getCellAtUnchecked(2, 0).setFloor({333});
    chunk.getCellAtUnchecked(3, 1).resetWall();
    chunk.getCellAtUnchecked(1, 2).resetFloor();

    EXPECT_TRUE(AreCellsEqual(chunk, JsonStringToChunk(ChunkToJsonString(chunk))));
}

} // namespace detail
} // namespace gridgoblin
} // namespace jbatnozic
