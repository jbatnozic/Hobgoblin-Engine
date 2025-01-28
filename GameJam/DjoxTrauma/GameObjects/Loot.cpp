#include "Loot.hpp"

#include "Managers/Environment_manager_interface.hpp"
#include "Managers/Resource_manager_interface.hpp"
#include "Sprite_manifest.hpp"

#include <cmath>

namespace {
static constexpr cpFloat WIDTH  = 100.0;
static constexpr cpFloat HEIGHT = 100.0;
} // namespace

LootObject::LootObject(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId)
    : SyncObjSuper{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIORITY_LOOT, "LootObject", aRegId, aSyncId}
    , _unibody{[this]() {
                   return _initColDelegate();
               },
               [this]() {
                   return hg::alvin::Body::createKinematic();
               },
               [this]() {
                   return hg::alvin::Shape::createBox(_unibody.body, WIDTH, HEIGHT);
               }} {
    if (isMasterObject()) {
        _getCurrentState().initMirror(); // To get autodiff optimization working
        _unibody.bindDelegate(*this);
        _unibody.addToSpace(ccomp<MEnvironment>().getSpace());
    }
}

LootObject::~LootObject() {
    if (isMasterObject()) {
        doSyncDestroy();
    }
}

void LootObject::init(float aX, float aY, LootKind aKind) {
    HG_HARD_ASSERT(isMasterObject());

    auto& self = _getCurrentState();
    self.x     = aX;
    self.y     = aY;
    self.kind  = (std::int8_t)aKind;

    cpBodySetPosition(_unibody, cpv(aX, aY));
}

void LootObject::_eventUpdate1(spe::IfDummy) {
    _wiggleAngle += 0.1;
}

void LootObject::_eventPostUpdate(spe::IfMaster) {
    _getCurrentState().commit();
}

void LootObject::_eventDraw1() {
    if (this->isDeactivated()) {
        return;
    }

    auto& winMgr = ccomp<MWindow>();
    auto& canvas = winMgr.getCanvas();

    const auto& self = _getCurrentState();

    if (self.kind == (std::int8_t)LootKind::NONE) {
        return; // TODO(temp.)
    }

    if (_spr.getSubspriteCount() == 0) {
        _spr = ccomp<MResource>().getSpriteLoader().getMultiBlueprint(SPR_POWER).multispr();
    }

    _spr.setPosition(self.x, self.y);
    _spr.setScale(0.5f, 0.5f);
    _spr.setRotation(hg::math::AngleF::fromDegrees(15.f) * std::sinf(_wiggleAngle));
    canvas.draw(_spr);

    // Body bbox
    // {
    //     hg::gr::RectangleShape rect{
    //         {(float)WIDTH, (float)HEIGHT}
    //     };
    //     rect.setOrigin({(float)WIDTH / 2.f, (float)HEIGHT / 2.f});
    //     rect.setFillColor(hg::gr::COLOR_GREEN);
    //     rect.setPosition(self.x, self.y);
    //     canvas.draw(rect);
    // }
}

void LootObject::_eventDraw2() {
#if 0
    if (this->isDeactivated())
        return;

    const auto& self = _getCurrentState();

    auto& lobbyBackend = ccomp<MLobbyBackend>();
    if (self.owningPlayerIndex > 0) {
        const auto&  name = lobbyBackend.getLockedInPlayerInfo(self.owningPlayerIndex).name;
        hg::gr::Text text{hg::gr::BuiltInFonts::getFont(hg::gr::BuiltInFonts::TITILLIUM_REGULAR),
                          name,
                          40};
        text.setFillColor(hg::gr::COLOR_WHITE);
        text.setOutlineColor(hg::gr::COLOR_BLACK);
        text.setOutlineThickness(2.f);
        const auto& localBounds = text.getLocalBounds();
        text.setOrigin({localBounds.w / 2.f, localBounds.h / 2.f});
        text.setPosition({self.x, self.y - 120.f});
        ccomp<MWindow>().getCanvas().draw(text);
    }
#endif
}

hg::alvin::CollisionDelegate LootObject::_initColDelegate() {
    auto builder = hg::alvin::CollisionDelegateBuilder{};
    // builder.addInteraction<CharacterInterface>(
    //     hg::alvin::COLLISION_PRE_SOLVE,
    //     [this](CharacterInterface& aCharacter, const hg::alvin::CollisionData& aCollisionData) {
    //         auto& self = _getCurrentState();
    //         switch ((LootKind)self.kind) {
    //         case LootKind::PROTEIN:
    //             aCharacter.addProtein();
    //             self.kind = (std::int8_t)LootKind::NONE;
    //             break;

    //         default:
    //             break;
    //         };
    //         return hg::alvin::Decision::ACCEPT_COLLISION;
    //     });

    return builder.finalize();
}

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(LootObject, (CREATE, UPDATE, DESTROY));

void LootObject::_syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(LootObject, aSyncCtrl);
}

void LootObject::_syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(LootObject, aSyncCtrl);
}

void LootObject::_syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(LootObject, aSyncCtrl);
}
