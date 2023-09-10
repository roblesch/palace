#pragma once

#include "types.hpp"

namespace pl {

struct UiHandlerCreateInfo {

};

class UiHandler {

};

using UniqueUiHandler = std::uinique_ptr<UiHandler>;

UniqueUiHandler createUiHandlerUnique(const UiHandlerCreateInfo& createInfo);

}
