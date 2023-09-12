#include "ui.hpp"

#include "graphics.hpp"

UiContext* UiContext::singleton = nullptr;

void UiContext::init()
{
    singleton = new UiContext();
    singleton->initialized = true;
}

void UiContext::cleanup()
{
    delete singleton;
}

bool UiContext::ready()
{
    return singleton->initialized;
}

UiContext* UiContext::get()
{
    return singleton;
}

UiContext::UiContext()
{
}

UiContext::~UiContext()
{
}

/*
*/

void UiContext::update()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(gfx::get()->sdlWindow());
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();
}

/*
*/

void ui::init()
{
    UiContext::init();
}

void ui::cleanup()
{
    UiContext::cleanup();
}

bool ui::ready()
{
    return UiContext::ready();
}

UiContext* ui::get()
{
    return UiContext::get();
}
