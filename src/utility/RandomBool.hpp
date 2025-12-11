#include <random>

class RandomBool {
    std::bernoulli_distribution dist;
    std::mt19937 gen;
public:
    RandomBool()
        : dist(0.5f),
          gen(rand())
    {
    }
    bool random() { return dist(gen); }
};