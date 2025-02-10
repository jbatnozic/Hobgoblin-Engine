// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/GridGoblin.hpp>

#include <GridGoblin/Rendering/Visibility_calculator.hpp>

#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Input.hpp>
#include <Hobgoblin/Math.hpp>
#include <Hobgoblin/Utility/Log_with_scoped_stopwatch.hpp>
#include <Hobgoblin/Utility/Randomization.hpp>
#include <Hobgoblin/Utility/Time_utils.hpp>
#include <Hobgoblin/Window.hpp>

#include <chrono>

#include <Hobgoblin/Logging.hpp>

namespace jbatnozic {
namespace gridgoblin {
namespace {

#define LOG_ID "LoSTest"

namespace hg = jbatnozic::hobgoblin;

namespace {
void DrawChunk(hg::gr::Canvas&             aCanvas,
               const World&                aWorld,
               ChunkId                     aChunkId,
               const hg::gr::SpriteLoader& aSpriteLoader) {
    const auto  cellRes = aWorld.getCellResolution();
    const auto* chunk   = aWorld.getChunkAtId(aChunkId);
    if (chunk == nullptr) {
        return;
    }

    hg::math::Vector2f start{aChunkId.x * chunk->getCellCountX() * cellRes,
                             aChunkId.y * chunk->getCellCountY() * cellRes};

    hg::gr::RectangleShape rect{
        {cellRes, cellRes}
    };
    rect.setOutlineColor(hg::gr::COLOR_BLACK);
    rect.setOutlineThickness(1.f);

    for (hg::PZInteger y = 0; y < chunk->getCellCountY(); y += 1) {
        for (hg::PZInteger x = 0; x < chunk->getCellCountX(); x += 1) {
            const auto& cell = chunk->getCellAtUnchecked(x, y);
            const auto  ext  = static_cast<const gridgoblin::detail::CellModelExt&>(cell);
            if (cell.isWallInitialized()) {
                const auto shape = cell.getWall().shape;
                auto       spr =
                    aSpriteLoader.getMultiBlueprint(ShapeToString(shape & ~Shape::HVFLIP)).multispr();
                spr.setPosition(start.x + x * cellRes, start.y + y * cellRes);
                float xScale = 1.f;
                float yScale = 1.f;
                if ((shape & Shape::HFLIP) == Shape::HFLIP) {
                    xScale = -1.f;
                    spr.move(cellRes, 0.f);
                }
                if ((shape & Shape::VFLIP) == Shape::VFLIP) {
                    yScale = -1.f;
                    spr.move(0.f, cellRes);
                }
                spr.setScale(xScale, yScale);
                aCanvas.draw(spr);
            } else {
                rect.setPosition(start.x + x * cellRes, start.y + y * cellRes);
                rect.setFillColor(hg::gr::COLOR_TRANSPARENT);
                aCanvas.draw(rect);
            }
        }
    }
}
} // namespace

#define CELL_COUNT_X     40
#define CELL_COUNT_Y     40
#define CELLRES          32.f
#define CELL_PROBABILITY 15

void RunVisibilityCalculatorTestImpl() {
    hg::log::SetMinimalLogSeverity(hg::log::Severity::Info);

    const auto root = std::filesystem::path{HG_TEST_ASSET_DIR};

    hg::gr::SpriteLoader loader;
    // clang-format off
    loader.startTexture(256, 256)
        ->addSprite("FULL_SQUARE()",             root / "full-square.png")
        ->addSprite("LARGE_TRIANGLE()",          root / "large-triangle.png")
        ->addSprite("SMALL_TRIANGLE_HOR()",      root / "small-triangle-hor.png")
        ->addSprite("TALL_SMALL_TRIANGLE_HOR()", root / "tall-small-triangle-hor.png")
        ->addSprite("HALF_SQUARE_HOR()",         root / "half-square-hor.png")
        ->addSprite("SMALL_TRIANGLE_VER()",      root / "small-triangle-ver.png")
        ->addSprite("TALL_SMALL_TRIANGLE_VER()", root / "tall-small-triangle-ver.png")
        ->addSprite("HALF_SQUARE_VER()",         root / "half-square-ver.png")
        ->finalize(hg::gr::TexturePackingHeuristic::BestAreaFit);
    // clang-format on

    WorldConfig config{.chunkCountX                 = 1,
                       .chunkCountY                 = 1,
                       .cellsPerChunkX              = CELL_COUNT_X,
                       .cellsPerChunkY              = CELL_COUNT_Y,
                       .cellResolution              = CELLRES,
                       .maxCellOpenness             = 3,
                       .maxLoadedNonessentialChunks = 1};

    World world{config};

    // Generate world:
    {
        hg::util::DoWith32bitRNG([](std::mt19937& aRng) {
            // const auto seed = hg::util::Generate32bitSeed();
            // const auto seed = 2593577924ULL;
            // aRng.seed(seed);
            aRng.seed(0xDEADBEEF);
            // HG_LOG_INFO(LOG_ID, "32bit seed = {}", seed);
        });
        hg::util::DoWith64bitRNG([](std::mt19937_64& aRng) {
            // const auto seed = hg::util::Generate64bitSeed();
            // const auto seed = 11823085330007581526ULL;
            // aRng.seed(seed);
            aRng.seed(0xDEADBEEFDEADBEEF);
            // HG_LOG_INFO(LOG_ID, "64bit seed = {}", seed);
        });

        auto perm = world.getPermissionToEdit();
        world.edit(*perm, [&world](World::Editor& aEditor) {
            for (int y = 0; y < world.getCellCountY(); y += 1) {
                for (int x = 0; x < world.getCellCountX(); x += 1) {
                    aEditor.setFloorAt({x, y}, CellModel::Floor{0});
                    if (hg::util::GetRandomNumber(0, 99) < CELL_PROBABILITY &&
                        x != CELL_COUNT_X / 2 - 1 && x != CELL_COUNT_X / 2) {
                        aEditor.setWallAt(
                            {x, y},
                            CellModel::Wall{0, 0, (Shape)hg::util::GetRandomNumber(0, 30)});
                    }
                }
            }
        });
    }

    VisibilityCalculator visCalc{world};
    hg::gr::Image        image;
    hg::gr::Texture      texture;

    const auto generateLoS = [&](hg::math::Vector2f pos) {
        HG_LOG_INFO(LOG_ID, "===============================================");
        HG_LOG_INFO(LOG_ID, "Running calc()...");
        {
            HG_LOG_WITH_SCOPED_STOPWATCH_MS(INFO, LOG_ID, "calc() took {}ms", elapsed_time_ms);
            visCalc.calc({CELL_COUNT_X * CELLRES * 0.5f, CELL_COUNT_Y * CELLRES * 0.5f},
                         {CELL_COUNT_X * CELLRES, CELL_COUNT_Y * CELLRES},
                         PositionInWorld{pos});
        }

        HG_LOG_INFO(LOG_ID,
                    "Triangles: {}, Comparisons: {}, Rings: {}",
                    visCalc.getStats().triangleCount,
                    visCalc.getStats().triangleCheckCount,
                    visCalc.getStats().highDetailRingCount);

        HG_LOG_INFO(LOG_ID, "Generating image...");
        {
            HG_LOG_WITH_SCOPED_STOPWATCH_MS(INFO, LOG_ID, "Image generation took {}ms", elapsed_time_ms);
            image.create(hg::ToPz(CELL_COUNT_X * CELLRES), hg::ToPz(CELL_COUNT_Y * CELLRES));
            for (int y = 0; y < hg::ToPz(CELL_COUNT_X * CELLRES); y += 1) {
                for (int x = 0; x < hg::ToPz(CELL_COUNT_Y * CELLRES); x += 1) {
                    const auto v = visCalc.testVisibilityAt({(float)x, (float)y});
                    if (!v.has_value() || *v == false) {
                        image.setPixel(x, y, hg::gr::COLOR_BLACK.withAlpha(150));
                    } else {
                        image.setPixel(x, y, hg::gr::COLOR_TRANSPARENT);
                    }
                }
            }
        }

        HG_LOG_INFO(LOG_ID, "Loading texture...");
        texture.loadFromImage(image);
    };

    generateLoS({CELL_COUNT_X * CELLRES * 0.5f, CELL_COUNT_Y * CELLRES * 0.5f});

    hg::gr::Sprite spr;
    spr.setTexture(&texture, true);

    hg::gr::RenderWindow window{
        hg::win::VideoMode{1280, 1280},
        "GridWorld"
    };
    window.setFramerateLimit(120);
    // window.setVerticalSyncEnabled(true);
    window.getView().setSize({CELL_COUNT_X * CELLRES, CELL_COUNT_Y * CELLRES});
    window.getView().setCenter({CELL_COUNT_X * CELLRES * 0.5f, CELL_COUNT_Y * CELLRES * 0.5f});

    hg::util::Stopwatch swatch;

    while (window.isOpen()) {
        const auto frameTime = swatch.restart<std::chrono::microseconds>();

        bool           mouseLClick = false;
        bool           mouseRClick = false;
        hg::win::Event ev;
        while (window.pollEvent(ev)) {
            ev.visit(
                [&](hg::win::Event::Closed&) {
                    window.close();
                },
                [&](hg::win::Event::MouseButtonPressed& aButton) {
                    if (aButton.button == hg::in::MB_LEFT) {
                        const auto coords = window.mapPixelToCoords({aButton.x, aButton.y});
                        HG_LOG_INFO(LOG_ID, "Coords = {}, {}", coords.x, coords.y);
                        generateLoS(coords);
                        mouseLClick = true;
                    }
                });
        } // end event processing

        // {
        //     using namespace hg::in;
        //     const auto lr = (float)CheckPressedPK(PK_D) - (float)CheckPressedPK(PK_A);
        //     const auto ud = (float)CheckPressedPK(PK_S) - (float)CheckPressedPK(PK_W);
        //     window.getView().move({lr * 1.f, ud * 1.f});
        // }

        window.clear(hg::gr::Color{155, 155, 155});
        DrawChunk(window, world, {0, 0}, loader);
        window.draw(spr);

        window.display();
    }
}

} // namespace
} // namespace gridgoblin
} // namespace jbatnozic

void RunVisibilityCalculatorTest() {
    jbatnozic::gridgoblin::RunVisibilityCalculatorTestImpl();
}
