#include "dbgreg_helpers.hpp"
#include <cstddef>
#include <system_error>
#include <stdexcept>

static long size_to_breakpoint_length(size_t size) {
    switch (size) {
        case 1: return 0b00;
        case 2: return 0b01;
        case 4: return 0b11;
        case 8: return 0b10;
        default: throw std::invalid_argument("Unsupported variable size for hardware breakpoint. Must be 1, 2, 4, or 8.");
    }
}

void set_hardware_breakpoint(pid_t pid, void* address, size_t size) {
    // Set variable address in DR0
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(struct user, u_debugreg[0]), address) == -1)
        throw std::system_error(errno, std::generic_category(), "PTRACE_POKEUSER to set DR0");

    // Read the current DR7 value
    long dr7_value = ptrace(PTRACE_PEEKUSER, pid, offsetof(struct user, u_debugreg[7]), nullptr);
    if (dr7_value == -1 && errno != 0)
        throw std::system_error(errno, std::generic_category(), "PTRACE_PEEKUSER to read DR7");

    // Clear the bits for DR0
    dr7_value &= ~((0b11 << 16) | (0b11 << 18) | 1);

    // Set the new configuration
    dr7_value |= 0b11L << 16; // Set R/W bits
    dr7_value |= size_to_breakpoint_length(size) << 18;      // Set LEN bits
    dr7_value |= 1;

    // Write the modified DR7 value back
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(struct user, u_debugreg[7]), dr7_value) == -1) {
        throw std::system_error(errno, std::generic_category(), "PTRACE_POKEUSER to write DR7");
    }
}

void clear_dr6(pid_t pid) {
    if (ptrace(PTRACE_POKEUSER, pid, offsetof(struct user, u_debugreg[6]), 0) == -1)
        throw std::system_error(errno, std::generic_category(), "PTRACE_POKEUSER to clear DR6");
}

uint64_t read_variable(pid_t pid, void* address, uint8_t size) {
    errno = 0;
    long word = ptrace(PTRACE_PEEKDATA, pid, address, nullptr);
    if (word == -1 && errno != 0)
        throw std::system_error(errno, std::generic_category(), "PTRACE_PEEKDATA failed");

    if (size <= 4) {
        uint64_t mask = (1ULL << (8 * size)) - 1ULL;
        return static_cast<uint64_t>(word) & mask;
    }

    if constexpr (sizeof(long) == 4) {
        long upperword = ptrace(PTRACE_PEEKDATA, pid, (std::byte*) address + 4, nullptr);
        if (word == -1 && errno != 0)
            throw std::system_error(errno, std::generic_category(), "PTRACE_PEEKDATA failed");
        return (upperword << 32) + word;
    }

    return word;
}