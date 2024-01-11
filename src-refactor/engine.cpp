#include "engine.hpp"

Engine::Engine()
    : input_context_(new InputContext())
    , graphics_context_(new GraphicsContext())
    , ui_context_(new UiContext())
{
    Input::bind(input_context_);
    Graphics::bind(graphics_context_);
    Ui::bind(ui_context_);
}

Engine::~Engine()
{
    delete input_context_;
    Input::unbind();

    delete graphics_context_;
    Graphics::unbind();

    delete ui_context_;
    Ui::unbind();
}

void Engine::init()
{
    graphics_context_->create();
}

void Engine::cleanup()
{
    graphics_context_->destroy();
}

bool Engine::running()
{
    return Graphics::ready() && Input::ready() && Ui::ready() && !quit_;
}

void Engine::input()
{
    if (input_context_->processEvents() == InputEventResult::QUIT)
        quit_ = true;
}

void Engine::update()
{
    ui_context_->update();
}

void Engine::render()
{
    graphics_context_->draw();
}
