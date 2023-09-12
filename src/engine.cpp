#include "engine.hpp"

#include "graphics.hpp"
#include "input.hpp"
#include "ui.hpp"

void Engine::init()
{
    gfx::init();
    io::init();
}

void Engine::cleanup()
{
    gfx::cleanup();
    io::cleanup();
}

bool Engine::running()
{
    return gfx::ready() && io::ready() && !quit;
}

void Engine::input()
{
    if (io::get()->handleSdlEvents() == InputContext::QUIT)
        quit = true;
}

void Engine::update()
{
    ui::get()->update();
}

void Engine::render()
{
    gfx::render();
}
