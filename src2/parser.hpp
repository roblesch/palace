#pragma once

#include <map>
#include <string>
#include <vector>

namespace pl {

class Parser {
public:
    Parser() = default;
    Parser(int argc, const char* argv[]);

    const char* arg(const char* flag);
    const char* gltf_path();

private:
    const std::vector<const char*> m_flags {
        "-g",
    };

    std::map<std::string, const char*> args;
};

}
