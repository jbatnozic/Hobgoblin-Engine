#include "Collisions.hpp"

void InitColliders(hg::alvin::MainCollisionDispatcher& aDispatcher, hg::NeverNull<cpSpace*> aSpace) {
    aDispatcher.registerEntityType<TerrainInterface>();
    aDispatcher.registerEntityType<DiverInterface>();
    aDispatcher.registerEntityType<SharkInterface>();
    aDispatcher.registerEntityType<LootInterface>();
    aDispatcher.bind(aSpace);
}
