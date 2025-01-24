#pragma once

#include "Engine.hpp"

#include "Cell_kind.hpp"

#include <Hobgoblin/Alvin.hpp>
#include <Hobgoblin/ChipmunkPhysics.hpp>
#include <Hobgoblin/Math.hpp>

#include <optional>

enum EntityCategories {
    CAT_CHARACTER = 0x01,
    CAT_DIVER     = 0x01,
    CAT_TERRAIN   = 0x02,
    CAT_LOOT      = 0x04,
};

enum EntityIds {
    EID_CHARACTER,
    EID_DIVER,
    EID_TERRAIN,
    EID_LOOT
};

class CharacterInterface : public hg::alvin::EntityBase {
public:
    using EntitySuperclass = hg::alvin::EntityBase;

    static constexpr hg::alvin::EntityTypeId ENTITY_TYPE_ID = EID_CHARACTER;

    static constexpr cpBitmask ENTITY_DEFAULT_CATEGORY = CAT_CHARACTER;
    static constexpr cpBitmask ENTITY_DEFAULT_MASK     = CAT_CHARACTER | CAT_TERRAIN | CAT_LOOT;

    virtual bool getFling() const = 0;
    virtual void addProtein()     = 0;
};

class DiverInterface : public hg::alvin::EntityBase {
public:
    using EntitySuperclass = hg::alvin::EntityBase;

    static constexpr hg::alvin::EntityTypeId ENTITY_TYPE_ID = EID_DIVER;

    static constexpr cpBitmask ENTITY_DEFAULT_CATEGORY = CAT_DIVER;
    static constexpr cpBitmask ENTITY_DEFAULT_MASK     = CAT_DIVER | CAT_TERRAIN | CAT_LOOT;
};

class TerrainInterface : public hg::alvin::EntityBase {
public:
    using EntitySuperclass = hg::alvin::EntityBase;

    static constexpr hg::alvin::EntityTypeId ENTITY_TYPE_ID = EID_TERRAIN;

    static constexpr cpBitmask ENTITY_DEFAULT_CATEGORY = CAT_TERRAIN;
    static constexpr cpBitmask ENTITY_DEFAULT_MASK     = CAT_CHARACTER;

    virtual std::optional<CellKind> getCellKindOfShape(NeverNull<cpShape*> aShape) const = 0;
};

class LootInterface : public hg::alvin::EntityBase {
public:
    using EntitySuperclass = hg::alvin::EntityBase;

    static constexpr hg::alvin::EntityTypeId ENTITY_TYPE_ID = EID_LOOT;

    static constexpr cpBitmask ENTITY_DEFAULT_CATEGORY = CAT_LOOT;
    static constexpr cpBitmask ENTITY_DEFAULT_MASK     = CAT_CHARACTER;
};

void InitColliders(hg::alvin::MainCollisionDispatcher& aDispatcher, hg::NeverNull<cpSpace*> aSpace);
