#include <cstddef>
#include <cstdint>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>

void set_hardware_breakpoint(pid_t pid, void* address, size_t size);
void clear_dr6(pid_t pid);
uint64_t read_variable(pid_t pid, void* address, uint8_t size);