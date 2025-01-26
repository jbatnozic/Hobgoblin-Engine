#pragma once

#include "Engine.hpp"

#include "Cell_kind.hpp"

#include <Hobgoblin/Alvin.hpp>
#include <Hobgoblin/ChipmunkPhysics.hpp>
#include <Hobgoblin/Math.hpp>

#include <optional>

enum EntityCategories {
    CAT_TERRAIN   = (1 << 0), //!< Terrain blocks
    CAT_CHARACTER = (1 << 1), //!< Divers, shark
    CAT_LOOT      = (1 << 2), //!< Pickups
};

enum EntityIds {
    EID_TERRAIN,
    EID_DIVER,
    EID_SHARK,
    EID_LOOT
};

class TerrainInterface : public hg::alvin::EntityBase {
public:
    using EntitySuperclass = hg::alvin::EntityBase;

    static constexpr hg::alvin::EntityTypeId ENTITY_TYPE_ID = EID_TERRAIN;

    static constexpr cpBitmask ENTITY_DEFAULT_CATEGORY = CAT_TERRAIN;
    static constexpr cpBitmask ENTITY_DEFAULT_MASK     = CAT_CHARACTER | CAT_LOOT;
};

class DiverInterface : public hg::alvin::EntityBase {
public:
    using EntitySuperclass = hg::alvin::EntityBase;

    static constexpr hg::alvin::EntityTypeId ENTITY_TYPE_ID = EID_DIVER;

    static constexpr cpBitmask ENTITY_DEFAULT_CATEGORY = CAT_CHARACTER;
    static constexpr cpBitmask ENTITY_DEFAULT_MASK     = CAT_CHARACTER | CAT_TERRAIN | CAT_LOOT;

    virtual void addOxygen(float aOxygen) = 0;
    virtual void kill() = 0;
};

class SharkInterface : public hg::alvin::EntityBase {
public:
    using EntitySuperclass = hg::alvin::EntityBase;

    static constexpr hg::alvin::EntityTypeId ENTITY_TYPE_ID = EID_SHARK;

    static constexpr cpBitmask ENTITY_DEFAULT_CATEGORY = CAT_CHARACTER;
    static constexpr cpBitmask ENTITY_DEFAULT_MASK     = CAT_CHARACTER | CAT_TERRAIN | CAT_LOOT;
};

class LootInterface : public hg::alvin::EntityBase {
public:
    using EntitySuperclass = hg::alvin::EntityBase;

    static constexpr hg::alvin::EntityTypeId ENTITY_TYPE_ID = EID_LOOT;

    static constexpr cpBitmask ENTITY_DEFAULT_CATEGORY = CAT_LOOT;
    static constexpr cpBitmask ENTITY_DEFAULT_MASK     = CAT_CHARACTER | CAT_TERRAIN;
};

void InitColliders(hg::alvin::MainCollisionDispatcher& aDispatcher, hg::NeverNull<cpSpace*> aSpace);
