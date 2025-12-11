#pragma once

#include <random>

class RandomIntInRange {
public:
    RandomIntInRange(int minimum, int maximum)
        : minVal(minimum), maxVal(maximum), gen(rand()),
        dist(minimum, maximum)
    {}
    int random() { return dist(gen); }
private:
    int minVal;
    int maxVal;
    // std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> dist;
};

class RandomFloatInRange {
public:
    RandomFloatInRange(float minimum, float maximum)
        : minVal(minimum), maxVal(maximum),
        // rd(), gen(rd()),
        gen(rand()),
        dist(minimum, maximum)
    {}
    float random() { return dist(gen); }
private:
    float minVal;
    float maxVal;
    // std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dist;
};
