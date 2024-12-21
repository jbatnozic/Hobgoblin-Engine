#pragma once

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/Graphics/Render_texture.hpp>
#include <Hobgoblin/Graphics/Sprite_loader.hpp>
#include <Hobgoblin/Math.hpp>
#include <Hobgoblin/Utility/Grids.hpp>
#include <Hobgoblin/Utility/Semaphore.hpp>

#include <GridWorld/Model/Cell.hpp>
#include <GridWorld/Model/Chunk.hpp>
#include <GridWorld/Model/Chunk_id.hpp>
#include <GridWorld/Model/Lights.hpp>
#include <GridWorld/Model/Sprites.hpp>

#include <GridWorld/World/Active_area.hpp>
#include <GridWorld/World/Binder.hpp>
#include <GridWorld/World/World_config.hpp>

#include <GridWorld/Private/Chunk_storage_handler.hpp>

#include <memory>
#include <optional>
#include <unordered_map>

namespace gridworld {

namespace hg = jbatnozic::hobgoblin;

class IsometricRenderer;

namespace detail {
class ChunkDiskIoHandlerInterface;
class ChunkSpoolerInterface;
} // namespace detail

class World : private Binder {
public:
    World(const WorldConfig& aConfig);

    World(const WorldConfig&                                  aConfig,
          hg::NeverNull<detail::ChunkDiskIoHandlerInterface*> aChunkDiskIoHandler);

#ifdef FUTURE
    World(const WorldConfig& aConfig, hg::NeverNull<detail::ChunkSpoolerInterface*> aChunkSpooler);
#endif

    ~World() override;

    void setBinder(Binder* aBinder) {
        _binder = aBinder;
    }

    // TODO: Cell height (z)

    void update();
    void prune();

    ///////////////////////////////////////////////////////////////////////////
    // CONVERSIONS                                                           //
    ///////////////////////////////////////////////////////////////////////////

    // Use the following methods to convert from a position in the world to the corresponding
    // cell. For example, if the cell resolution (as defined by the world config) is 32.0f,
    // positions >= 0.0f and < 32.0f belong to cell 0, positions >= 32.0f and < 64.0f belong
    // to cell 1, and so on (the same principle applies to both X and Y axis).

    hg::math::Vector2pz posToCell(float aX, float aY) const;

    hg::math::Vector2pz posToCell(hg::math::Vector2f aPos) const;

    hg::math::Vector2pz posToCellUnchecked(float aX, float aY) const;

    hg::math::Vector2pz posToCellUnchecked(hg::math::Vector2f aPos) const;

    // Use the following methods to convert from the coordinates of a single cell to the
    // corresponding chunk. For example, if the chunk width (as defined by the world config) is
    // 8, then cells with coordinates [0, y]-[7, y] belong to chunk [0, Y]; cells with
    // coordinates [8, y]-[15, y] belong to chunk [1, Y]; and so on. If the chunk height
    // is also 8, then cells with coordinates [x, 0]-[x, 7] belong to chunk [X, 0]; and so on.

    ChunkId cellToChunkId(hg::PZInteger aX, hg::PZInteger aY) const;

    ChunkId cellToChunkId(hg::math::Vector2pz aCell) const;

    ChunkId cellToChunkIdUnchecked(hg::PZInteger aX, hg::PZInteger aY) const;

    ChunkId cellToChunkIdUnchecked(hg::math::Vector2pz aCell) const;

    ///////////////////////////////////////////////////////////////////////////
    // LOCKING                                                               //
    ///////////////////////////////////////////////////////////////////////////

    class EditPermission {
    public:
        ~EditPermission();

    private:
        EditPermission() = default;
        friend class World;
    };

    std::unique_ptr<EditPermission> getPermissionToEdit();

    ///////////////////////////////////////////////////////////////////////////
    // CELL GETTERS                                                          //
    ///////////////////////////////////////////////////////////////////////////

    float getCellResolution() const;

    hg::PZInteger getCellCountX() const;

    hg::PZInteger getCellCountY() const;

    // Without locking (no on-demand loading)

    const CellModel* getCellAt(hg::PZInteger aX, hg::PZInteger aY) const;

    const CellModel* getCellAt(hg::math::Vector2pz aCell) const;

    const CellModel* getCellAtUnchecked(hg::PZInteger aX, hg::PZInteger aY) const;

    const CellModel* getCellAtUnchecked(hg::math::Vector2pz aCell) const;

    // With locking (on-demand loading enabled)

    const CellModel& getCellAt(const EditPermission& aPerm, hg::PZInteger aX, hg::PZInteger aY) const;

    const CellModel& getCellAt(const EditPermission& aPerm, hg::math::Vector2pz aCell) const;

    const CellModel& getCellAtUnchecked(const EditPermission& aPerm,
                                        hg::PZInteger         aX,
                                        hg::PZInteger         aY) const;

    const CellModel& getCellAtUnchecked(const EditPermission& aPerm, hg::math::Vector2pz aCell) const;

    ///////////////////////////////////////////////////////////////////////////
    // CELL UPDATERS                                                         //
    ///////////////////////////////////////////////////////////////////////////

