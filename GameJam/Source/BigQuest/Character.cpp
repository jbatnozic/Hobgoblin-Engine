#include "Character.hpp"

#include "Environment_manager.hpp"
#include "Main_gameplay_manager_interface.hpp"
#include "Player_controls.hpp"

#include <Hobgoblin/HGExcept.hpp>

#include "Config.hpp"
#include "Varmap_ids.hpp"
#include <Hobgoblin/Math.hpp>
#include <cmath>

namespace {
static constexpr cpFloat WIDTH  = 100.0;
static constexpr cpFloat HEIGHT = 200.0;

// For grabbing hitbox
static constexpr cpFloat RAY_X_OFFSET = +150.0;
static constexpr cpFloat RAY_Y_OFFSET = -130.0;

static constexpr float CAMERA_OFFSET = 512.f;

#define NUM_COLORS 12
static const hg::gr::Color COLORS[NUM_COLORS] = {hg::gr::COLOR_BLACK,
                                                 hg::gr::COLOR_RED,
                                                 hg::gr::COLOR_GREEN,
                                                 hg::gr::COLOR_YELLOW,
                                                 hg::gr::COLOR_BLUE,
                                                 hg::gr::COLOR_ORANGE,
                                                 hg::gr::COLOR_PURPLE,
                                                 hg::gr::COLOR_TEAL,
                                                 hg::gr::COLOR_BROWN,
                                                 hg::gr::COLOR_FUCHSIA,
                                                 hg::gr::COLOR_GREY,
                                                 hg::gr::COLOR_WHITE};
} // namespace

CharacterObject::CharacterObject(QAO_RuntimeRef aRuntimeRef, spe::RegistryId aRegId, spe::SyncId aSyncId)
    : SyncObjSuper{aRuntimeRef,
                   SPEMPE_TYPEID_SELF,
                   PRIORITY_PLAYERAVATAR,
                   "CharacterObject",
                   aRegId,
                   aSyncId}
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
    } else {
        _renderer.emplace(ctx(), hg::gr::COLOR_RED);
        _renderer->setMode(CharacterRenderer::Mode::STILL);
    }
}

CharacterObject::~CharacterObject() {
    if (isMasterObject()) {
        doSyncDestroy();
    }
}

void CharacterObject::init(int aOwningPlayerIndex, float aX, float aY) {
    HG_HARD_ASSERT(isMasterObject());

    auto& self             = _getCurrentState();
    self.x                 = aX;
    self.y                 = aY;
    self.owningPlayerIndex = aOwningPlayerIndex;

    cpBodySetPosition(_unibody, cpv(aX, aY));
}

bool CharacterObject::getFling() const {
    return jump;
}

void CharacterObject::addProtein() {
    _size += 1.f / 3.f;
    auto& self = _getCurrentState();
    self.size  = std::min((std::int8_t)std::roundf(_size), (std::int8_t)5);
}

