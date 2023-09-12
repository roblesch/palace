#pragma once

class Engine {
    bool quit = false;

public:
    void init();
    void cleanup();
    bool running();

    void input();
    void update();
    void render();
};
