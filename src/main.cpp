#include "engine.hpp"

Engine* engine;

int main(int argc, char* argv[])
{
    engine = new Engine();
    engine->init();

    while (engine->running())
    {
    }

    engine->cleanup();
}