void CharacterObject::_eventUpdate1(spe::IfMaster) {
    if (ctx().getGameState().isPaused)
        return;

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

        float finalX = 0;
        float finalY = 0;

        float xInput = 0;
        float yInput = 0;

        if (grounded) {
            vSpeed = 0;
            yInput = (float)down - (float)up;
            xInput = (float)right - (float)left;
        } else {
            vSpeed -= gravity;
        }

        if (xInput != 0 || yInput != 0) {
            auto angle  = hg::math::AngleF::fromVector({xInput, yInput});
            auto normal = angle.asNormalizedVector();

            finalX = character_speed * normal.x;
            finalY = character_speed * normal.y;
            wrapper.pollSimpleEvent(clientIndex, CTRL_ID_JUMP, [&]() {
                if (grounded && currentFlingCooldown <= 0) {
                    jumpDirection        = normal;
                    jump                 = true;
                    grounded             = false;
                    currentFlingCooldown = fling_timer;
                }
            });
        }
        if (!grounded) {
            finalX += jumpDirection.x * fling_speed;
            finalY += jumpDirection.y * fling_speed;
        } else {
            jumpDirection = {0.f, 0.f};
        }

        if (currentFlingCooldown > 0) {
            currentFlingCooldown--;
        }
        if (currentGroundTimer > 0) {
            currentGroundTimer--;
        }

        finalY -= vSpeed;

        bool  touchingTerrain = false;
        auto& space           = ccomp<MEnvironment>().getSpace();
        // space.runShapeQuery(_unibody.shape,
        //                     cpShapeFilterNew(0, CP_ALL_CATEGORIES, CAT_TERRAIN),
        //                     [&, this](const hg::alvin::ShapeQueryInfo&) {
        //                         touchingTerrain = true;
        //                     });
        space.runRaycastQuery(cpv(self.x - RAY_X_OFFSET, self.y + RAY_Y_OFFSET),
                              cpv(self.x + RAY_X_OFFSET, self.y + RAY_Y_OFFSET),
                              10.0,
                              cpShapeFilterNew(0, CP_ALL_CATEGORIES, CAT_TERRAIN),
                              [&, this](const hg::alvin::RaycastQueryInfo&) {
                                  touchingTerrain = true;
                              });

        if (!touchingTerrain) {
            if (currentGroundTimer <= 0 && grounded != false) {
                currentGroundTimer = fall_timer;
            }
            grounded = false;
        } else {
            if (currentGroundTimer <= 0 && currentFlingCooldown <= 0) {
                grounded = true;
                jump     = false;
            }
        }

        if (self.y > y_floor) {
            self.y   = y_floor;
            grounded = true;
            jump     = false;
        }

        // HG_LOG_INFO(LOG_ID, "SADKASD KASMD {}, {}", currentGroundTimer, currentFlingCooldown);

        self.x += finalX;
        self.y += finalY;

        {
            const auto worldGridSize = ccomp<MEnvironment>().getGridSize();
            const auto worldPadding  = 32.f;
            const auto worldMaxX =
                static_cast<float>(worldGridSize.x * single_terrain_size) - worldPadding;
            const auto worldMaxY =
                static_cast<float>(worldGridSize.y * single_terrain_size) - worldPadding;

            if (self.x < worldPadding) {
                self.x = worldPadding;
                finalX = std::max(finalX, 0.f);
            }
            if (self.x > worldMaxX) {
                self.x = worldMaxX;
                finalX = std::min(finalX, 0.f);
            }
            if (self.y < worldPadding) {
                self.y = worldPadding;
                finalY = std::max(finalY, 0.f);
            }
            if (self.y > worldMaxY) {
                self.y = worldMaxY;
                finalY = std::min(finalY, 0.f);
            }
        }

        auto& gpMgr = ccomp<MainGameplayManagerInterface>();
        if (gpMgr.getContender1() == this) {
            const auto scalesPos = ccomp<MEnvironment>().getScalesGridPosition();
            self.x               = scalesPos.x * single_terrain_size + 250.f;
            self.y               = scalesPos.y * single_terrain_size + 70.f;
            finalX               = 0;
            finalY               = 0;
        } else if (gpMgr.getContender2() == this) {
            const auto scalesPos = ccomp<MEnvironment>().getScalesGridPosition();
            self.x               = (scalesPos.x + 4.f) * single_terrain_size - 350.f;
            self.y               = scalesPos.y * single_terrain_size + 70.f;
            finalX               = 0;
            finalY               = 0;
        }

        cpBodySetPosition(_unibody, cpv(self.x, self.y));

        // Determine rendering mode
        if (std::abs(finalY) < 0.05f) {
            if (std::abs(finalX) < 0.05f) {
                self.renderMode = (std::int8_t)CharacterRenderer::Mode::STILL;
            } else {
                if (finalX > 0.f) {
                    self.renderMode = (std::int8_t)CharacterRenderer::Mode::CRAWL_HORIZONTAL_PLUS;
                } else {
                    self.renderMode = (std::int8_t)CharacterRenderer::Mode::CRAWL_HORIZONTAL_MINUS;
                }
            }
        } else {
            if (finalY < 0 && jump) {
                self.renderMode = (std::int8_t)CharacterRenderer::Mode::FLING;
            } else {
                self.renderMode = (std::int8_t)CharacterRenderer::Mode::CRAWL_VERTICAL;
            }
        }

        // Determine direction
        self.direction = DIRECTION_NONE;
        if (std::abs(finalX) >= 0.05f) {
            if (finalX > 0.0f) {
                self.direction |= DIRECTION_RIGHT;
            } else {
                self.direction |= DIRECTION_LEFT;
            }
        }
        if (std::abs(finalY) >= 0.05f) {
            if (finalY > 0.0f) {
                self.direction |= DIRECTION_DOWN;
            } else {
                self.direction |= DIRECTION_UP;
            }
        }
    }
}

void CharacterObject::_eventUpdate1(spe::IfDummy) {
    if (this->isDeactivated() || ctx().getGameState().isPaused) {
        return;
    }

    auto& self = _getCurrentState();
    _renderer->setMode((CharacterRenderer::Mode)self.renderMode);
    _renderer->setSize(self.size);
    _renderer->update();

    auto& lobbyBackend = ccomp<MLobbyBackend>();
    if (lobbyBackend.getLocalPlayerIndex() == self.owningPlayerIndex) {
        _adjustView();
    }
}

void CharacterObject::_eventPostUpdate(spe::IfMaster) {
    _getCurrentState().commit();
}

void CharacterObject::_eventDraw1() {
    if (this->isDeactivated()) {
        return;
    }

    auto& winMgr = ccomp<MWindow>();
    auto& canvas = winMgr.getCanvas();

    const auto& self = _getCurrentState();

    _renderer->setColor(COLORS[self.owningPlayerIndex % NUM_COLORS]);
    _renderer->setPosition({self.x, self.y});
    _renderer->draw(canvas);

#if 0
    // Body bbox
    {
        hg::gr::RectangleShape rect{
            {(float)WIDTH, (float)HEIGHT}
        };
        rect.setOrigin({(float)WIDTH / 2.f, (float)HEIGHT / 2.f});
        rect.setFillColor(hg::gr::COLOR_TRANSPARENT);
        rect.setOutlineColor(hg::gr::COLOR_YELLOW);
        rect.setOutlineThickness(2.f);
        rect.setPosition(self.x, self.y);
        canvas.draw(rect);
    }

    // Hand bbox
    {
        hg::gr::RectangleShape rect{
            {(float)RAY_X_OFFSET * 2.f, 1.f}
        };
        rect.setFillColor(hg::gr::COLOR_TRANSPARENT);
        rect.setOutlineColor(hg::gr::COLOR_YELLOW);
        rect.setOutlineThickness(2.f);
        rect.setPosition(self.x - (float)RAY_X_OFFSET, self.y + (float)RAY_Y_OFFSET);
        canvas.draw(rect);
    }
#endif
}

