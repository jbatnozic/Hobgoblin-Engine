#include "Shark.hpp"

#include "Config.hpp"
#include "Environment_manager.hpp"
#include "Main_gameplay_manager_interface.hpp"
#include "Player_controls.hpp"
#include "Resource_manager_interface.hpp"
#include "Sprite_manifest.hpp"
#include "Varmap_ids.hpp"

#include <Hobgoblin/HGExcept.hpp>
#include <Hobgoblin/Math.hpp>

#include <cmath>

namespace {
// Physics parameters for Shark
static constexpr cpFloat PHYS_MASS                   = 800.0;
static constexpr cpFloat PHYS_RADIUS                 = 48.0;
static constexpr cpFloat PHYS_DAMPING                = 0.95;
static constexpr cpFloat PHYS_ROTATIONAL_FORCE       = 1'750'000.0;
static constexpr cpFloat PHYS_PROPULSION_FORCE_MIN   = 15'000.0;
static constexpr cpFloat PHYS_PROPULSION_FORCE_MAX   = 1400'000.0;
static constexpr cpFloat PHYS_PROPULSION_FORCE_STEPS = 350.0;
} // namespace

Shark::Shark(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId)
    : SyncObjSuper{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIORITY_PLAYERAVATAR, "Shark", aRegId, aSyncId}
    , _propulsionForce{PHYS_PROPULSION_FORCE_MIN} //
{
    if (isMasterObject()) {
        _getCurrentState().initMirror(); // To get autodiff optimization working

        _unibody.init(
            [this]() {
                return _initColDelegate();
            },
            [this]() {
                return hg::alvin::Body::createDynamic(
                    PHYS_MASS,
                    cpMomentForCircle(PHYS_MASS, 0.0, PHYS_RADIUS, cpvzero));
            },
            [this]() {
                return hg::alvin::Shape::createCircle(_unibody.body, PHYS_RADIUS, cpvzero);
            });
        _unibody.bindDelegate(*this);
        _unibody.addToSpace(ccomp<MEnvironment>().getSpace());

        cpBodySetVelocityUpdateFunc(
            _unibody,
            [](cpBody* aBody, cpVect aGravity, cpFloat /*aDamping*/, cpFloat aDt) {
                cpBodyUpdateVelocity(aBody, aGravity, PHYS_DAMPING, aDt);
            });
    } else {
        auto& resMgr = ccomp<ResourceManagerInterface>();
        _bodySprite  = resMgr.getSpriteLoader().getMultiBlueprint(SPR_KRAKEN_BODY).multispr();
        _finsSprite  = resMgr.getSpriteLoader().getMultiBlueprint(SPR_KRAKEN_FINS).multispr();
    }
}

Shark::~Shark() {
    if (isMasterObject()) {
        doSyncDestroy();
    }
}

void Shark::init(int aOwningPlayerIndex, float aX, float aY) {
    HG_HARD_ASSERT(isMasterObject());

    auto& self             = _getCurrentState();
    self.owningPlayerIndex = aOwningPlayerIndex;
    self.x                 = aX;
    self.y                 = aY;
    self.directionInRad    = 0.f;

    cpBodySetPosition(_unibody, cpv(aX, aY));
}

