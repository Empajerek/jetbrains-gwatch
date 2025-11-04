#include "parser.hpp"

int wait_for_child(pid_t child);
pid_t create_child(const Arguments& args);
void ptrace_cont(int child);