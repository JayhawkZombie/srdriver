#include "PointPlayerEffect.h"

PointPlayerEffect::PointPlayerEffect(int id, const PointPlayerEffectConfig& config)
    : Effect(id), config_(config) {
    for (int n = 0; n < NUM_PLAYERS; ++n)
        nextWaypointIndex_[n] = 0;
}

void PointPlayerEffect::initialize(Light* output, int numLEDs) {
    outputBuffer_ = output;
    numLEDs_ = numLEDs;

    const int rows = config_.rows;
    const int cols = config_.cols;

    // 3 fixed waypoints (triangle)
    waypoints_[0] = GridPt(2, 2);
    waypoints_[1] = GridPt(static_cast<uint8_t>(cols / 2), 2);
    waypoints_[2] = GridPt(static_cast<uint8_t>(cols / 2), static_cast<uint8_t>(rows / 2));

    Light colors[NUM_PLAYERS] = { config_.color1, config_.color2 };

    for (int n = 0; n < NUM_PLAYERS; ++n) {
        // This player's path segment
        uint8_t* pX = &pathX_[n * N_PT_EACH];
        uint8_t* pY = &pathY_[n * N_PT_EACH];
        // Fill ring with waypoints round-robin so path is valid from the start
        for (int i = 0; i < N_PT_EACH; ++i) {
            const GridPt& w = waypoints_[i % NUM_WAYPOINTS];
            pX[i] = w.x;
            pY[i] = w.y;
        }

        PointPlayer& pp = players_[n];
        pp.bindToGrid(outputBuffer_, rows, cols);
        pp.setup(pX[0], pY[0], N_PT_EACH, config_.speed, colors[n]);
        pp.fadeLength = config_.fadeLength;
        pp.Start();

        nextWaypointIndex_[n] = 0;
    }

    isInitialized_ = true;
}

void PointPlayerEffect::update(float dt) {
    if (!isActive || !isInitialized_) return;

    for (int n = 0; n < NUM_PLAYERS; ++n) {
        PointPlayer& pp = players_[n];
        uint8_t cp = pp.currPoint;
        pp.update(dt);

        // Assign 2 ahead when currPoint changes (dad's pattern: nextPoint is in use, so assign cp2)
        if (pp.currPoint != cp) {
            uint8_t cp1 = (pp.currPoint + 1) % N_PT_EACH;
            uint8_t cp2 = (cp1 + 1) % N_PT_EACH;
            int idx = n * N_PT_EACH;
            const GridPt& w = waypoints_[nextWaypointIndex_[n] % NUM_WAYPOINTS];
            // Avoid zero-length segment: if next slot already equals this waypoint, use the next waypoint
            uint8_t nextSlot = (cp2 + 1) % N_PT_EACH;
            if (pathX_[idx + nextSlot] == w.x && pathY_[idx + nextSlot] == w.y) {
                const GridPt& w2 = waypoints_[(nextWaypointIndex_[n] + 1) % NUM_WAYPOINTS];
                pathX_[idx + cp2] = w2.x;
                pathY_[idx + cp2] = w2.y;
                nextWaypointIndex_[n] += 2;  // consumed two waypoints
            } else {
                pathX_[idx + cp2] = w.x;
                pathY_[idx + cp2] = w.y;
                ++nextWaypointIndex_[n];
            }
        }
    }
}

void PointPlayerEffect::render(Light* output) {
    if (!isActive || !isInitialized_) return;

    for (int i = 0; i < numLEDs_; ++i) {
        output[i] = Light(0, 0, 0);
    }

    for (auto& pp : players_) {
        pp.draw2();
    }
}
