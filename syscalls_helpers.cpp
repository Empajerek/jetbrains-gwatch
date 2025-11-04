#include "syscalls_helpers.hpp"
#include <cstring>
#include <iostream>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <system_error>

int wait_for_child(pid_t child) {
    int status;
    if (waitpid(child, &status, 0) < 0)
        throw std::system_error(errno, std::generic_category(), "waitpid failed");
    return status;
}

pid_t create_child(const Arguments& args) {
    pid_t child = fork();
    if (child < 0)
        throw std::system_error(errno, std::generic_category(), "fork failed");

    if (child == 0) {
        // child
        if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
            std::cerr << "PTRACE_TRACEME failed: " << std::strerror(errno) << "\n";
            _exit(1);
        }

        execv(args.exec_name.c_str(), args.exec_argv);
        std::cerr << "execv failed: " << std::strerror(errno) << "\n";
        _exit(127);
    }

    return child;
};

void ptrace_cont(int child) {
    if (ptrace(PTRACE_CONT, child, 0, 0) < 0)
        throw std::system_error(errno, std::generic_category(), "PTRACE_CONT failed");
}