void CharacterObject::_eventDraw2() {
    if (this->isDeactivated())
        return;

    const auto& self = _getCurrentState();

    auto& lobbyBackend = ccomp<MLobbyBackend>();
    if (self.owningPlayerIndex > 0) {
        const auto&  name = lobbyBackend.getLockedInPlayerInfo(self.owningPlayerIndex).name;
        hg::gr::Text text{hg::gr::BuiltInFonts::getFont(hg::gr::BuiltInFonts::TITILLIUM_REGULAR),
                          name,
                          30};
        text.setScale({2.f, 2.f});
        text.setFillColor(hg::gr::COLOR_WHITE);
        text.setOutlineColor(hg::gr::COLOR_BLACK);
        text.setOutlineThickness(4.f);
        const auto& localBounds = text.getLocalBounds();
        text.setOrigin({localBounds.w / 2.f, localBounds.h / 2.f});
        text.setPosition({self.x, self.y - 180.f});
        ccomp<MWindow>().getCanvas().draw(text);
    }
}

hg::alvin::CollisionDelegate CharacterObject::_initColDelegate() {
    auto builder = hg::alvin::CollisionDelegateBuilder{};
    builder.addInteraction<CharacterInterface>(
        hg::alvin::COLLISION_PRE_SOLVE,
        [this](CharacterInterface& aCharacter, const hg::alvin::CollisionData& aCollisionData) {
            if (aCharacter.getFling()) {
                grounded           = false;
                currentGroundTimer = fall_timer;
            }
            return hg::alvin::Decision::ACCEPT_COLLISION;
        });
    builder.addInteraction<TerrainInterface>(
        hg::alvin::COLLISION_CONTACT,
        [this](TerrainInterface& aTerrain, const hg::alvin::CollisionData& aCollisionData) {
            CP_ARBITER_GET_SHAPES(aCollisionData.arbiter, shape1, shape2);
            NeverNull<cpShape*> otherShape = shape1;
            if (otherShape == _unibody.shape) {
                otherShape = shape2;
            }
            const auto cellKind = aTerrain.getCellKindOfShape(otherShape);
            if (cellKind && *cellKind == CellKind::SCALE) {
                HG_LOG_INFO(LOG_ID, "Character reached the scales.");
                ccomp<MainGameplayManagerInterface>().characterReachedTheScales(*this);
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

void CharacterObject::_adjustView() {
    auto& self = _getCurrentState();
    auto& view = [this]() -> hg::gr::View& {
        auto gameOver = ccomp<MVarmap>().getInt64(VARMAP_ID_GAME_OVER);
        if (gameOver.has_value() && *gameOver == 1) {
            auto& winMgr = ccomp<MWindow>();
            winMgr.getView(0).setEnabled(false);
            return winMgr.getView(1);
        } else {
            auto& winMgr = ccomp<MWindow>();
            winMgr.getView(1).setEnabled(false);
            return winMgr.getView(0);
        }
    }();
    view.setEnabled(true);

    auto targetPos = sf::Vector2f{self.x, self.y};
    if (self.direction & DIRECTION_RIGHT) {
        targetPos.x += CAMERA_OFFSET;
    }
    if (self.direction & DIRECTION_LEFT) {
        targetPos.x -= CAMERA_OFFSET;
    }
    if (self.direction & DIRECTION_DOWN) {
        targetPos.y += CAMERA_OFFSET;
    }
    if (self.direction & DIRECTION_UP) {
        targetPos.y -= CAMERA_OFFSET;
    }

    {
        auto gameOver = ccomp<MVarmap>().getInt64(VARMAP_ID_GAME_OVER);
        if (gameOver.has_value() && *gameOver == 1) {
            const auto scalesPos = ccomp<MEnvironment>().getScalesGridPosition();
            targetPos            = {static_cast<float>((scalesPos.x + 2.f) * single_terrain_size),
                                    static_cast<float>(scalesPos.y * single_terrain_size)};
        }
    }

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

    // Round
    const auto center = view.getCenter();
    view.setCenter(std::roundf(center.x), std::roundf(center.y));
}

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(CharacterObject, (CREATE, UPDATE, DESTROY));

void CharacterObject::_syncCreateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(CharacterObject, aSyncCtrl);
}

void CharacterObject::_syncUpdateImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(CharacterObject, aSyncCtrl);
}

void CharacterObject::_syncDestroyImpl(spe::SyncControlDelegate& aSyncCtrl) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(CharacterObject, aSyncCtrl);
}
