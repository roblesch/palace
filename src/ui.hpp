#pragma once

#include "types.hpp"

class UiContext {
    static UiContext* singleton;
    bool initialized;

public:
    static void init();
    static void cleanup();
    static bool ready();
    static UiContext* get();

    UiContext();
    ~UiContext();

    void update();
};

namespace ui {

void init();
void cleanup();
bool ready();
UiContext* get();

void update();

}