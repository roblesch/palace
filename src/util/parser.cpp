#include "parser.hpp"

namespace pl {

Parser::Parser(int argc, const char* argv[])
{
    for (size_t i = 1; i < argc; i += 2) {
        args[argv[i]] = argv[i + 1];
    }
    return;
}

const char* Parser::arg(const char* flag)
{
    return args.find(flag)->second;
}

const char* Parser::obj_path()
{
    return args.find("-o")->second;
}

const char* Parser::gltf_path()
{
    return args.find("-g")->second;
}

const char* Parser::texture_path()
{
    return args.find("-t")->second;
}

}
