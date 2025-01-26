#include "Environment_manager.hpp"

#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/RigelNet_macros.hpp>
#include <Hobgoblin/Utility/Randomization.hpp>

#include "Collisions.hpp"

#include "Config.hpp"
#include "Loot.hpp"
#include "Shell.hpp"
#include "Sponge.hpp"
#include "Resource_manager_interface.hpp"
#include "Sprite_manifest.hpp"
#include <array>
#include <deque>
#include <sstream>
#include <vector>

void SetTerrainImpl(EnvironmentManager& aEnvMgr,
                    hg::PZInteger       aWidth,
                    hg::PZInteger       aHeight,
                    hg::PZInteger       aRowIdx,
                    const std::string&  aCellData);

namespace {
constexpr cpFloat CELL_RESOLUTION = single_terrain_size;

enum class CellShape {
    //
    //
    //
    //
    //
    EMPTY,

    //0-0
    // X X X X X
    // X X X X X
    // X X X X X
    // X X X X X
    // X X X X X
    FULL_SQUARE,


    //3-2,2-2
    // X X X X X
    // X X X X X
    // X X X X X
    //
    //
    HALF_SQUARE_TOP,


    //3-0,2-0
    //
    //
    // X X X X X
    // X X X X X
    // X X X X X
    HALF_SQUARE_BOTTOM,


    //4-3
    // X X X X X
    // X X X X
    // X X X
    // X X
    // X
    LARGE_TRIANGLE_TL,

    //4-2
    // X X X X X
    //   X X X X
    //     X X X
    //       X X
    //         X
    LARGE_TRIANGLE_TR,

    //4-1
    // X
    // X X
    // X X X
    // X X X X
    // X X X X X
    LARGE_TRIANGLE_BL,

    //4-0
    //         X
    //       X X
    //     X X X
    //   X X X X
    // X X X X X
    LARGE_TRIANGLE_BR,


    //1-3
    // X X X X X
    // X X X
    // X
    //
    //
    SMALL_TRIANGLE_TL,

    //1-2
    // X X X X X
    //     X X X
    //         X
    //
    //
    SMALL_TRIANGLE_TR,

    //1-1
    //
    //
    // X
    // X X X
    // X X X X X
    SMALL_TRIANGLE_BL,


    //1-0
    //
    //
    //         X
    //     X X X
    // X X X X X
    SMALL_TRIANGLE_BR,


    // 12-0
    // X
    // X
    // X X
    // X X
    // X X X
    SMALL_TRIANGLE_FLIP_BR,

    // 12-1
    SMALL_TRIANGLE_FLIP_BL,

    // 12-2
    SMALL_TRIANGLE_FLIP_TR,
    // 12-3
    SMALL_TRIANGLE_FLIP_TL,

    // 8-3,9,7
    // X X X X X
    // X X X X X
    // X X X X X
    // X X X
    // X
    HIGH_SMALL_TRIANGLE_TL,

    //8-2,7,8,9
    // X X X X X
    // X X X X X
    // X X X X X
    //     X X X
    //         X
    HIGH_SMALL_TRIANGLE_TR,


    //8-1,7,9
    // X
    // X X X
    // X X X X X
    // X X X X X
    // X X X X X
    HIGH_SMALL_TRIANGLE_BL,
    
