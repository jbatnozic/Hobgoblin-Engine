// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Private/Cell_model_ext.hpp>
#include <GridGoblin/Private/Light_model_ext.hpp>

namespace jbatnozic {
namespace gridgoblin {

// Cells

inline const detail::CellModelExt::ExtensionData& GetExtensionData(const CellModel& aCell) {
    return static_cast<const detail::CellModelExt&>(aCell).mutableExtensionData;
}

inline detail::CellModelExt::ExtensionData& GetMutableExtensionData(const CellModel& aCell) {
    return static_cast<const detail::CellModelExt&>(aCell).mutableExtensionData;
}

// Lights

inline const detail::LightModelExt::ExtensionData& GetExtensionData(const LightModel& aLight) {
    return static_cast<const detail::LightModelExt&>(aLight).mutableExtensionData;
}

inline detail::LightModelExt::ExtensionData& GetMutableExtensionData(const LightModel& aLight) {
    return static_cast<const detail::LightModelExt&>(aLight).mutableExtensionData;
}

} // namespace gridgoblin
} // namespace jbatnozic
