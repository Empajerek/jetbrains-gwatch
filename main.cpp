#include <cstring>
#include <exception>
#include <iostream>

#include "dbgreg_helpers.hpp"
#include "symbols_helpers.hpp"
#include "parser.hpp"
#include "syscalls_helpers.hpp"


int main(int argc, char **argv) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    try {
        Arguments args = parse_arguments(argc, argv);
        pid_t child = create_child(args);
        int status = wait_for_child(child);

        if (!WIFSTOPPED(status))
            throw std::runtime_error("child didn't stop as expected");

        // find symbol and setup debug register
        auto symbol = get_symbol_from_elf(args.exec_name.data(), args.var_name.data());
        if (symbol.is_pie)
            symbol.address += get_offset_from_maps(child, args.exec_name.c_str());
        set_hardware_breakpoint(child, reinterpret_cast<void*>(symbol.address), symbol.size);

        // find starting value of variable
        uint64_t old_value = read_variable(child, reinterpret_cast<void*>(symbol.address), symbol.size);
        ptrace_cont(child);

        while (true) {
            int status = wait_for_child(child);

            if (WIFEXITED(status))
                return 0;
            if (WIFSIGNALED(status)) {
                std::cerr << "child signaled: " << strsignal(WTERMSIG(status)) << "\n";
                return 0;
            }

            if (WIFSTOPPED(status)) {
                int sig = WSTOPSIG(status);
                if (sig == SIGTRAP) {
                    uint64_t new_value = read_variable(child, reinterpret_cast<void*>(symbol.address), symbol.size);
                    if (new_value != old_value) {
                        std::cout << "<" << args.var_name << ">    write    " << old_value << " -> " << new_value << "\n";
                        old_value = new_value;
                    } else {
                        std::cout << "<" << args.var_name << ">    read    " << old_value << "\n";
                    }
                }
                ptrace_cont(child);
            } else {
                // unexpected wait
                throw std::runtime_error("unexpected wait status");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        return 1;
    }
}