void Shark::_eventUpdate1(spe::IfMaster) {
    if (ctx().getGameState().isPaused) {
        return;
    }

    auto& self = _getCurrentState();
    HG_HARD_ASSERT(self.owningPlayerIndex >= 0);

    auto& lobbyBackend = ccomp<MLobbyBackend>();
    if (const auto clientIndex = lobbyBackend.playerIdxToClientIdx(self.owningPlayerIndex);
        clientIndex != spe::CLIENT_INDEX_UNKNOWN) {

        spe::InputSyncManagerWrapper wrapper{ccomp<MInput>()};

        const auto left  = wrapper.getSignalValue<ControlDirectionType>(clientIndex, CTRL_ID_LEFT);
        const auto right = wrapper.getSignalValue<ControlDirectionType>(clientIndex, CTRL_ID_RIGHT);

        const auto up   = wrapper.getSignalValue<ControlDirectionType>(clientIndex, CTRL_ID_UP);
        const auto down = wrapper.getSignalValue<ControlDirectionType>(clientIndex, CTRL_ID_DOWN);

        const auto gameStage = ccomp<MainGameplayManagerInterface>().getCurrentGameStage();
        if (gameStage >= GAME_STAGE_HIDE_N_SEEK && gameStage < GAME_STAGE_FINISHED) {
            _execMovement(left, right, up, down);
        }

        const auto pos      = cpBodyGetPosition(_unibody);
        self.x              = static_cast<float>(pos.x);
        self.y              = static_cast<float>(pos.y);
        const auto rot      = cpBodyGetRotation(_unibody);
        self.directionInRad = hg::math::AngleF::fromVector({(float)rot.x, (float)rot.y}).asRadians();

        if (clientIndex != spe::CLIENT_INDEX_LOCAL) {
            ccomp<MainGameplayManagerInterface>().setPositionOfClient(clientIndex, pos);
        }
    }
}

void Shark::_eventUpdate1(spe::IfDummy) {
    if (this->isDeactivated() || ctx().getGameState().isPaused) {
        return;
    }

    auto& self = _getCurrentState();

    auto& lobbyBackend = ccomp<MLobbyBackend>();
    const auto localPlayerIndex = lobbyBackend.getLocalPlayerIndex();
    if (localPlayerIndex == self.owningPlayerIndex) {
        _adjustView();

        auto& varmap = ccomp<spe::SyncedVarmapManagerInterface>();
        const auto cs = varmap.getString(VARMAP_ID_PLAYER_STATUS + std::to_string(localPlayerIndex));
        if (cs != "(Kraken)") {
            varmap.requestToSetString(VARMAP_ID_PLAYER_STATUS + std::to_string(localPlayerIndex), "(Kraken)");
        }
    }
}

void Shark::_eventPostUpdate(spe::IfMaster) {
    _getCurrentState().commit();
}

void Shark::_eventDraw1() {
    if (this->isDeactivated()) {
        return;
    }

    auto& winMgr = ccomp<MWindow>();
    auto& canvas = winMgr.getCanvas();

    const auto& self = _getCurrentState();

    _bodySprite.selectSubsprite(0);
    _bodySprite.setPosition({self.x, self.y});
    _bodySprite.setRotation(hg::math::AngleF::fromRad(self.directionInRad + hg::math::Pi<float>() / 2.f));
    canvas.draw(_bodySprite);

    //_finsSprite.selectSubsprite(0);
    _finsSprite.advanceSubsprite(0.2f);
    _finsSprite.setPosition({self.x, self.y});
    _finsSprite.setRotation(hg::math::AngleF::fromRad(self.directionInRad + hg::math::Pi<float>() / 2.f));
    canvas.draw(_finsSprite);

    hg::gr::CircleShape cir{(float)PHYS_RADIUS};
    cir.setFillColor(hg::gr::COLOR_TRANSPARENT);
    cir.setOutlineColor(hg::gr::COLOR_YELLOW);
    cir.setOutlineThickness(2.f);
    cir.setOrigin({(float)PHYS_RADIUS, (float)PHYS_RADIUS});
    cir.setPosition(self.x, self.y);
    canvas.draw(cir);

    // hg::gr::CircleShape eye{8.f};
    // eye.setFillColor(hg::gr::COLOR_RED);
    // eye.setOrigin({8.f, 32.f});
    // eye.setPosition(self.x, self.y);
    // eye.setRotation(hg::math::AngleF::fromRadians(self.directionInRad));
    // canvas.draw(eye);
}

