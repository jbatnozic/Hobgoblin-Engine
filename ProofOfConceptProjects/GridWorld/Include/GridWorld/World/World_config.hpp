#pragma once

#include <GridWorld/Model/Chunk_extension.hpp>

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/HGExcept.hpp>

#include <filesystem>
#include <functional>
#include <memory>

namespace gridworld {

namespace hg = ::jbatnozic::hobgoblin;

struct WorldConfig {
    //! Total count of chunks in the horizontal (X) direction.
    //! Must be between 1 and 4096.
    hg::PZInteger chunkCountX;
    //! Total count of chunks in the vertical (Y) direction.
    //! Must be between 1 and 4096.
    hg::PZInteger chunkCountY;

    //! Total count of cells in a single chunk in the horizontal (X) direction.
    //! Must be between 1 and 1024.
    hg::PZInteger cellsPerChunkX;
    //! Total count of cells in a single chunk in the vertical (Y) direction.
    //! Must be between 1 and 1024.
    hg::PZInteger cellsPerChunkY;

    //! The width and height of a single cell, in pixels. Must be positive.
    float cellResolution = 32.f;

    //! The 'openness' of a cell is defined as follows (openness = N, for brevity):
    //! - if `N == 0`, then the cell is solid.
    //!
    //! - if `N > 0 && IsOdd(N)`, then it is possible to place an object of size N x N or smaller
    //!   in this cell (such that their centers match) and it won't collide with any cells.
    //!
    //! - if `N > 0 && IsEven(N)`, then there exists at least one combination of numbers
    //!   xOff and yOff (xOff <= N/2, yOff <= N/2) such that it is possible to place an object 
    //!   of size N x N or smaller in this cell, in a way that the center of the cell matches the 
    //!   center of the object shifted by xOff along the X axis and by yOff along the Y axis.
    //! 
    //! \note in the above definition, when we say that an object is of size N x N cells, that
    //!       really means that it's of size N*cellResolution x N*cellResolution (pixels). The
    //!       same applies for offsets xOff and yOff - they are smaller than N*cellResolution/2.
    //! 
    //! \note cell openness is intended to be used for pathfinding algorithms: Paths of objects
    //!       that are up to 1 cell in width and height can pass through cells where openness is
    //!       1 or greater, objects that are up to 2 cells in width and height can pass through
    //!       cells whose openness is 2 or greater, and so on.
    //! 
    //! This field, `maxCellOpenness`, lets the openness algorithm know at which point to stop.
    //! Determining whether openness is 0 or 1 needs checking only a single cell, but checking
    //! whether it's 2 usually needs checking 8 more cells, then checking whether it's 3
    //! needs checking 16 more cells and so on. Because this gets increasingly more expensive
    //! for larger values, make sure to set `maxCellOpenness` to no more than the largest object
    //! you expect to have in your game world.
    //!
    //! \warning `maxCellOpenness` must be either 0, or an odd number (because the first step of
    //! the openness algorithm checks whether openness is 0 or 1, then the second step checks
    //! whether it's 2 or 3, and so on, so it's not possible to stop at an even number - unless
    //! that number is zero, in which case there's really no check being performed at all).
    //! `maxCellOpenness` must also NOT be greater than either `cellsPerChunkX` or `cellsPerChunkY`.
    hg::PZInteger maxCellOpenness = 0;

    //! TODO(description)
    hg::PZInteger maxLoadedNonessentialCells = 0;

    //! Directory from which to load chuks and to which to save them.
    std::filesystem::path chunkDirectoryPath = "";

    //! Method to check if a configuration object is valid.
    //! \throws hg::InvalidArgumentError if the object is not valid.
    //! \returns the same configuration object that was passed in.
    static WorldConfig& validate(WorldConfig& aConfig) {
        HG_VALIDATE_ARGUMENT(aConfig.chunkCountX >= 1 && aConfig.chunkCountX <= 4096);

        HG_VALIDATE_ARGUMENT(aConfig.chunkCountY >= 1 && aConfig.chunkCountY <= 4096);

        HG_VALIDATE_ARGUMENT(aConfig.cellsPerChunkX >= 1 && aConfig.cellsPerChunkX <= 1024);

        HG_VALIDATE_ARGUMENT(aConfig.cellsPerChunkY >= 1 && aConfig.cellsPerChunkY <= 1024);

        HG_VALIDATE_ARGUMENT(aConfig.cellResolution > 0.f);

        HG_VALIDATE_ARGUMENT(aConfig.maxCellOpenness == 0 || (aConfig.maxCellOpenness % 2 == 1));

        HG_VALIDATE_ARGUMENT(aConfig.maxCellOpenness <= aConfig.cellsPerChunkX &&
                             aConfig.maxCellOpenness <= aConfig.cellsPerChunkY);

        return aConfig;
    }

    //! Method to check if a configuration object is valid.
    //! \throws hg::InvalidArgumentError if the object is not valid.
    //! \returns the same configuration object that was passed in.
    static const WorldConfig& validate(const WorldConfig& aConfig) {
        return validate(const_cast<WorldConfig&>(aConfig));
    }
};

// TODO: config validation

} // namespace gridworld
