#include "input.hpp"

#include "graphics.hpp"

/*
*/

InputContext* InputContext::singleton = nullptr;

void InputContext::init()
{
    singleton = new InputContext();
    singleton->initialized = true;
}

void InputContext::cleanup()
{
    delete singleton;
}

bool InputContext::ready()
{
    return singleton->initialized;
}

InputContext* InputContext::get()
{
    return singleton;
}

InputContext::InputContext()
{
}

InputContext::~InputContext()
{
}

/*
*/

InputContext::Result InputContext::handleSdlEvents()
{
    Result result = CONTINUE;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
        case SDL_QUIT:
            result = QUIT;
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
                gfx::get()->resize();
                break;
            }
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                result = QUIT;
                break;
            }
        }
    }

    return result;
}

/*
*/

void io::init()
{
    InputContext::init();
}

void io::cleanup()
{
    InputContext::cleanup();
}

bool io::ready()
{
    return InputContext::ready();
}

InputContext* io::get()
{
    return InputContext::get();
}
