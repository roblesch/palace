#pragma once

#include "graphics.hpp"
#include "types.hpp"

class Engine {
    bool _running { false };

public:
    void init();
    void cleanup();

    bool running();
};
