#pragma once

#include "input.hpp"
#include "graphics.hpp"
#include "ui.hpp"

class Engine {
    bool quit_ = false;

    InputContext* input_context_;
    GraphicsContext* graphics_context_;
    UiContext* ui_context_;

public:
    Engine();
    ~Engine();

    void init();
    void cleanup();
    bool running();

    void input();
    void update();
    void render();
};
