#pragma once
#include <string>

struct Arguments {
    std::string var_name;
    std::string exec_name;
    char** exec_argv;
};

Arguments parse_arguments(int argc, char** argv);