#include "log.hpp"
#include <stdio.h>

void LOG(const char* severity, const char* message, const char* type)
{
    printf("(VK_:%s:%s) %s\n", severity, type, message);
}

namespace vk_ {

void LOG_INFO(const char* message, const char* type)
{
    LOG("INFO", message, type);
}

void LOG_ERROR(const char* message, const char* type)
{
    LOG("ERROR", message, type);
}

}
