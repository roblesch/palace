#pragma once

#include "types.hpp"

namespace pl {

struct InputHandlerCreateInfo {

};

class InputHandler {

};

using UniqueInputHandler = std::unique_ptr<InputHandler>;

UniqueInputHandler createInputHandlerUnique(const InputHandlerCreateInfo& createInfo);

}
