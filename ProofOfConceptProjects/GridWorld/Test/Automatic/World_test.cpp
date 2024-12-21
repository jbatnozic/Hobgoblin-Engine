
#include <GridWorld/World/World.hpp>

#include <gtest/gtest.h>

#include "Fake_disk_io_handler.hpp"

namespace gridworld {

class WorldTest : public ::testing::Test {
public:
    WorldTest()
        : _config{_makeWorldConfig()}
        , _world{_config, &_fakeDiskIoHandler} {}

protected:
    test::FakeDiskIoHandler _fakeDiskIoHandler;
    WorldConfig             _config;
    World                   _world;

    static WorldConfig _makeWorldConfig() {
        return {.chunkCountX                = 8,
                .chunkCountY                = 8,
                .cellsPerChunkX             = 8,
                .cellsPerChunkY             = 8,
                .cellResolution             = 32.f,
                .maxCellOpenness            = 5,
                .maxLoadedNonessentialCells = 1};
    }
};

TEST_F(WorldTest, ChunkGetters) {
    auto& w = _world;

    const hg::math::Vector2pz pos = {9, 9};
    const ChunkId             id  = w.cellToChunkId(pos);

    EXPECT_EQ(w.getChunkAt(pos), nullptr);
    EXPECT_EQ(w.getChunkAt(pos.x, pos.y), nullptr);
    EXPECT_EQ(w.getChunkAtId(id), nullptr);
    EXPECT_EQ(w.getChunkAtUnchecked(pos), nullptr);
    EXPECT_EQ(w.getChunkAtUnchecked(pos.x, pos.y), nullptr);
    EXPECT_EQ(w.getChunkAtIdUnchecked(id), nullptr);

    const auto editPerm = w.getPermissionToEdit();

    const auto& chunk = w.getChunkAt(*editPerm, pos);
    EXPECT_EQ(&w.getChunkAt(*editPerm, pos.x, pos.y), &chunk);
    EXPECT_EQ(&w.getChunkAtId(*editPerm, id), &chunk);
    EXPECT_EQ(&w.getChunkAtUnchecked(*editPerm, pos), &chunk);
    EXPECT_EQ(&w.getChunkAtUnchecked(*editPerm, pos.x, pos.y), &chunk);
    EXPECT_EQ(&w.getChunkAtIdUnchecked(*editPerm, id), &chunk);

    EXPECT_NE(w.getChunkAt(pos), nullptr);
    EXPECT_NE(w.getChunkAt(pos.x, pos.y), nullptr);
    EXPECT_NE(w.getChunkAtId(id), nullptr);
    EXPECT_NE(w.getChunkAtUnchecked(pos), nullptr);
    EXPECT_NE(w.getChunkAtUnchecked(pos.x, pos.y), nullptr);
    EXPECT_NE(w.getChunkAtIdUnchecked(id), nullptr);
}

} // namespace gridworld
