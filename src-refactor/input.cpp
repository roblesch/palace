#include "input.hpp"

#include "ui.hpp"

/*
*/

InputContext::InputContext()
{
}

InputContext::~InputContext()
{
}

/*
*/

void InputContext::init()
{
}

/*
 */

std::vector<const char*> InputContext::instanceExtensions()
{
    uint32_t extensionCount;
    SDL_Vulkan_GetInstanceExtensions(sdl_window_, &extensionCount, nullptr);
    std::vector<const char*> instanceExtensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(sdl_window_, &extensionCount, instanceExtensions.data());
    return instanceExtensions;
}

/*
 *
 */

InputEventResult InputContext::processEvents()
{
    InputEventResult result = CONTINUE;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
//        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
        case SDL_QUIT:
            result = QUIT;
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
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

InputContext* Input::input_context_ = nullptr;

void Input::bind(InputContext* input_context)
{
    input_context_ = input_context;
}

void Input::unbind()
{
    input_context_ = nullptr;
}

bool Input::ready()
{
    return input_context_ != nullptr;
}