void Shark::_eventDraw2() {
    if (this->isDeactivated())
        return;

    // TODO: draw player name
    //  const auto& self = _getCurrentState();

    // auto& lobbyBackend = ccomp<MLobbyBackend>();
    // if (self.owningPlayerIndex > 0) {
    //     const auto&  name = lobbyBackend.getLockedInPlayerInfo(self.owningPlayerIndex).name;
    //     hg::gr::Text text{hg::gr::BuiltInFonts::getFont(hg::gr::BuiltInFonts::TITILLIUM_REGULAR),
    //                       name,
    //                       30};
    //     text.setScale({2.f, 2.f});
    //     text.setFillColor(hg::gr::COLOR_WHITE);
    //     text.setOutlineColor(hg::gr::COLOR_BLACK);
    //     text.setOutlineThickness(4.f);
    //     const auto& localBounds = text.getLocalBounds();
    //     text.setOrigin({localBounds.w / 2.f, localBounds.h / 2.f});
    //     text.setPosition({self.x, self.y - 180.f});
    //     ccomp<MWindow>().getCanvas().draw(text);
    // }
}

void Shark::_execMovement(bool aLeft, bool aRight, bool aUp, bool aDown) {
    // Rotation
    if (aLeft || aRight || aUp || aDown) {
        const cpFloat lr = static_cast<cpFloat>(aRight) - static_cast<cpFloat>(aLeft);
        const cpFloat ud = static_cast<cpFloat>(aDown) - static_cast<cpFloat>(aUp);

        const auto currentRotationVec = cpBodyGetRotation(_unibody);
        const auto currentRotation =
            hg::math::Angle<cpFloat>::fromVector(currentRotationVec.x, currentRotationVec.y);

        const auto targetRotation =
            hg::math::Angle<cpFloat>::fromVector({lr, ud}) - hg::math::Angle<cpFloat>::fromDeg(90.0);

        const auto rotationDiff = currentRotation.shortestDistanceTo(targetRotation);

        cpVect rotationalForce;
        if (rotationDiff.asRadians() < 0.0) {
            rotationalForce = cpv(0.0, -PHYS_ROTATIONAL_FORCE);
        } else {
            rotationalForce = cpv(0.0, +PHYS_ROTATIONAL_FORCE);
        }

        rotationalForce =
            rotationalForce * (std::abs(rotationDiff.asRadians()) / hg::math::Pi<cpFloat>());

        cpBodyApplyForceAtLocalPoint(_unibody,
                                     cpvneg(rotationalForce),
                                     cpv(24.0, 0.0)); // Idk why 12 but it works
        cpBodyApplyForceAtLocalPoint(_unibody, rotationalForce, cpvzero);
    }

    // Propulsion
    static constexpr cpFloat PROPULSION_FORCE_DELTA =
        (PHYS_PROPULSION_FORCE_MAX - PHYS_PROPULSION_FORCE_MIN) / PHYS_PROPULSION_FORCE_STEPS;
    if (aLeft || aRight || aUp || aDown) {
        if (_propulsionForce < PHYS_PROPULSION_FORCE_MAX) {
            _propulsionForce += PROPULSION_FORCE_DELTA;
        }

        const auto   angleRad = cpvtoangle(cpBodyGetRotation(_unibody)) - hg::math::Pi<cpFloat>() / 2;
        const cpVect force    = cpvmult(cpvforangle(angleRad), _propulsionForce);
        cpBodyApplyForceAtWorldPoint(_unibody, force, cpBodyGetPosition(_unibody));
    } else {
        if (_propulsionForce > PHYS_PROPULSION_FORCE_MIN) {
            _propulsionForce -= PROPULSION_FORCE_DELTA;
        }
    }
}

bool Shark::_isInMouth(const cpVect& aPosition) const {
    const auto currentRotationVec = cpBodyGetRotation(_unibody);
    const auto currentRotation =
        hg::math::Angle<cpFloat>::fromVector(currentRotationVec.x, currentRotationVec.y) +
        hg::math::Angle<cpFloat>::fromDeg(90.0);

    const auto currentPosition = cpBodyGetPosition(_unibody);
    const auto targetRotation =
        hg::math::PointDirection(currentPosition.x, currentPosition.y, aPosition.x, aPosition.y);

    HG_LOG_INFO(LOG_ID,
                "_isInMouth() - aPosition = ({}, {}); currentPosition = ({}, {}); currentRotation = "
                "{}deg; targetRotation = {}deg.",
                aPosition.x,
                aPosition.y,
                currentPosition.x,
                currentPosition.y,
                currentRotation.asDeg(),
                targetRotation.asDeg());

    return std::abs(currentRotation.shortestDistanceTo(targetRotation).asDeg()) <= 30.0;
}

