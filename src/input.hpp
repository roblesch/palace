#pragma once

#include "types.hpp"

class InputContext {
    static InputContext* singleton;
    bool initialized = false;

public:
    enum Result { CONTINUE, QUIT };

    static void init();
    static void cleanup();
    static bool ready();
    static InputContext* get();

    InputContext();
    ~InputContext();

    InputContext::Result handleSdlEvents();

};

namespace io {

void init();
void cleanup();
bool ready();
InputContext* get();

};