#include "Collisions.hpp"

void InitColliders(hg::alvin::MainCollisionDispatcher& aDispatcher, hg::NeverNull<cpSpace*> aSpace) {
    aDispatcher.registerEntityType<CharacterInterface>();
    aDispatcher.registerEntityType<TerrainInterface>();
    aDispatcher.registerEntityType<LootInterface>();
    aDispatcher.bind(aSpace);
}