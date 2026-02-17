#include "PointPlayerEffect.h"

PointPlayerEffect::PointPlayerEffect(int id, const PointPlayerEffectConfig& config)
    : Effect(id), config_(config) {}

void PointPlayerEffect::initialize(Light* output, int numLEDs) {
    outputBuffer_ = output;
    numLEDs_ = numLEDs;

    const int rows = config_.rows;
    const int cols = config_.cols;

    // Fixed path: (1,1) -> (15,2) -> (7,5) -> (13,15)
    pathX_[0] = 0;   pathY_[0] = 31;
    pathX_[1] = 15;  pathY_[1] = 0;
    pathX_[2] = 31;   pathY_[2] = 31;
    pathX_[3] = 0;  pathY_[3] = 7;
    pathX_[4] = 31;   pathY_[4] = 7;

    Light colors[NUM_PLAYERS] = { config_.color1, config_.color2 };

    for (int n = 0; n < NUM_PLAYERS; ++n) {
        PointPlayer& pp = players_[n];
        pp.bindToGrid(outputBuffer_, rows, cols);
        pp.setup(pathX_[0], pathY_[0], NUM_POINTS, config_.speed, colors[n]);
        pp.fadeLength = config_.fadeLength;
        pp.Start();
    }

    isInitialized_ = true;
}

void PointPlayerEffect::update(float dt) {
    if (!isActive || !isInitialized_) return;
    for (auto& pp : players_) {
        pp.update(dt);
    }
}

void PointPlayerEffect::render(Light* output) {
    if (!isActive || !isInitialized_) return;

    for (auto& pp : players_) {
        pp.draw3();
    }
}
