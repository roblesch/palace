#include "log.hpp"
#include <stdio.h>

namespace pl {

namespace {

void LOG(const char* severity, const char* message, const char* type)
{
    printf("(PL:%s:%s) %s\n", severity, type, message);
}

}

void LOG_INFO(const char* message, const char* type)
{
    LOG("INFO", message, type);
}

void LOG_WARN(const char* message, const char* type)
{
    LOG("WARN", message, type);
}

void LOG_ERROR(const char* message, const char* type)
{
    LOG("ERROR", message, type);
}

}
