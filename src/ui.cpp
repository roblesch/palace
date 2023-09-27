#include "ui.hpp"

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
//    ImGui_ImplVulkan_NewFrame();
//    ImGui_ImplSDL2_NewFrame(nullptr);
//    ImGui::NewFrame();
//    ImGui::ShowDemoWindow();
//    ImGui::Render();
}

/*
*/

UiContext* Ui::ui_context_ = nullptr;

void Ui::bind(UiContext* ui_context)
{
    ui_context_ = ui_context;
}

void Ui::unbind()
{
    ui_context_ = nullptr;
}

bool Ui::ready()
{
    return ui_context_ != nullptr;
}
