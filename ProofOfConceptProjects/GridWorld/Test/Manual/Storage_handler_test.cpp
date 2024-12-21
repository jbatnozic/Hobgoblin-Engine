#include <GridWorld/Private/Chunk_storage_handler.hpp>
#include <GridWorld/Private/Chunk_spooler_default.hpp>

#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Utility/Grids.hpp>
#include <Hobgoblin/Window.hpp>

#include "Fake_disk_io_handler.hpp"

#include <algorithm>
#include <memory>
#include <unordered_map>

namespace hg = jbatnozic::hobgoblin;

using gridworld::ActiveArea;
using gridworld::ChunkId;
using gridworld::Chunk;
using gridworld::WorldConfig;
using gridworld::detail::ChunkDiskIoHandlerInterface;
using gridworld::detail::ChunkSpoolerInterface;
using gridworld::detail::ChunkStorageHandler;

#define CHUNK_COUNT_X 32
#define CHUNK_COUNT_Y 32

namespace {
constexpr auto LOG_ID = "Griddy.ManualTest";

template <class taContainer>
bool Contains(const taContainer& aContainer, const typename taContainer::value_type& aValue) {
    for (const auto& item : aContainer) {
        if (item == aValue) {
            return true;
        }
    }
    return false;
}

class FakeWorld {
public:
    FakeWorld(hg::PZInteger          aWorldWidth,
              hg::PZInteger          aWorldHeight,
              ChunkSpoolerInterface& aChunkSpooler)
        : _chunkStorageHandler{_makeWorldConfig()}
        , _activeArea{_chunkStorageHandler.createNewActiveArea()} //
    {
        _chunkStorageHandler.setChunkSpooler(&aChunkSpooler);
    }

    void update(hg::math::Vector2pz aPosition) {
        if (aPosition != _playerPosition) {
            _playerPosition = aPosition;
            const auto id = ChunkId{(std::uint16_t)aPosition.x, (std::uint16_t)aPosition.y};
            _activeArea.setToChunkRingDiamond(id, 2);
        }
        
        _chunkStorageHandler.update();
        _chunkStorageHandler.prune();
    }

    void draw(hg::gr::Canvas& aCanvas) {
        const float resolution = 16.f;

        hg::gr::RectangleShape emptyRect{
            {resolution, resolution}
        };
        emptyRect.setFillColor(hg::gr::COLOR_TRANSPARENT);
        emptyRect.setOutlineColor(hg::gr::COLOR_BLACK);
        emptyRect.setOutlineThickness(2.f);

        hg::gr::RectangleShape filledRect{
            {resolution, resolution}
        };
        filledRect.setFillColor(hg::gr::COLOR_GREEN);

        for (hg::PZInteger y = 0; y < CHUNK_COUNT_Y; y += 1) {
            for (hg::PZInteger x = 0; x < CHUNK_COUNT_X; x += 1) {
                if (_chunkStorageHandler.getCellAtUnchecked(x, y) == nullptr) {
                    emptyRect.setPosition({x * resolution, y * resolution});
                    aCanvas.draw(emptyRect);
                } else {
                    if (Contains(_activeArea.getChunkList(), ChunkId{x, y})) {
                        filledRect.setFillColor(hg::gr::COLOR_GREEN);
                    } else {
                        filledRect.setFillColor(hg::gr::COLOR_DARK_TURQUOISE);
                    }
                    filledRect.setPosition({x * resolution, y * resolution});
                    aCanvas.draw(filledRect);
                }
                if (x == _playerPosition.x && y == _playerPosition.y) {
                    hg::gr::CircleShape circle{resolution / 4.f};
                    circle.setFillColor(hg::gr::COLOR_RED);
                    circle.setOrigin({resolution / 4.f, resolution / 4.f});
                    circle.setPosition({(x + 0.5f) * resolution, (y + 0.5f) * resolution});
                    aCanvas.draw(circle);
                }
            }
        }
    }

private:
    ChunkStorageHandler _chunkStorageHandler;
    hg::math::Vector2pz _playerPosition;
    ActiveArea          _activeArea;

    static WorldConfig _makeWorldConfig() {
        return {.chunkCountX                = CHUNK_COUNT_X,
                .chunkCountY                = CHUNK_COUNT_Y,
                .cellsPerChunkX             = 1,
                .cellsPerChunkY             = 1,
                .cellResolution             = 32.f,
                .maxCellOpenness            = 4,
                .maxLoadedNonessentialCells = 32};
    }
};

class Fixture {
public:
    gridworld::test::FakeDiskIoHandler     _fakeDiskIoHandler;
    gridworld::detail::DefaultChunkSpooler _chunkSpooler;
    FakeWorld                              _fakeWorld{32, 32, _chunkSpooler};

    Fixture() {
        _chunkSpooler.setDiskIoHandler(&_fakeDiskIoHandler);
    }
};
} // namespace

void RunStorageHandlerTest() {
    Fixture fixture;

    hg::gr::RenderWindow window;
    window.create(hg::win::VideoMode{800, 800}, "StorageHandlerTest");
    window.setViewCount(1);
    window.getView(0).setCenter({16 * 16.f, 16 * 16.f});
    window.getView(0).setSize({32 * 16.f, 32 * 16.f});
    window.getView(0).setViewport({0.f, 0.f, 1.f, 1.f});
    window.setFramerateLimit(60);

    hg::math::Vector2pz playerPosition{0, 0};
    fixture._fakeWorld.update(playerPosition);

    while (window.isOpen()) {
        hg::win::Event ev;
        while (window.pollEvent(ev)) {
            ev.visit(
                [&](const hg::win::Event::Closed&) {
                window.close();
            },
                [&](const hg::win::Event::KeyPressed& aData) {
                switch (aData.physicalKey) {
                case hg::in::PK_LEFT:
                    if (playerPosition.x > 0) {
                        playerPosition.x -= 1;
                    }
                    break;

                case hg::in::PK_RIGHT:
                    if (playerPosition.x < CHUNK_COUNT_X - 1) {
                        playerPosition.x += 1;
                    }
                    break;

                case hg::in::PK_UP:
                    if (playerPosition.y > 0) {
                        playerPosition.y -= 1;
                    }
                    break;

                case hg::in::PK_DOWN:
                    if (playerPosition.y < CHUNK_COUNT_Y - 1) {
                        playerPosition.y += 1;
                    }
                    break;

                default:
                    break;
                }
            });
        }

        window.clear(hg::gr::COLOR_LIGHT_GRAY);
        fixture._fakeWorld.update(playerPosition);
        fixture._fakeWorld.draw(window);
        window.display();
    }
}