hg::alvin::CollisionDelegate Shark::_initColDelegate() {
    auto builder = hg::alvin::CollisionDelegateBuilder{};
    builder.setDefaultDecision(hg::alvin::Decision::ACCEPT_COLLISION);

    builder.addInteraction<DiverInterface>(
        hg::alvin::COLLISION_CONTACT,
        [this](DiverInterface& aDiver, const hg::alvin::CollisionData& aCollisionData) {
            const auto contactPoints = cpArbiterGetContactPointSet(aCollisionData.arbiter);
            for (int i = 0; i < contactPoints.count; i += 1) {
                if (_isInMouth(contactPoints.points[i].pointA) ||
                    _isInMouth(contactPoints.points[i].pointB)) {
                    aDiver.kill();
                    break;
                }
            }
            return hg::alvin::Decision::ACCEPT_COLLISION;
        });

    return builder.finalize();
}

namespace {
hg::math::AngleF PointDirection2(hg::math::Vector2f aFrom, hg::math::Vector2f aTo) {
    return hg::math::PointDirection(aFrom.x, aFrom.y, aTo.x, aTo.y);
}
} // namespace

void Shark::_adjustView() {
    auto& self = _getCurrentState();
    auto& view = ccomp<MWindow>().getView(0);
#if 1
    auto targetPos = sf::Vector2f{self.x, self.y};
    // if (self.direction & DIRECTION_RIGHT) {
    //     targetPos.x += CAMERA_OFFSET;
    // }
    // if (self.direction & DIRECTION_LEFT) {
    //     targetPos.x -= CAMERA_OFFSET;
    // }
    // if (self.direction & DIRECTION_DOWN) {
    //     targetPos.y += CAMERA_OFFSET;
    // }
    // if (self.direction & DIRECTION_UP) {
    //     targetPos.y -= CAMERA_OFFSET;
    // }

    // {
    //     auto gameOver = ccomp<MVarmap>().getInt64(VARMAP_ID_GAME_OVER);
    //     if (gameOver.has_value() && *gameOver == 1) {
    //         const auto scalesPos = ccomp<MEnvironment>().getScalesGridPosition();
    //         targetPos            = {static_cast<float>((scalesPos.x + 2.f) * single_terrain_size),
    //                                 static_cast<float>(scalesPos.y * single_terrain_size)};
    //     }
    // }

    const auto viewCenter = view.getCenter();
    const auto dist =
        hg::math::EuclideanDist<float>(viewCenter.x, viewCenter.y, targetPos.x, targetPos.y);
    const auto theta = PointDirection2(view.getCenter(), targetPos).asRadians();

    if (dist >= 1000.0 || dist < 2.f) {
        view.setCenter(targetPos);
    } else if (dist >= 2.f) {
        view.move(+std::cosf(theta) * dist * 0.045f, -std::sinf(theta) * dist * 0.045f);
    }

    {
        auto&       envMgr = ccomp<MEnvironment>();
        const float minX   = view.getSize().x / 2.f;
        const float minY   = view.getSize().y / 2.f;
        const float maxX   = envMgr.getGridSize().x * single_terrain_size - minX;
        const float maxY   = envMgr.getGridSize().y * single_terrain_size - minY;

        auto pos = view.getCenter();
        if (pos.x < minX) {
            pos.x = minX;
        } else if (pos.x >= maxX) {
            pos.x = maxX - 1.f;
        }
        if (pos.y < minY) {
            pos.y = minY;
        } else if (pos.y >= maxY) {
            pos.y = maxY - 1.f;
        }
        view.setCenter(pos);
    }
#else
    view.setCenter({300.f, 300.f});
#endif

    // Round
    const auto center = view.getCenter();
    view.setCenter(std::roundf(center.x), std::roundf(center.y));
}

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(Shark, (CREATE, UPDATE, DESTROY));

void Shark::_syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(Shark, aSyncCtrl);
}

void Shark::_syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(Shark, aSyncCtrl);
}

void Shark::_syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(Shark, aSyncCtrl);
}
