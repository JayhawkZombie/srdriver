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
 * Two zoomies sharing one path: (1,1) -> (15,2) -> (7,5) -> (13,15). PointPlayer
 * round-robins through the path; we just assign pathX/pathY and pass them in.
 */
class PointPlayerEffect : public Effect {
public:
    static constexpr int NUM_POINTS = 4;
    static constexpr int NUM_PLAYERS = 2;

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

    uint8_t pathX_[NUM_POINTS];
    uint8_t pathY_[NUM_POINTS];
    std::array<PointPlayer, NUM_PLAYERS> players_;
};
