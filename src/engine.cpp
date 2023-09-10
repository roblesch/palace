#include "engine.hpp"

void Engine::init()
{
    gfx::init();
}

void Engine::cleanup()
{
    gfx::cleanup();
}

bool Engine::running()
{
    return _running;
}
