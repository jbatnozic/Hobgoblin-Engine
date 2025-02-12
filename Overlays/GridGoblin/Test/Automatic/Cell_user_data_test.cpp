
// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Model/Cell_model.hpp>

#include <gtest/gtest.h>

namespace jbatnozic {
namespace gridgoblin {

TEST(GridGoblinCellModelTest, UserDataTest) {
    const std::int64_t value = 0x1234567890abcdefLL;

    CellModel cell;
    cell.setUserData(value);

    EXPECT_EQ(cell.getUserData(), value);
}

} // namespace gridgoblin
} // namespace jbatnozic