    //8-0,7,9
    //         X
    //     X X X
    // X X X X X
    // X X X X X
    // X X X X X
    HIGH_SMALL_TRIANGLE_BR,
};

void GetCellVertices(
    /*  in */ CellShape              aCellShape,
    /* out */ std::array<cpVect, 5>& aVertices,
    /* out */ std::size_t&           aVertexCount) //
{
    // Note: always set vertices in clockwise order
    switch (aCellShape) {
    case CellShape::EMPTY:
        aVertexCount = 0;
        break;

    case CellShape::FULL_SQUARE:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 0.0),
            cpv(1.0, 1.0),
            cpv(0.0, 1.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::HALF_SQUARE_TOP:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 0.0),
            cpv(1.0, 0.5),
            cpv(0.0, 0.5),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::HALF_SQUARE_BOTTOM:
        aVertices = {
            cpv(0.0, 0.5),
            cpv(1.0, 0.5),
            cpv(1.0, 1.0),
            cpv(0.0, 1.0),
            cpv(0.0, 0.5),
        };
        aVertexCount = 5;
        break;

    case CellShape::LARGE_TRIANGLE_TL:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 0.0),
            cpv(0.0, 1.0),
            cpv(0.0, 0.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::LARGE_TRIANGLE_TR:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 0.0),
            cpv(1.0, 1.0),
            cpv(0.0, 0.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::LARGE_TRIANGLE_BL:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 1.0),
            cpv(0.0, 1.0),
            cpv(0.0, 0.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::LARGE_TRIANGLE_BR:
        aVertices = {
            cpv(1.0, 0.0),
            cpv(1.0, 1.0),
            cpv(0.0, 1.0),
            cpv(1.0, 0.0),
            cpv(1.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::SMALL_TRIANGLE_TL:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 0.0),
            cpv(0.0, 0.5),
            cpv(0.0, 0.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::SMALL_TRIANGLE_TR:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 0.0),
            cpv(1.0, 0.5),
            cpv(0.0, 0.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::SMALL_TRIANGLE_BL:
        aVertices = {
            cpv(0.0, 0.5),
            cpv(1.0, 1.0),
            cpv(0.0, 1.0),
            cpv(0.0, 0.5),
            cpv(0.0, 0.5),
        };
        aVertexCount = 5;
        break;

    case CellShape::SMALL_TRIANGLE_BR:
        aVertices = {
            cpv(0.0, 1.0),
            cpv(1.0, 0.5),
            cpv(1.0, 1.0),
            cpv(0.0, 1.0),
            cpv(0.0, 1.0),
        };
        aVertexCount = 5;
        break;
    case CellShape::SMALL_TRIANGLE_FLIP_BR:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(0.5, 1.0),
            cpv(0.0, 1.0),
            cpv(0.0, 0.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;
    case CellShape::SMALL_TRIANGLE_FLIP_BL:
        aVertices = {
            cpv(1.0, 0.0),
            cpv(1.0, 1.0),
            cpv(0.5, 1.0),
            cpv(1.0, 0.0),
            cpv(1.0, 0.0),
        };
        aVertexCount = 5;
        break;
    case CellShape::SMALL_TRIANGLE_FLIP_TR:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(0.5, 0.0),
            cpv(0.0, 1.0),
            cpv(0.0, 0.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;
    case CellShape::SMALL_TRIANGLE_FLIP_TL:
        aVertices = {
            cpv(0.5, 0.0),
            cpv(1.0, 0.0),
            cpv(1.0, 1.0),
            cpv(0.5, 0.0),
            cpv(0.5, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::HIGH_SMALL_TRIANGLE_TL:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 0.0),
            cpv(0.0, 0.5),
            cpv(0.0, 1.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::HIGH_SMALL_TRIANGLE_TR:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 0.0),
            cpv(1.0, 1.0),
            cpv(1.0, 0.5),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::HIGH_SMALL_TRIANGLE_BL:
        aVertices = {
            cpv(0.0, 0.0),
            cpv(1.0, 0.5),
            cpv(1.0, 1.0),
            cpv(0.0, 1.0),
            cpv(0.0, 0.0),
        };
        aVertexCount = 5;
        break;

    case CellShape::HIGH_SMALL_TRIANGLE_BR:
        aVertices = {
            cpv(1.0, 0.0),
            cpv(1.0, 1.0),
            cpv(0.0, 1.0),
            cpv(0.0, 0.5),
            cpv(1.0, 0.0),
        };
        aVertexCount = 5;
        break;

    default:
        HG_UNREACHABLE("Invalid value for CellShape ({}).", (int)aCellShape);
    }
}

NeverNull<cpShape*> CreateCellPolyShape(NeverNull<cpBody*>  aBody,
                                        hg::math::Vector2pz aGridPosition,
                                        CellShape           aCellShape) {
    std::array<cpVect, 5> vertices;
    std::size_t           vertexCount;

    GetCellVertices(aCellShape, vertices, vertexCount);

    for (std::size_t i = 0; i < vertexCount; i += 1) {
        auto& v = vertices[i];

        v.x += (cpFloat)aGridPosition.x;
        v.x *= CELL_RESOLUTION;

        v.y += (cpFloat)aGridPosition.y;
        v.y *= CELL_RESOLUTION;
    }

    return cpPolyShapeNew(aBody,
                          static_cast<int>(vertices.size()),
                          vertices.data(),
                          cpTransformIdentity,
                          0.0);
}

RN_DEFINE_RPC(SetTerrain,
              RN_ARGS(std::int32_t,
                      aWidth,
                      std::int32_t,
                      aHeight,
                      std::int32_t,
                      aRowIdx,
                      hg::util::BufferStream&,
                      aCellData)) {
    RN_NODE_IN_HANDLER().callIfServer([](RN_ServerInterface& aServer) {
        throw RN_IllegalMessage{};
    });
    RN_NODE_IN_HANDLER().callIfClient([&](RN_ClientInterface& aClient) {
        const spe::RPCReceiverContext rc{aClient};
        auto& envMgr = static_cast<EnvironmentManager&>(rc.gameContext.getComponent<MEnvironment>());
        SetTerrainImpl(envMgr, aWidth, aHeight, aRowIdx, aCellData);
    });
}
} // namespace

void SetTerrainImpl(EnvironmentManager& aEnvMgr,
                    hg::PZInteger       aWidth,
                    hg::PZInteger       aHeight,
                    hg::PZInteger       aRowIdx,
                    hg::util::BufferStream&  aCellData) {
    if (aEnvMgr._cells.getWidth() != aWidth || aEnvMgr._cells.getHeight() != aHeight) {
        aEnvMgr._cells.reset(aWidth, aHeight);
    }


    std::size_t idx = 0;
    for (hg::PZInteger x = 0; x < aWidth; x += 1) {
        aEnvMgr._cells[aRowIdx][x].push_back(aCellData.extract<std::string>());
        idx++;
    }
}

EnvironmentManager::EnvironmentManager(QAO_RuntimeRef aRuntimeRef, int aExecutionPriority)
    : NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, aExecutionPriority, "EnvironmentManager"} {
    ccomp<MNetworking>().addEventListener(this);

    if (ccomp<MResource>().getMode() == ResourceManagerInterface::Mode::CLIENT) {
        const auto& sprLoader = ccomp<MResource>().getSpriteLoader();

        _spr       = sprLoader.getMultiBlueprint(SPR_TERRAIN).multispr();
    }
}

EnvironmentManager::~EnvironmentManager() {
    ccomp<MNetworking>().removeEventListener(this);
}

void EnvironmentManager::setToHeadlessHostMode() {
    SPEMPE_VALIDATE_GAME_CONTEXT_FLAGS(ctx(), privileged == true, headless == true);
    HG_HARD_ASSERT(_mode == Mode::UNINITIALIZED);
    _mode = Mode::HEADLESS_HOST;

    _collisionDispatcher.emplace();
    _space.emplace();
    InitColliders(*_collisionDispatcher, *_space);
}

void EnvironmentManager::setToClientMode() {
    SPEMPE_VALIDATE_GAME_CONTEXT_FLAGS(ctx(), privileged == false, headless == false);
    HG_HARD_ASSERT(_mode == Mode::UNINITIALIZED);
    _mode = Mode::CLIENT;
}

EnvironmentManager::Mode EnvironmentManager::getMode() const {
    return _mode;
}

std::vector<std::string> Split(std::string str, char split_char) {

    std::stringstream        str_stream(str);
    std::string              segment;
    std::vector<std::string> seglist;

    while (std::getline(str_stream, segment, split_char)) {
        seglist.push_back(segment);
    }

    return seglist;
}

void EnvironmentManager::_createSpongeAt(hg::math::Vector2f aPosition) {
    auto* p = QAO_PCreate<Sponge>(ctx().getQAORuntime(),
                                 ccomp<MNetworking>().getRegistryId(),
                                 spe::SYNC_ID_NEW);
    p->init(aPosition.x, aPosition.y);
}

void EnvironmentManager::_createShellAt(hg::math::Vector2f aPosition) {
    auto* p = QAO_PCreate<Shell>(ctx().getQAORuntime(),
                                 ccomp<MNetworking>().getRegistryId(),
                                 spe::SYNC_ID_NEW);
    p->init(aPosition.x, aPosition.y);
}
void EnvironmentManager::generateTerrain(hg::PZInteger aWidth, hg::PZInteger aHeight) {

    loadTerrainText();
    // Cells

    // Collision delegate
    _collisionDelegate.emplace(hg::alvin::CollisionDelegateBuilder{}
                                   .setDefaultDecision(hg::alvin::Decision::ACCEPT_COLLISION)
                                   .finalize());

    // Terrain body
    _terrainBody.emplace(hg::alvin::Body::createStatic());
    _space->add(*_terrainBody);

    // Shapes
    _shapes.reset(_cells.getWidth(), _cells.getHeight());
    for (hg::PZInteger y = 0; y < _cells.getHeight(); y += 1) {
        for (hg::PZInteger x = 0; x < _cells.getWidth(); x += 1) {
            if (_cells[y][x].size() > 0 && _cells[y][x].size() < 14) {

                std::vector<std::string> cell_value   = Split(_cells[y][x][0], '-');
                int                      spr_index    = std::stoi(cell_value[0]);
                int                      spr_rotation = std::stoi(cell_value[1]);
                CellShape                shape        = CellShape::EMPTY;
                if (spr_index == 100) {
                    shape = CellShape::EMPTY;
                }else if (spr_index == 0 || spr_index == 6 || spr_index == 5 || spr_index == 10 ||
                           spr_index == 11 || spr_index == 13) {
                    shape = CellShape::FULL_SQUARE;
                } else if ((spr_index == 3 || spr_index == 2) && spr_rotation == 2) {
                    shape = CellShape::HALF_SQUARE_TOP;
                } else if ((spr_index == 3 || spr_index == 2) && spr_rotation == 0) {
                    shape = CellShape::HALF_SQUARE_BOTTOM;
                } else if (spr_index == 4 && spr_rotation == 3) {
                    shape = CellShape::LARGE_TRIANGLE_TL;
                } else if (spr_index == 4 && spr_rotation == 2) {
                    shape = CellShape::LARGE_TRIANGLE_TR;
                } else if (spr_index == 4 && spr_rotation == 1) {
                    shape = CellShape::LARGE_TRIANGLE_BL;
                } else if (spr_index == 4 && spr_rotation == 0) {
                    shape = CellShape::LARGE_TRIANGLE_BR;
                } else if (spr_index == 1 && spr_rotation == 3) {
                    shape = CellShape::SMALL_TRIANGLE_TL;
                } else if (spr_index == 1 && spr_rotation == 2) {
                    shape = CellShape::SMALL_TRIANGLE_TR;
                } else if (spr_index == 1 && spr_rotation == 1) {
                    shape = CellShape::SMALL_TRIANGLE_BL;
                } else if (spr_index == 1 && spr_rotation == 0) {
                    shape = CellShape::SMALL_TRIANGLE_BR;
                } else if ((spr_index == 7 || spr_index == 8 || spr_index == 9 ) &&
                           spr_rotation == 3) {
                    shape = CellShape::HIGH_SMALL_TRIANGLE_TL;
                } else if ((spr_index == 7 || spr_index == 8 || spr_index == 9 ) &&
                           spr_rotation == 2) {
                    shape = CellShape::HIGH_SMALL_TRIANGLE_TR;
                } else if ((spr_index == 7 || spr_index == 8 || spr_index == 9 ) &&
                           spr_rotation == 1) {
                    shape = CellShape::HIGH_SMALL_TRIANGLE_BL;
                } else if ((spr_index == 7 || spr_index == 8 || spr_index == 9 ) &&
                           spr_rotation == 0) {
                    shape = CellShape::HIGH_SMALL_TRIANGLE_BR;
                } else if ((spr_index == 12) && spr_rotation == 0) {
                    shape = CellShape::SMALL_TRIANGLE_FLIP_BR;
                } else if ((spr_index == 12) && spr_rotation == 1) {
                    shape = CellShape::SMALL_TRIANGLE_FLIP_BL;
                } else if ((spr_index == 12) && spr_rotation == 2) {
                    shape = CellShape::SMALL_TRIANGLE_FLIP_TR;
                } else if ((spr_index == 12) && spr_rotation == 3) {
                    shape = CellShape::SMALL_TRIANGLE_FLIP_TL;
                } else if (spr_index == 14) {
                    _createShellAt({
                        x * (float)CELL_RESOLUTION,
                        y * (float)CELL_RESOLUTION,
                    });
                } else if (spr_index == 15) {
                    _createSpongeAt({
                        x * (float)CELL_RESOLUTION,
                        y * (float)CELL_RESOLUTION,
                    });
                }
                if (shape != CellShape::EMPTY) {
                    auto alvinShape =
                        hg::alvin::Shape{CreateCellPolyShape(*_terrainBody, {x, y}, shape)};
                    /* {
                        auto pair = _shapeToPosition.insert(
                            std::make_pair(static_cast<cpShape*>(alvinShape), hg::math::Vector2pz{x,
                    y})); HG_HARD_ASSERT(pair.second && "Insertion must happen!");
                    }*/
                    _shapes[y][x].emplace(std::move(alvinShape));
                    _collisionDelegate->bind(*this, *_shapes[y][x]);
                    _space->add(*_shapes[y][x]);
                }

            } 
        }
    }

}

hg::alvin::Space& EnvironmentManager::getSpace() {
    HG_ASSERT(_mode == Mode::HEADLESS_HOST && _space.has_value());
    return *_space;
}



hg::math::Vector2pz EnvironmentManager::getGridSize() const {
    return {_cells.getWidth(), _cells.getHeight()};
}

void EnvironmentManager::generatePearls() {

}

void EnvironmentManager::_eventUpdate1() {
    if (_space.has_value() && !ctx().getGameState().isPaused) {
        _space->step(1.0 / 60.0);
    }
}


void EnvironmentManager::_eventDraw1() {
    auto& winMgr = ccomp<MWindow>();
    auto& view   = winMgr.getView(0);
    auto& canvas = winMgr.getCanvas();
    canvas.clear(hg::gr::COLOR_BLACK);

    {
        auto spr = ccomp<MResource>().getSpriteLoader().getMultiBlueprint(SPR_BACKGROUND).multispr();
        spr.selectSubsprite(0);

        //spr.setRotation(hg::math::AngleF::fromDeg(60));

        const auto bounds    = spr.getLocalBounds();
        const auto worldSize = getGridSize();
        spr.setOrigin(0.5f, 0.5f);

        sf::Vector2f camera_pos = view.getCenter();

                spr.setPosition(camera_pos.x, camera_pos.y);

        canvas.draw(spr);

        hg::gr::RectangleShape rect{
            {4000.f, 4000.f}
        };
        rect.setFillColor(hg::gr::Color{255, 255, 255, 25});
        rect.setScale({worldSize.x * (float)CELL_RESOLUTION / bounds.w,
                       worldSize.y * (float)CELL_RESOLUTION / bounds.h});
        rect.setPosition(0.f, 0.f);
        canvas.draw(rect);
    }
    const hg::PZInteger startX = std::max(
        static_cast<int>((view.getCenter().x - view.getSize().x / 2.f) / (float)CELL_RESOLUTION - 1.f),
        0);
    const hg::PZInteger startY = std::max(
        static_cast<int>((view.getCenter().y - view.getSize().y / 2.f) / (float)CELL_RESOLUTION - 1.f),
        0);
    const hg::PZInteger endX = std::min(
        static_cast<int>((view.getCenter().x + view.getSize().x / 2.f) / (float)CELL_RESOLUTION + 1.f),
        _cells.getWidth());
    const hg::PZInteger endY = std::min(
        static_cast<int>((view.getCenter().y + view.getSize().y / 2.f) / (float)CELL_RESOLUTION + 1.f),
        _cells.getHeight());
    bool renderScale = true;
    for (hg::PZInteger y = startY; y < endY; y += 1) {
        for (hg::PZInteger x = startX; x < endX; x += 1) {

            std::vector<std::string> cell_value  = Split(_cells[y][x][0], '-');
            int                      spr_index   = std::stoi(cell_value[0]);
            int                      spr_rotation = std::stoi(cell_value[1]);

            if (spr_index < 14) {
                _spr.selectSubsprite(spr_index);
                if (spr_rotation == 0) {
                    _spr.setScale(1, 1);
                }
                else if (spr_rotation == 1) {
                    _spr.setScale(-1, 1);
                } else if (spr_rotation == 2) {
                    _spr.setScale(1, -1);
                } else if (spr_rotation == 3) {
                    _spr.setScale(-1, -1);
                }
                const auto& bounds = _spr.getLocalBounds();
                _spr.setOrigin(bounds.w / 2.f, bounds.h / 2.f);
                _spr.setPosition((float)CELL_RESOLUTION * (x + 0.5f),
                                 (float)CELL_RESOLUTION * (y + 0.5f));
                canvas.draw(_spr);
            }

        }
    }

    // {
    //     hg::gr::RectangleShape rect{
    //         {2.f, view.getSize().y}
    //     };
    //     rect.setFillColor(hg::gr::COLOR_RED);
    //     rect.setPosition({0.f, view.getCenter().y - view.getSize().y / 2.f});
    //     canvas.draw(rect);
    //     rect.setPosition(
    //         {getGridSize().x * (float)CELL_RESOLUTION, view.getCenter().y - view.getSize().y / 2.f});
    //     canvas.draw(rect);
    // }
}


void EnvironmentManager::onNetworkingEvent(const RN_Event& aEvent) {
    if (!ctx().isPrivileged()) {
        return;
    }

    aEvent.visit([this](const RN_Event::Connected& aEventData) {
        if (!aEventData.clientIndex.has_value()) {
            return;
        }

        for (hg::PZInteger y = 0; y < _cells.getHeight(); y += 1) {
            hg::util::BufferStream stream;

            for (hg::PZInteger x = 0; x < _cells.getWidth(); x += 1) {
                //TODO
                stream<< _cells[y][x][0];
            }

            Compose_SetTerrain(ccomp<MNetworking>().getNode(),
                               *aEventData.clientIndex,
                               static_cast<std::int32_t>(_cells.getWidth()),
                               static_cast<std::int32_t>(_cells.getHeight()),
                               static_cast<std::int32_t>(y),
                               stream);
        }
    });
}



void EnvironmentManager::loadTerrainText() {

    std::filesystem::path root = std::filesystem::current_path();
    for (int i = 0; i < 10; i += 1) {
        if (std::filesystem::exists(root / "Assets")) {
            break;
        }
        root = root.parent_path();
    }

    std::string mapText = hg::util::SlurpFileBytes(root/map_path);
    std::vector<std::string> lines     = Split(mapText, '\n');
    std::vector<std::vector<std::string>> locations   = {};
    std::vector<std::vector<std::vector<std::string>>> cells = {};

    for (auto l : lines) {
        locations.push_back(Split(l, ','));
        std::vector<std::vector<std::string>> cell{};
        for (auto c : locations.back()) {
            cell.push_back(Split(c,':'));
        }
        cells.push_back(cell);
    }



    
    HG_LOG_FATAL(LOG_ID, "sosilica --------------------------------------------");

   // std::stringstream oss;
    /* for (auto l : cells) {
        oss << "\n ";
        for (auto ll : l) {
            oss << "|";
            for (auto lll : ll) {
                oss << lll << '+';
            }

        }
    }*/

    hg::PZInteger aWidth  = static_cast<hg::PZInteger>(cells[0].size());
    hg::PZInteger aHeight = static_cast<hg::PZInteger>(cells.size());

    _cells.reset(aWidth, aHeight);
    std::stringstream oss;
        for (hg::PZInteger y = 0; y < aHeight; y += 1) {
        oss << "\n ";
            for (hg::PZInteger x = 0; x < cells[y].size(); x += 1) {
                _cells[y][x] = cells[y][x];
                oss << _cells[y][x][0] << ';';
        }
    }



        
    HG_LOG_FATAL(LOG_ID, oss.str());
}
