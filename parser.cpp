#include <stdexcept>
#include "parser.hpp"

Arguments parse_arguments(int argc, char** argv) {
    if (argc < 5)
        throw std::invalid_argument(
            "Usage: gwatch --var <symbol> --exec <path> [-- arg1 ... argN]"
        );

    std::string var_name;
    std::string exec_name;
    char** exec_argv;

    int i = 1;
    for (; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--var") {
            if (++i >= argc)
                throw std::invalid_argument("Missing variable name after --var");
            var_name = argv[i];

        } else if (arg == "--exec") {
            if (++i >= argc)
                throw std::invalid_argument("Missing executable path after --exec");
            exec_name = argv[i];

        } else if (arg == "--") {
            break;
        }
    }

    if (var_name.empty() || exec_name.empty())
        throw std::invalid_argument("--var and --exec are required");

    exec_argv = &argv[i];
    argv[i] = exec_name.data();

    return Arguments{
        std::move(var_name),
        std::move(exec_name),
        exec_argv
    };
}