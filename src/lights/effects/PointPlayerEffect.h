#pragma once

#include "Effect.h"
#include "../PointPlayer.h"
#include "../Light.h"
#include <array>

/** Integer grid coordinate for path waypoints (effect layer only; PointPlayer still uses pathX/pathY). */
struct GridPt {
    uint8_t x = 0;
    uint8_t y = 0;
    GridPt() = default;
    GridPt(uint8_t x_, uint8_t y_) : x(x_), y(y_) {}
};

/** Config for the point-player effect (grid size and optional speed/fade/colors). */
struct PointPlayerEffectConfig {
    int rows = 32;
    int cols = 32;
    float speed = 40.0f;
    float fadeLength = 8.0f;
    Light color1 = Light(200, 0, 100);
    Light color2 = Light(0, 200, 40);
};

/**
 * Point Player Effect
 *
 * Uses PointPlayer(s) to draw zoomies that jump between fixed waypoints with a tail.
 * Follows dad's pattern: each player has a ring of N_PT_EACH path points; we "assign
 * 2 ahead" in update() so the next destination is always set before the player gets there.
 * For 3 fixed waypoints we assign round-robin into that slot.
 */
class PointPlayerEffect : public Effect {
public:
    static constexpr int NUM_WAYPOINTS = 3;
    static constexpr int NUM_PLAYERS = 2;
    /** Path points per player (ring buffer; we assign the slot 2 ahead when currPoint changes). */
    static constexpr int N_PT_EACH = 16;

    explicit PointPlayerEffect(int id, const PointPlayerEffectConfig& config);
    virtual ~PointPlayerEffect() = default;

    void update(float dt) override;
    void initialize(Light* output, int numLEDs) override;
    void render(Light* output) override;
    bool isFinished() const override { return false; }

private:
    PointPlayerEffectConfig config_;
    Light* outputBuffer_ = nullptr;
    int numLEDs_ = 0;
    bool isInitialized_ = false;

    /** Waypoints in our struct form (clarity). */
    std::array<GridPt, NUM_WAYPOINTS> waypoints_;
    /** Path buffers: N_PT_EACH per player. Player n uses &pathX_[n*N_PT_EACH]. */
    uint8_t pathX_[N_PT_EACH * NUM_PLAYERS];
    uint8_t pathY_[N_PT_EACH * NUM_PLAYERS];
    /** Round-robin index for "assign 2 ahead" (which of the 3 waypoints to assign next, per player). */
    int nextWaypointIndex_[NUM_PLAYERS];

    std::array<PointPlayer, NUM_PLAYERS> players_;
};
