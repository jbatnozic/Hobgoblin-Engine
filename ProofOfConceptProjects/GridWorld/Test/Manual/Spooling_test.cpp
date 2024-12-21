#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Logging.hpp>
#include <Hobgoblin/Utility/Grids.hpp>
#include <Hobgoblin/Window.hpp>

#include <GridWorld/Private/Chunk_spooler_default.hpp>
#include "Fake_disk_io_handler.hpp"

#include <memory>
#include <unordered_map>

namespace hg = jbatnozic::hobgoblin;

using gridworld::ChunkId;
using gridworld::Chunk;
using gridworld::detail::ChunkDiskIoHandlerInterface;
using gridworld::detail::ChunkSpoolerInterface;

#define CHUNK_COUNT_X 32
#define CHUNK_COUNT_Y 32

namespace {
constexpr auto LOG_ID = "Griddy.ManualTest";

class FakeWorld {
public:
    FakeWorld(hg::PZInteger          aWorldWidth,
              hg::PZInteger          aWorldHeight,
              ChunkSpoolerInterface& aChunkSpooler)
        : _chunkspool{aChunkSpooler} {
        _chunkGrid.reset(aWorldWidth, aWorldHeight);
    }

    void update(hg::math::Vector2pz aPosition) {
        _chunkspool.pause();
        _playerPosition = aPosition;

        // Collect loaded chunks
        {
            for (auto iter = _requests.begin(); iter != _requests.end();) {
                auto& handle = iter->second;
                HG_HARD_ASSERT(handle != nullptr);

                if (handle->isFinished()) {
                    const auto id    = handle->getChunkId();
                    auto       chunk = handle->takeChunk();
                    if (chunk) {
                        // TODO: check that this grid location is null
                        _chunkGrid.at(id.y, id.x) = std::make_unique<Chunk>(std::move(*chunk));
                        HG_LOG_INFO(LOG_ID, "Loaded chunk {}, {} from disk.", id.x, id.y);
                    } else {
                        // create new one
                        _chunkGrid.at(id.y, id.x) = std::make_unique<Chunk>(1, 1);
                        HG_LOG_INFO(LOG_ID, "Created chunk {}, {}.", id.x, id.y);
                    }
                    iter = _requests.erase(iter);
                } else {
                    iter = std::next(iter);
                }
            }
        }

        // Unload excess tiles
        {
            for (hg::PZInteger y = 0; y < _chunkGrid.getHeight(); y += 1) {
                for (hg::PZInteger x = 0; x < _chunkGrid.getWidth(); x += 1) {
                    auto& chunkPtr = _chunkGrid.at(y, x);
                    if (!chunkPtr) {
                        continue;
                    }
                    if (std::abs(_playerPosition.x - x) > 2 || std::abs(_playerPosition.y - y) > 2) {
                        _chunkspool.unloadChunk({(std::uint16_t)x, (std::uint16_t)y},
                                                std::move(*chunkPtr));
                        chunkPtr.reset();
                        HG_LOG_INFO(LOG_ID, "Unloaded chunk {}, {}.", x, y);
                    }
                }
            }
        }

        // Cancel requests we don't need anymore
        {
            for (auto iter = _requests.begin(); iter != _requests.end();) {
                auto& handle = iter->second;
                HG_HARD_ASSERT(handle != nullptr);

                const auto id = handle->getChunkId();
                if (std::abs(_playerPosition.x - id.x) > 2 || std::abs(_playerPosition.y - id.y) > 2) {
                    handle->cancel();
                    iter = _requests.erase(iter);
                } else {
                    iter = std::next(iter);
                }
            }
        }

        // Set new chunks to load
        {
            std::vector<ChunkSpoolerInterface::LoadRequest> loadRequests;
            for (int yOff = -2; yOff <= 2; yOff += 1) {
                for (int xOff = -2; xOff <= 2; xOff += 1) {
                    const auto x = _playerPosition.x + xOff;
                    const auto y = _playerPosition.y + yOff;
                    if (x >= 0 && x < CHUNK_COUNT_X && y >= 0 && y < CHUNK_COUNT_Y) {
                        if (_chunkGrid[y][x] == nullptr) {
                            const auto reqIter = _requests.find({(std::uint16_t)x, (std::uint16_t)y});
                            if (reqIter == _requests.end()) {
                                ChunkSpoolerInterface::LoadRequest lr;
                                lr.chunkId  = {(std::uint16_t)x, (std::uint16_t)y};
                                lr.priority = (std::abs(xOff) + std::abs(yOff));
                                loadRequests.push_back(lr);
                            } else {
                                reqIter->second->trySwapPriority((std::abs(xOff) + std::abs(yOff)));
                            }
                        }
                    }
                }
            }
            auto handles = _chunkspool.loadChunks(std::move(loadRequests));
            for (auto& handle : handles) {
                // TODO: some safety?
                _requests[handle->getChunkId()] = std::move(handle);
            }
        }

        _chunkspool.unpause();
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

        for (hg::PZInteger y = 0; y < _chunkGrid.getHeight(); y += 1) {
            for (hg::PZInteger x = 0; x < _chunkGrid.getWidth(); x += 1) {
                if (_chunkGrid[y][x] == nullptr) {
                    emptyRect.setPosition({x * resolution, y * resolution});
                    aCanvas.draw(emptyRect);
                } else {
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
    ChunkSpoolerInterface& _chunkspool;

    hg::util::RowMajorGrid<std::unique_ptr<Chunk>> _chunkGrid;

    std::unordered_map<ChunkId, std::shared_ptr<ChunkSpoolerInterface::RequestHandleInterface>>
        _requests;

    hg::math::Vector2pz _playerPosition{0, 0};
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

void RunSpoolingTest() {
    Fixture fixture;

    hg::gr::RenderWindow window;
    window.create(hg::win::VideoMode{800, 800}, "SpoolingTest");
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
