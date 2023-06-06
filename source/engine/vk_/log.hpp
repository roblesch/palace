#ifndef PALACE_ENGINE_VK_LOG_HPP
#define PALACE_ENGINE_VK_LOG_HPP

namespace vk_ {

namespace {

void LOG(const char* severity, const char* message, const char* type)
{
    printf("(VK_:%s:%s) %s\n", severity, type, message);
}

}

void LOG_INFO(const char* message, const char* type = "GENERAL")
{
    LOG("INFO", message, type);
}

void LOG_ERROR(const char* message, const char* type = "GENERAL")
{
    LOG("ERROR", message, type);
}

}

#endif