    class Editor {
    public:
        // Floor

        void setFloorAt(hg::PZInteger                          aX,
                        hg::PZInteger                          aY,
                        const std::optional<CellModel::Floor>& aFloorOpt);

        void setFloorAt(hg::math::Vector2pz aCell, const std::optional<CellModel::Floor>& aFloorOpt);

        void setFloorAtUnchecked(hg::PZInteger                          aX,
                                 hg::PZInteger                          aY,
                                 const std::optional<CellModel::Floor>& aFloorOpt);

        void setFloorAtUnchecked(hg::math::Vector2pz                    aCell,
                                 const std::optional<CellModel::Floor>& aFloorOpt);

        // Wall

        void setWallAt(hg::PZInteger                         aX,
                       hg::PZInteger                         aY,
                       const std::optional<CellModel::Wall>& aWallOpt);

        void setWallAt(hg::math::Vector2pz aCell, const std::optional<CellModel::Wall>& aWallOpt);

        void setWallAtUnchecked(hg::PZInteger                         aX,
                                hg::PZInteger                         aY,
                                const std::optional<CellModel::Wall>& aWallOpt);

        void setWallAtUnchecked(hg::math::Vector2pz                   aCell,
                                const std::optional<CellModel::Wall>& aWallOpt);

    private:
        friend class World;
        Editor(World& aWorld) : _world{aWorld} {}
        World& _world;
    };

    // TODO: how to solve cache overflow in very large edits?
    template <class taCallable>
    void edit(const EditPermission&, taCallable&& aCallable) {
        _startEdit();
        Editor editor{*this};
        aCallable(editor);
        _endEdit();
    }

    ///////////////////////////////////////////////////////////////////////////
    // CHUNKS                                                                //
    ///////////////////////////////////////////////////////////////////////////

    hg::PZInteger getChunkCountX() const;

    hg::PZInteger getChunkCountY() const;

    // Without locking (no on-demand loading)

    const Chunk* getChunkAt(hg::PZInteger aX, hg::PZInteger aY) const;

    const Chunk* getChunkAtUnchecked(hg::PZInteger aX, hg::PZInteger aY) const;

    const Chunk* getChunkAt(hg::math::Vector2pz aCell) const;

    const Chunk* getChunkAtUnchecked(hg::math::Vector2pz aCell) const;

    const Chunk* getChunkAtId(ChunkId aChunkId) const;

    const Chunk* getChunkAtIdUnchecked(ChunkId aChunkId) const;

    // With locking (on-demand loading enabled)

    const Chunk& getChunkAt(const EditPermission& aPerm, hg::PZInteger aX, hg::PZInteger aY) const;

    const Chunk& getChunkAtUnchecked(const EditPermission& aPerm,
                                     hg::PZInteger         aX,
                                     hg::PZInteger         aY) const;

    const Chunk& getChunkAt(const EditPermission& aPerm, hg::math::Vector2pz aCell) const;

    const Chunk& getChunkAtUnchecked(const EditPermission& aPerm, hg::math::Vector2pz aCell) const;

    const Chunk& getChunkAtId(const EditPermission& aPerm, ChunkId aChunkId) const;

    const Chunk& getChunkAtIdUnchecked(const EditPermission& aPerm, ChunkId aChunkId) const;

    // TODO: iterate over all loaded chunks

    ///////////////////////////////////////////////////////////////////////////
    // ACTIVE AREAS                                                          //
    ///////////////////////////////////////////////////////////////////////////

    ActiveArea createActiveArea();

    ///////////////////////////////////////////////////////////////////////////
    // LIGHTS                                                                //
    ///////////////////////////////////////////////////////////////////////////

    LightId createLight(SpriteId aSpriteId, hg::math::Vector2pz aSize);

    void updateLight(LightId aLightHandle, hg::math::Vector2f aPosition, hg::math::AngleF aAngle);

    void destroyLight(LightId aLightHandle);

    // TODO(createRAIILight())

    // TODO(this shouldn't be exposed like this)
    std::unordered_map<LightId, detail::LightModelExt>::const_iterator lightDataBegin() const {
        return _lights.begin();
    }

    // TODO(this shouldn't be exposed like this)
    std::unordered_map<LightId, detail::LightModelExt>::const_iterator lightDataEnd() const {
        return _lights.end();
    }

private:
    friend class ActiveArea;

    // ===== Config =====

    struct WorldConfigExt : WorldConfig {
        hg::PZInteger cellCountX;
        hg::PZInteger cellCountY;

        WorldConfigExt(const WorldConfig& aConfig)
            : WorldConfig{aConfig}
            , cellCountX{chunkCountX * cellsPerChunkX}
            , cellCountY{chunkCountY * cellsPerChunkY} {}
    };

    WorldConfigExt _config;

    // ===== Subcomponents =====

    std::unique_ptr<detail::ChunkDiskIoHandlerInterface> _internalChunkDiskIoHandler;
    detail::ChunkDiskIoHandlerInterface* _chunkDiskIoHandler;

    std::unique_ptr<detail::ChunkSpoolerInterface> _internalChunkSpooler;
    hg::NeverNull<detail::ChunkSpoolerInterface*> _chunkSpooler;

