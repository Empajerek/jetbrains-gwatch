#include <cstdint>
#include <sys/pidfd.h>

struct MySymbol {
    uint64_t address;
    uint8_t size;
    bool is_pie;
};

MySymbol get_symbol_from_elf(const char* file_name, const char* symbol_name);
uint64_t get_offset_from_maps(pid_t pid, const char *file_name);