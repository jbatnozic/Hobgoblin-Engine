// Copyright 2024 Jovan Batnozic. Released under MS-PL licence in Serbia.
// See https://github.com/jbatnozic/Hobgoblin?tab=readme-ov-file#licence

#include <GridGoblin/Private/Cell_model_ext.hpp>
#include <GridGoblin/Private/Light_ext.hpp>

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

inline const detail::LightExt::ExtensionData& GetExtensionData(const Light& aLight) {
    return static_cast<const detail::LightExt&>(aLight).mutableExtensionData;
}

inline detail::LightExt::ExtensionData& GetMutableExtensionData(const Light& aLight) {
    return static_cast<const detail::LightExt&>(aLight).mutableExtensionData;
}

} // namespace gridgoblin
} // namespace jbatnozic
