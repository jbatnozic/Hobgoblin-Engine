// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Private/Cell_model_ext.hpp>

#include <Hobgoblin/HGExcept.hpp>

namespace jbatnozic {
namespace gridgoblin {
namespace detail {

namespace predicate {
namespace {

constexpr float Half(float aVal) {
    return aVal * 0.5;
}

bool IsPovCloserThanCell(PositionInWorld aCellTopLeft, PositionInWorld aPointOfView) {
    return ((aCellTopLeft->x - aCellTopLeft->y) > (aPointOfView->x - aPointOfView->y));
}

DrawingData FullWhenPovIsCloserToCamera(float           aCellResolution,
                                        PositionInWorld aCellTopLeft,
                                        PositionInWorld aPointOfView) {
    if (IsPovCloserThanCell(aCellTopLeft, aPointOfView)) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData X1(float aCellResolution, PositionInWorld aCellTopLeft, PositionInWorld aPointOfView) {
    if (aPointOfView->y <= aCellTopLeft->y || aPointOfView->x >= aCellTopLeft->x + aCellResolution) {
        return {DrawingData::REDUCED};
    }
    return {DrawingData::FULL};
}

DrawingData FullWhenPovIsSouthOrCloserToCamera(float           aCellResolution,
                                               PositionInWorld aCellTopLeft,
                                               PositionInWorld aPointOfView) {
    if ((aPointOfView->y >= aCellTopLeft->y + aCellResolution) ||
        IsPovCloserThanCell(aCellTopLeft, aPointOfView)) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData FullWhenPovIsWestOrCloserToCamera(float           aCellResolution,
                                              PositionInWorld aCellTopLeft,
                                              PositionInWorld aPointOfView) {
    if ((aPointOfView->x <= aCellTopLeft->x) || IsPovCloserThanCell(aCellTopLeft, aPointOfView)) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData FullWhenPovIsWestOrSouth(float           aCellResolution,
                                     PositionInWorld aCellTopLeft,
                                     PositionInWorld aPointOfView) {
    if ((aPointOfView->x <= aCellTopLeft->x) || (aPointOfView->y >= aCellTopLeft->y + aCellResolution)) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData FullWhenPovIsWest(float           aCellResolution,
                              PositionInWorld aCellTopLeft,
                              PositionInWorld aPointOfView) {
    if (aPointOfView->x <= aCellTopLeft->x) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData FullWhenPovIsWest2(float           aCellResolution,
                               PositionInWorld aCellTopLeft,
                               PositionInWorld aPointOfView) {
    if (aPointOfView->x <= aCellTopLeft->x + aCellResolution) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData FullWhenPovIsSouth(float           aCellResolution,
                               PositionInWorld aCellTopLeft,
                               PositionInWorld aPointOfView) {
    if (aPointOfView->y >= aCellTopLeft->y + aCellResolution) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData FullWhenPovIsSouth2(float           aCellResolution,
                                PositionInWorld aCellTopLeft,
                                PositionInWorld aPointOfView) {
    if (aPointOfView->y >= aCellTopLeft->y + aCellResolution) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData FullWhenPovIsSouthAndCloserToCamera(float           aCellResolution,
                                                PositionInWorld aCellTopLeft,
                                                PositionInWorld aPointOfView) {
    if ((aPointOfView->y >= aCellTopLeft->y + aCellResolution) &&
        IsPovCloserThanCell(aCellTopLeft, aPointOfView)) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData FullWhenPovIsWestAndCloserToCamera(float           aCellResolution,
                                               PositionInWorld aCellTopLeft,
                                               PositionInWorld aPointOfView) {
    if ((aPointOfView->x <= aCellTopLeft->x) && IsPovCloserThanCell(aCellTopLeft, aPointOfView)) {
        return {DrawingData::FULL};
    }
    return {DrawingData::REDUCED};
}

DrawingData AlwaysLowered(float           aCellResolution,
                          PositionInWorld aCellTopLeft,
                          PositionInWorld aPointOfView) {
    return {DrawingData::REDUCED};
}

DrawingData AlwaysNone(float           aCellResolution,
                       PositionInWorld aCellTopLeft,
                       PositionInWorld aPointOfView) {
    return {DrawingData::NONE};
}

std::array<decltype(&AlwaysNone), 16> SELECTION_TABLE = {
    // b0000
    //   _
    // _ O _
    //   _
    //&FullWhenPovIsCloserToCamera,
    &X1,

    // b0001
    //   _
    // _ O X
    //   _
    &FullWhenPovIsSouthOrCloserToCamera,

    // b0010
    //   X
    // _ O _
    //   _
    &FullWhenPovIsWestOrCloserToCamera,

    // b0011
    //   X
    // _ O X
    //   _
    &FullWhenPovIsWestOrSouth,

    // b0100
    //   _
    // X O _
    //   _
    &FullWhenPovIsSouth,

    // b0101
    //   _
    // X O X
    //   _
    //&FullWhenPovIsSouth,
    &FullWhenPovIsSouth2,

    // b0110
    //   X
    // X O _
    //   _
    &FullWhenPovIsSouthAndCloserToCamera,

    // b0111
    //   X
    // X O X
    //   _
    &FullWhenPovIsSouth,

    // b1000
    //   _
    // _ O _
    //   X
    &FullWhenPovIsWestAndCloserToCamera,

    // b1001
    //   _
    // _ O X
    //   X
    &FullWhenPovIsWestAndCloserToCamera,

    // b1010
    //   X
    // _ O _
    //   X
    //&FullWhenPovIsWest,
    &FullWhenPovIsWest2,

    // b1011
    //   X
    // _ O X
    //   X
    &FullWhenPovIsWest,

    // b1100
    //   _
    // X O _
    //   X
    &AlwaysLowered,

    // b1101
    //   _
    // X O X
    //   X
    &AlwaysLowered,

    // b1110
    //   X
    // X O _
    //   X
    &AlwaysLowered,

    // b1111
    //   X
    // X O X
    //   X
    &AlwaysNone,
};
} // namespace
} // namespace predicate

CellModelExt::ExtensionData::ExtensionData()
    : _pointerStorage{&predicate::AlwaysNone} {}

void CellModelExt::ExtensionData::setRendererMask(std::uint16_t aMask) {
    _rendererMask = aMask;
}

std::uint16_t CellModelExt::ExtensionData::getRendererMask() const {
    return _rendererMask;
}

void CellModelExt::ExtensionData::setChunkExtensionPointer(
    ChunkExtensionInterface* aChunkExtensionPointer)
//
{
    _pointerStorage.chunkExtension = aChunkExtensionPointer;
    _holdingExtension              = true;
}

bool CellModelExt::ExtensionData::hasChunkExtensionPointer() const {
    return _holdingExtension;
}

ChunkExtensionInterface* CellModelExt::ExtensionData::getChunkExtensionPointer() const {
    return _pointerStorage.chunkExtension;
}

#if 0
namespace {
bool IsSolid(const CellModelExt* aCell) {
    return !aCell || aCell->isFloorInitialized();
}
} // namespace

void CellModelExt::ExtensionData::refresh(const CellModelExt* aNorthNeighbour,
                                          const CellModelExt* aWestNeighbour,
                                          const CellModelExt* aEastNeighbour,
                                          const CellModelExt* aSouthNeighbour) {
    HG_ASSERT(!_holdingExtension);

    const bool blockedFromNorth = IsSolid(aNorthNeighbour);
    const bool blockedFromWest  = IsSolid(aWestNeighbour);
    const bool blockedFromEast  = IsSolid(aEastNeighbour);
    const bool blockedFromSouth = IsSolid(aSouthNeighbour);

    const auto selector = (blockedFromEast ? 0x01 : 0) | (blockedFromNorth ? 0x02 : 0) |
                          (blockedFromWest ? 0x04 : 0) | (blockedFromSouth ? 0x08 : 0);

    _pointerStorage.drawingDataPredicate = predicate::SELECTION_TABLE[static_cast<std::size_t>(selector)];
}
#endif

void CellModelExt::ExtensionData::refresh(const CellModelExt* aNorthNeighbour,
                                          const CellModelExt* aWestNeighbour,
                                          const CellModelExt* aEastNeighbour,
                                          const CellModelExt* aSouthNeighbour) {
    HG_ASSERT(!_holdingExtension);

    (void)aNorthNeighbour; // TODO
    (void)aWestNeighbour;
    (void)aEastNeighbour;
    (void)aSouthNeighbour;

    // TODO: horrible nonportable hack
    CellModelExt* cell = reinterpret_cast<CellModelExt*>(reinterpret_cast<char*>(this) -
                                                         offsetof(CellModelExt, mutableExtensionData));

    // clang-format off
    static constexpr auto FLAG_MASK =
        (CellModelExt::OBSTRUCTED_FULLY_BY_NORTH_NEIGHBOR |
         CellModelExt::OBSTRUCTED_FULLY_BY_WEST_NEIGHBOR  |
         CellModelExt::OBSTRUCTED_FULLY_BY_EAST_NEIGHBOR  |
         CellModelExt::OBSTRUCTED_FULLY_BY_SOUTH_NEIGHBOR ) &
        ~(CellModelExt::OBSTRUCTED_BY_NORTH_NEIGHBOR | 
          CellModelExt::OBSTRUCTED_BY_WEST_NEIGHBOR  |
          CellModelExt::OBSTRUCTED_BY_EAST_NEIGHBOR  |
          CellModelExt::OBSTRUCTED_BY_SOUTH_NEIGHBOR );
    // clang-format on

    static constexpr int FLAG_SHIFT = 12;

    static_assert(0xF << FLAG_SHIFT == FLAG_MASK);

    const auto selector = static_cast<std::size_t>((cell->getFlags() & FLAG_MASK) >> FLAG_SHIFT);

    HG_ASSERT(selector < predicate::SELECTION_TABLE.size());

    _pointerStorage.drawingDataPredicate = predicate::SELECTION_TABLE[selector];
}

DrawingData CellModelExt::ExtensionData::getDrawingData(float           aCellResolution,
                                                        PositionInWorld aCellTopLeft,
                                                        PositionInWorld aPointOfView) const {
    HG_ASSERT(!_holdingExtension);

    return _pointerStorage.drawingDataPredicate(aCellResolution, aCellTopLeft, aPointOfView);
}

} // namespace detail
} // namespace gridgoblin
} // namespace jbatnozic
