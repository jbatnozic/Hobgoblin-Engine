// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Rendering/Dimetric_renderer.hpp>
#include <GridGoblin/Rendering/Dimetric_transform.hpp>
#include <GridGoblin/Rendering/Drawing_order.hpp>
#include <GridGoblin/Spatial/Position_conversions.hpp>

#include "../Detail_access.hpp"

#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Math/Core.hpp>

#include <algorithm>

namespace jbatnozic {
namespace gridgoblin {

namespace {
hg::gr::Color Blend(hg::gr::Color aCol1, hg::gr::Color aCol2) {
    return hg::gr::Color{static_cast<std::uint8_t>((aCol1.r + aCol2.r) / 2),
                         static_cast<std::uint8_t>((aCol1.g + aCol2.g) / 2),
                         static_cast<std::uint8_t>((aCol1.b + aCol2.b) / 2),
                         255};
}

hg::gr::Color ColorMax(hg::gr::Color aCol1, hg::gr::Color aCol2) {
    return hg::gr::Color{std::max(aCol1.r, aCol2.r),
                         std::max(aCol1.g, aCol2.g),
                         std::max(aCol1.b, aCol2.b),
                         255};
}
} // namespace

// MARK: Cell renderer mask bits

#define RM_REDUCTION_COUNTER_MASK 0x03FF
#define RM_RENDER_CYCLE_FLAG      0x0400
#define RM_CELL_TOUCHED           0x0800
#define RM_SHOULD_REDUCE          0x1000

// MARK: Templates

template <class taCallable>
void DimetricRenderer::_diagonalTraverse(const World&                      aWorld,
                                         const DimetricRenderer::ViewData& aViewData,
                                         taCallable&&                      aFunc) {
    const float cellRes = aWorld.getCellResolution();

    const int cellsPerRow =
        (aViewData.size.x + aViewData.overdraw.left + aViewData.overdraw.right) / (cellRes * 2.f) + 1;
    const int cellsPerColumn =
        (aViewData.size.y + aViewData.overdraw.top + aViewData.overdraw.bottom) * 2.f / cellRes;

    int startX = static_cast<int>(trunc(aViewData.topLeft->x / cellRes));
    int startY = static_cast<int>(trunc(aViewData.topLeft->y / cellRes));

    for (int row = 0; row < cellsPerColumn; row += 1) {
        for (int col = 0; col < cellsPerRow; col += 1) {
            const int x = startX + col;
            const int y = startY + col;

            if (x >= 0 && x < aWorld.getCellCountX() && y >= 0 && y < aWorld.getCellCountY()) {
                const auto* cell = aWorld.getCellAtUnchecked(x, y);

                const auto posInWorld = PositionInWorld{(x + 0.5f) * cellRes, (y + 0.5f) * cellRes};
                const auto posInView  = dimetric::ToPositionInView(posInWorld);

                aFunc(CellInfo{cell, x, y}, posInView);
            }
        }

        if (row % 2 == 0) {
            startX -= 1;
        } else {
            startY += 1;
        }
    }
}

// MARK: Public

DimetricRenderer::DimetricRenderer(const World&                  aWorld,
                                   const hg::gr::SpriteLoader&   aSpriteLoader,
                                   const DimetricRendererConfig& aConfig)
    : _world{aWorld}
    , _spriteLoader{aSpriteLoader}
    , _config{aConfig} //
{
    HG_VALIDATE_ARGUMENT(aConfig.wallReductionConfig.lowerBound <
                         aConfig.wallReductionConfig.upperBound);
}

void DimetricRenderer::startPrepareToRender(const hg::gr::View&       aView,
                                            const OverdrawAmounts&    aOverdrawAmounts,
                                            PositionInWorld           aPointOfView,
                                            std::int32_t              aRenderFlags,
                                            const VisibilityProvider* aVisProv,
                                            const LightingRenderer*   aLightingRenderer) {
    HG_VALIDATE_ARGUMENT(!!(aRenderFlags & REDUCE_WALLS_BASED_ON_VISIBILITY) == !!aVisProv);

    _viewData.center   = PositionInView{aView.getCenter()};
    _viewData.size     = aView.getSize();
    _viewData.overdraw = aOverdrawAmounts;

    _viewData.topLeft = dimetric::ToPositionInWorld(
        PositionInView{_viewData.center->x - (_viewData.size.x / 2.f) - _viewData.overdraw.left,
                       _viewData.center->y - (_viewData.size.y / 2.f) - _viewData.overdraw.top});

    _viewData.pointOfView = aPointOfView;

    _objectsToRender.clear();
    _cellAdapters.clear();

    _lightingAdapter.reset(aView, aLightingRenderer);
    _objectsToRender.push_back(&_lightingAdapter);

    _prepareCells(aRenderFlags, aVisProv, aLightingRenderer);

    _renderCycleCounter += 1;
}

void DimetricRenderer::addObject(const RenderedObject& aObject) {
    _objectsToRender.push_back(&aObject);
}

void DimetricRenderer::endPrepareToRender() {
    std::sort(_objectsToRender.begin(),
              _objectsToRender.end(),
              [](const RenderedObject* aLhs, const RenderedObject* aRhs) -> bool {
                  // Implements: Does `aLhs` come before `aRhs`?

                  HG_ASSERT(aLhs != nullptr && aRhs != nullptr);

                  const auto order =
                      dimetric::CheckDrawingOrder(aLhs->getSpatialInfo(), aRhs->getSpatialInfo());

                  switch (order) {
                  case dimetric::DRAW_LHS_FIRST:
                      return true;

                  case dimetric::DOES_NOT_MATTER:
                      return (aLhs < aRhs);

                  case dimetric::DRAW_RHS_FIRST:
                      return false;

                  default:
                      HG_UNREACHABLE("Invalid value for drawing order ({}).", (int)order);
                      return {};
                  }
              });
}

void DimetricRenderer::render(hg::gr::Canvas& aCanvas) const {
    for (const auto& object : _objectsToRender) {
        const auto& spatialInfo = object->getSpatialInfo();
        object->render(aCanvas, dimetric::ToPositionInView(spatialInfo.getCenter()));
    }
}

// MARK: Private

hg::gr::Sprite& DimetricRenderer::_getSprite(SpriteId aSpriteId) const {
    const auto iter = _spriteCache.find(aSpriteId);
    if (iter != _spriteCache.end()) {
        return iter->second;
    }

    const auto blueprint = _spriteLoader.getBlueprint(aSpriteId);
    const auto newIter   = _spriteCache.emplace(std::make_pair(aSpriteId, blueprint.spr()));
    return newIter.first->second;
}

void DimetricRenderer::_reduceCellsBelowIfCellIsVisible(hg::math::Vector2pz       aCell,
                                                        PositionInView            aCellPosInView,
                                                        const VisibilityProvider& aVisProv) {
    const auto cr = _world.getCellResolution();

    static constexpr float   PADDING      = 1.f;
    const hg::math::Vector2f positions[4] = {
        {(aCell.x + 0) * cr + PADDING, (aCell.y + 0) * cr + PADDING},
        {(aCell.x + 1) * cr - PADDING, (aCell.y + 0) * cr + PADDING},
        {(aCell.x + 1) * cr - PADDING, (aCell.y + 1) * cr - PADDING},
        {(aCell.x + 0) * cr + PADDING, (aCell.y + 1) * cr - PADDING}
    };

    const bool cellIsVisible =
        std::any_of(std::begin(positions), std::end(positions), [&aVisProv](const auto& aPos) {
            return aVisProv.testVisibilityAt(PositionInWorld{aPos}).value_or(false);
        });

    const auto markCell = [this](hg::math::Vector2pz aCell) {
        auto* cell = _world.getCellAt(aCell);
        if (cell) {
            auto& ext  = GetMutableExtensionData(*cell);
            auto  mask = ext.getRendererMask();
            if ((_renderCycleCounter & 0x01) == 0) {
                mask |= (RM_RENDER_CYCLE_FLAG | RM_CELL_TOUCHED | RM_SHOULD_REDUCE);
            } else {
                mask &= ~(RM_RENDER_CYCLE_FLAG | RM_CELL_TOUCHED);
                mask |= RM_SHOULD_REDUCE;
            }
            ext.setRendererMask(mask);
        }
    };

    if (!cellIsVisible) {
        return;
    }

    const auto screenPov = dimetric::ToPositionInView(_viewData.pointOfView);
    const auto limit     = static_cast<int>(_world.getWallHeight() / cr);
    for (int i = 0; i < limit; i += 1) {
        if (screenPov->x > aCellPosInView->x) {
            const int x = aCell.x - 1 - i;
            const int y = aCell.y + 0 + i;

            if (x >= 0 && x < _world.getCellCountX() && y >= 0 && y < _world.getCellCountY()) {
                markCell({x, y});
            }
        }
        {
            const int x = aCell.x - 1 - i;
            const int y = aCell.y + 1 + i;

            if (x >= 0 && x < _world.getCellCountX() && y >= 0 && y < _world.getCellCountY()) {
                markCell({x, y});
            }
        }
        if (screenPov->x < aCellPosInView->x) {
            const int x = aCell.x - 0 - i;
            const int y = aCell.y + 1 + i;

            if (x >= 0 && x < _world.getCellCountX() && y >= 0 && y < _world.getCellCountY()) {
                markCell({x, y});
            }
        }
    } // end_for
}

void DimetricRenderer::_prepareCells(std::int32_t              aRenderFlags,
                                     const VisibilityProvider* aVisProv,
                                     const LightingRenderer*   aLightingRenderer) //
{
    const auto cr = _world.getCellResolution();

    _diagonalTraverse(
        _world,
        _viewData,
        [this, cr, aRenderFlags, aVisProv, aLightingRenderer](const CellInfo& aCellInfo,
                                                              PositionInView  aPosInView) {
            if (aCellInfo.cell == nullptr) {
                return;
            }

            std::uint16_t mask;
            mask = _updateFlagsOfCellRendererMask(*aCellInfo.cell);

            const auto flags = aCellInfo.cell->getFlags();

            if (aVisProv != nullptr && (flags & CellModel::WALL_INITIALIZED) == 0) {
                _reduceCellsBelowIfCellIsVisible({aCellInfo.gridX, aCellInfo.gridY},
                                                 aPosInView,
                                                 *aVisProv);
            }

            const auto drawingData = GetExtensionData(*aCellInfo.cell)
                                         .getDrawingData(cr,
                                                         {aCellInfo.gridX * cr, aCellInfo.gridY * cr},
                                                         _viewData.pointOfView);

            mask = _updateFadeValueOfCellRendererMask(aCellInfo, drawingData, aRenderFlags);

            if (drawingData.suggestedDrawMode == detail::DrawingData::NONE) {
                return;
            }

            if (flags & CellModel::WALL_INITIALIZED) {
                _cellAdapters.emplace_back(
                    *this,
                    *aCellInfo.cell,
                    SpatialInfo::fromTopLeftAndSize({aCellInfo.gridX * cr, aCellInfo.gridY * cr},
                                                    {cr, cr},
                                                    Layer::WALL),
                    mask,
                    _getColorForWall(aCellInfo, flags, mask, aLightingRenderer));
            } else if (flags & CellModel::FLOOR_INITIALIZED) {
                _cellAdapters.emplace_back(
                    *this,
                    *aCellInfo.cell,
                    SpatialInfo::fromTopLeftAndSize({aCellInfo.gridX * cr, aCellInfo.gridY * cr},
                                                    {cr, cr},
                                                    Layer::FLOOR),
                    mask,
                    hg::gr::COLOR_WHITE);
            }
        });

    for (const auto& adapter : _cellAdapters) {
        _objectsToRender.push_back(&adapter);
    }
}

std::uint16_t DimetricRenderer::_updateFlagsOfCellRendererMask(const CellModel& aCell) {
    auto& ext  = GetMutableExtensionData(aCell);
    auto  mask = ext.getRendererMask();

    if ((_renderCycleCounter & 0x01) == 0) {
        static constexpr auto EXPECTED           = RM_RENDER_CYCLE_FLAG | RM_CELL_TOUCHED;
        const bool            cellAlreadyTouched = ((mask & EXPECTED) == EXPECTED);
        if (!cellAlreadyTouched) {
            mask &= ~RM_SHOULD_REDUCE;
            mask |= EXPECTED;
        }
    } else {
        static constexpr auto EXPECTED           = RM_RENDER_CYCLE_FLAG | RM_CELL_TOUCHED;
        const bool            cellAlreadyTouched = ((mask & EXPECTED) == 0);
        if (!cellAlreadyTouched) {
            mask &= ~RM_SHOULD_REDUCE;
            mask &= ~EXPECTED;
        }
    }

    ext.setRendererMask(mask);
    return mask;
}

std::uint16_t DimetricRenderer::_updateFadeValueOfCellRendererMask(
    const CellInfo&            aCellInfo,
    const detail::DrawingData& aDrawingData,
    std::int32_t               aRenderFlags) //
{
    auto& ext  = GetMutableExtensionData(*aCellInfo.cell);
    auto  mask = ext.getRendererMask();

    const auto cr = _world.getCellResolution();
    const auto cellPos =
        hg::math::Vector2f{(aCellInfo.gridX + 0.5f) * cr, (aCellInfo.gridY + 0.5f) * cr};

    if (hg::math::EuclideanDist(cellPos, *_viewData.pointOfView) >
        _config.wallReductionConfig.reductionDistanceLimit) //
    {
        mask &= ~RM_SHOULD_REDUCE;
    } else if ((aRenderFlags & REDUCE_WALLS_BASED_ON_POSITION) != 0) {
        switch (aDrawingData.suggestedDrawMode) {
        case detail::DrawingData::NONE:
            mask &= ~RM_SHOULD_REDUCE;
            break;

        case detail::DrawingData::REDUCED:
            mask |= RM_SHOULD_REDUCE;
            break;

        case detail::DrawingData::FULL:
            if ((aRenderFlags & REDUCE_WALLS_BASED_ON_VISIBILITY) == 0) {
                mask &= ~RM_SHOULD_REDUCE;
            }
            break;

        default:
            HG_UNREACHABLE("Unexpected value for DrawingData::DrawMode ({}).",
                           (int)aDrawingData.suggestedDrawMode);
            break;
        }
    } else if ((aRenderFlags & REDUCE_WALLS_BASED_ON_VISIBILITY) == 0) {
        mask &= ~RM_SHOULD_REDUCE;
    }

    auto reductionCounter = mask & RM_REDUCTION_COUNTER_MASK;
    if ((mask & RM_SHOULD_REDUCE) == 0) {
        if (reductionCounter < _config.wallReductionConfig.delta) {
            reductionCounter = 0;
        } else {
            reductionCounter -= _config.wallReductionConfig.delta;
        }
    } else {
        if (reductionCounter + _config.wallReductionConfig.delta > WallReductionConfig::MAX_VALUE) {
            reductionCounter = WallReductionConfig::MAX_VALUE;
        } else {
            reductionCounter += _config.wallReductionConfig.delta;
        }
    }

    mask = (mask & ~RM_REDUCTION_COUNTER_MASK) | reductionCounter;

    ext.setRendererMask(mask);
    return mask;
}

hg::gr::Color DimetricRenderer::_getColorForWall(const CellInfo&         aCellInfo,
                                                 std::uint16_t           aCellFlags,
                                                 std::uint16_t           aRendererMask,
                                                 const LightingRenderer* aLightingRenderer) const {
    if (aLightingRenderer == nullptr) {
        return hg::gr::COLOR_WHITE;
    }

    const bool northOpen = !(aCellFlags & CellModel::TOP_EDGE_OBSTRUCTED);
    const bool westOpen  = !(aCellFlags & CellModel::LEFT_EDGE_OBSTRUCTED);
    const bool eastOpen  = !(aCellFlags & CellModel::RIGHT_EDGE_OBSTRUCTED);
    const bool southOpen = !(aCellFlags & CellModel::BOTTOM_EDGE_OBSTRUCTED);

    const auto cr = _world.getCellResolution();

    // clang-format off
    #define OFFSET    2.f
    #define POS_NORTH (PositionInWorld{(aCellInfo.gridX + 0.5f) * cr,           aCellInfo.gridY         * cr - OFFSET})
    #define POS_WEST  (PositionInWorld{ aCellInfo.gridX         * cr - OFFSET, (aCellInfo.gridY + 0.5f) * cr})
    #define POS_EAST  (PositionInWorld{(aCellInfo.gridX + 1.f)  * cr + OFFSET, (aCellInfo.gridY + 0.5f) * cr})
    #define POS_SOUTH (PositionInWorld{(aCellInfo.gridX + 0.5f) * cr,          (aCellInfo.gridY + 1.f)  * cr + OFFSET})
    // clang-format on

    const auto ambient = aLightingRenderer->getAmbientColor();

    if ((aRendererMask & RM_SHOULD_REDUCE) == 0) {
        if (westOpen && southOpen) {
            const auto col1 = aLightingRenderer->getColorAt(POS_WEST).value_or(ambient);
            const auto col2 = aLightingRenderer->getColorAt(POS_SOUTH).value_or(ambient);
            return ColorMax(col1, col2);
        } else if (westOpen) {
            return aLightingRenderer->getColorAt(POS_WEST).value_or(ambient);
        } else if (southOpen) {
            return aLightingRenderer->getColorAt(POS_SOUTH).value_or(ambient);
        }
        return ambient;
    } else {
        hg::gr::Color color = ambient;
        if (northOpen) {
            color = ColorMax(color, aLightingRenderer->getColorAt(POS_NORTH).value_or(ambient));
        }
        if (westOpen) {
            color = ColorMax(color, aLightingRenderer->getColorAt(POS_WEST).value_or(ambient));
        }
        if (eastOpen) {
            color = ColorMax(color, aLightingRenderer->getColorAt(POS_EAST).value_or(ambient));
        }
        if (southOpen) {
            color = ColorMax(color, aLightingRenderer->getColorAt(POS_SOUTH).value_or(ambient));
        }
        return color;
    }

    // clang-format off
    #undef POS_SOUTH
    #undef POS_EAST
    #undef POS_WEST
    #undef POS_NORTH
    #undef OFFSET
    // clang-format on
}

// MARK: Cell adapter impl

DimetricRenderer::CellToRenderedObjectAdapter::CellToRenderedObjectAdapter(
    DimetricRenderer&  aRenderer,
    const CellModel&   aCell,
    const SpatialInfo& aSpatialInfo,
    std::uint16_t      aRendererMask,
    hg::gr::Color      aColor)
    : RenderedObject{aSpatialInfo}
    , _renderer{aRenderer}
    , _cell{aCell}
    , _rendererMask{aRendererMask}
    , _color{aColor} {}

void DimetricRenderer::CellToRenderedObjectAdapter::render(hg::gr::Canvas& aCanvas,
                                                           PositionInView  aScreenPosition) const {
    switch (_spatialInfo.getLayer()) {
    case Layer::FLOOR:
        {
            auto& sprite = _renderer._getSprite(_cell.getFloor().spriteId);
            sprite.setPosition(*aScreenPosition);
            sprite.setColor(hg::gr::COLOR_WHITE);
            aCanvas.draw(sprite);
        }
        break;

    case Layer::WALL:
        {
            std::uint8_t opacity;
            const auto   reductionCounter = _rendererMask & RM_REDUCTION_COUNTER_MASK;
            const auto&  wrConfig         = _renderer._config.wallReductionConfig;

            if (reductionCounter <= wrConfig.lowerBound) {
                opacity = 255;
            } else if (reductionCounter >= wrConfig.upperBound) {
                opacity = static_cast<std::uint8_t>(255 * (1.f - wrConfig.maxReduction));
            } else {
                const auto stepCount        = wrConfig.upperBound - wrConfig.lowerBound;
                const auto stepsTaken       = reductionCounter - wrConfig.lowerBound;
                const auto reductionPerStep = wrConfig.maxReduction / stepCount;

                opacity = static_cast<std::uint8_t>(
                    hg::math::Clamp(255.f * (1.f - stepsTaken * reductionPerStep), 0.f, 255.f));
            }

            if (opacity < 255) {
                auto& reducedSprite = _renderer._getSprite(_cell.getWall().spriteId_reduced);

                reducedSprite.setPosition(*aScreenPosition);
                reducedSprite.setColor(_color);
                aCanvas.draw(reducedSprite);
            }

            if (opacity > 0) {
                auto& fullSprite = _renderer._getSprite(_cell.getWall().spriteId);

                fullSprite.setPosition(*aScreenPosition);
                fullSprite.setColor(_color.withAlpha(opacity));
                aCanvas.draw(fullSprite);
            }
        }
        break;

    default:
        break;
    }
}

// MARK: Lighting adapter impl

DimetricRenderer::LightingProviderToRenderedObjectAdapter::LightingProviderToRenderedObjectAdapter() {
    _spatialInfo.setLayer(Layer::LIGHTING);
}

void DimetricRenderer::LightingProviderToRenderedObjectAdapter::reset(
    const hg::gr::View&     aView,
    const LightingRenderer* aLightingRenderer) //
{
    _spatialInfo.setCenter(dimetric::ToPositionInWorld(PositionInView{aView.getCenter()}));
    _lightingRenderer = aLightingRenderer;
}

void DimetricRenderer::LightingProviderToRenderedObjectAdapter::render(
    hg::gr::Canvas& aCanvas,
    PositionInView  aPosInView) const //
{
    if (_lightingRenderer != nullptr) {
        const hg::gr::RenderStates states{nullptr, nullptr, DIMETRIC_TRANSFORM, hg::gr::BLEND_MULTIPLY};
        _lightingRenderer->render(aCanvas, states);
    }
}

} // namespace gridgoblin
} // namespace jbatnozic
