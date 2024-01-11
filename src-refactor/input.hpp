#pragma once

#include "types.hpp"
#include <vector>

enum InputEventResult {
    CONTINUE,
    QUIT
};

class InputContext {
    SDL_Window* sdl_window_;

public:
    InputContext();
    ~InputContext();

    void init();

    std::vector<const char*> instanceExtensions();

    InputEventResult processEvents();
};

class Input {
    static InputContext* input_context_;

public:
    static void bind(InputContext* context);
    static void unbind();
    static bool ready();
};