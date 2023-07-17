add_library(util "log.hpp" "log.cpp" "parser.hpp" "parser.cpp")
add_library(pl::util ALIAS util)