    detail::ChunkStorageHandler _chunkStorage;

    void _connectSubcomponents();
    void _disconnectSubcomponents();

    // ===== Edit Permissions =====

    // hg::util::Semaphore _editPermSemaphore{1};

    // ===== Lights =====

    std::unordered_map<LightId, detail::LightModelExt> _lights;
    LightId                                            _lightIdCounter = 0;

    // ===== Other =====

    Binder* _binder = nullptr;

    void onChunkLoaded(ChunkId aChunkId, const Chunk* aChunk) override;
    void onChunkUnloaded(ChunkId aChunkId) override;
    std::unique_ptr<ChunkExtensionInterface> createChunkExtension() override {
        if (!_binder) {
            return nullptr;
        }
        return _binder->createChunkExtension();
    }

    // ===== Editing cells =====

    int _editMinX = -1;
    int _editMinY = -1;
    int _editMaxX = -1;
    int _editMaxY = -1;

    void _startEdit();
    void _endEdit();

    struct RingAssessment {
        bool hasOccupiedCells = false;
        bool extend           = false;
    };

    template <bool taAllowedToLoadAdjacent>
    RingAssessment _assessRing(hg::PZInteger aX, hg::PZInteger aY, hg::PZInteger aRing);

    // TODO: since we determine openness at chunk load, this will lead into endless chunk load loop
    // TODO: it's possible to optimize in case of PLACING A NEW WALL
    template <bool taAllowedToLoadAdjacent>
    hg::PZInteger _calcOpennessAt(hg::PZInteger aX, hg::PZInteger aY);

    void _refreshCellAtUnchecked(hg::PZInteger aX, hg::PZInteger aY);

    void _setFloorAt(hg::PZInteger                          aX,
                     hg::PZInteger                          aY,
                     const std::optional<CellModel::Floor>& aFloorOpt);

    void _setFloorAt(hg::math::Vector2pz aCell, const std::optional<CellModel::Floor>& aFloorOpt);

    void _setFloorAtUnchecked(hg::PZInteger                          aX,
                              hg::PZInteger                          aY,
                              const std::optional<CellModel::Floor>& aFloorOpt);

    void _setFloorAtUnchecked(hg::math::Vector2pz                    aCell,
                              const std::optional<CellModel::Floor>& aFloorOpt);

    void _setWallAt(hg::PZInteger aX, hg::PZInteger aY, const std::optional<CellModel::Wall>& aWallOpt);

    void _setWallAt(hg::math::Vector2pz aCell, const std::optional<CellModel::Wall>& aWallOpt);

    void _setWallAtUnchecked(hg::PZInteger                         aX,
                             hg::PZInteger                         aY,
                             const std::optional<CellModel::Wall>& aWallOpt);

    void _setWallAtUnchecked(hg::math::Vector2pz aCell, const std::optional<CellModel::Wall>& aWallOpt);
};

///////////////////////////////////////////////////////////////////////////
// CELL UPDATES                                                          //
///////////////////////////////////////////////////////////////////////////

// Floor

inline void World::Editor::setFloorAt(hg::PZInteger                          aX,
                                      hg::PZInteger                          aY,
                                      const std::optional<CellModel::Floor>& aFloorOpt) {
    _world._setFloorAt(aX, aY, aFloorOpt);
}

inline void World::Editor::setFloorAt(hg::math::Vector2pz                    aCell,
                                      const std::optional<CellModel::Floor>& aFloorOpt) {
    _world._setFloorAt(aCell, aFloorOpt);
}

inline void World::Editor::setFloorAtUnchecked(hg::PZInteger                          aX,
                                               hg::PZInteger                          aY,
                                               const std::optional<CellModel::Floor>& aFloorOpt) {
    _world._setFloorAtUnchecked(aX, aY, aFloorOpt);
}

inline void World::Editor::setFloorAtUnchecked(hg::math::Vector2pz                    aCell,
                                               const std::optional<CellModel::Floor>& aFloorOpt) {
    _world._setFloorAtUnchecked(aCell, aFloorOpt);
}

// Wall

inline void World::Editor::setWallAt(hg::PZInteger                         aX,
                                     hg::PZInteger                         aY,
                                     const std::optional<CellModel::Wall>& aWallOpt) {
    _world._setWallAt(aX, aY, aWallOpt);
}

inline void World::Editor::setWallAt(hg::math::Vector2pz                   aCell,
                                     const std::optional<CellModel::Wall>& aWallOpt) {
    _world._setWallAt(aCell, aWallOpt);
}

inline void World::Editor::setWallAtUnchecked(hg::PZInteger                         aX,
                                              hg::PZInteger                         aY,
                                              const std::optional<CellModel::Wall>& aWallOpt) {
    _world._setWallAtUnchecked(aX, aY, aWallOpt);
}

inline void World::Editor::setWallAtUnchecked(hg::math::Vector2pz                   aCell,
                                              const std::optional<CellModel::Wall>& aWallOpt) {
    _world._setWallAtUnchecked(aCell, aWallOpt);
}

} // namespace gridworld
