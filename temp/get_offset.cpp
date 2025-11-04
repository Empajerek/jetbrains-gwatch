#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

static short global_variable = 5;

int main() {
    // memory mappings file for the current process
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) {
        std::cerr << "Failed to open /proc/self/maps" << std::endl;
        return 1;
    }

    std::string line;
    std::string exe_path;
    char exe_buf[4096];

    // get my path
    ssize_t len = readlink("/proc/self/exe", exe_buf, sizeof(exe_buf) - 1);
    if (len != -1) {
        exe_buf[len] = '\0';
        exe_path = exe_buf;
    } else {
        std::cerr << "Failed to read /proc/self/exe" << std::endl;
        return 1;
    }

    bool start_found = 0;

    while (std::getline(maps, line)) {
        if (line.find(exe_path) != std::string::npos && line.find("rw-p") != std::string::npos) {
            std::string base_addr = line.substr(0, line.find('-'));
            std::cout << "Runtime .data base: 0x" << base_addr << std::endl;
            std::cout << "Address of example global variable: 0x" << std::hex << &global_variable << std::dec << std::endl;
        }

        if (!start_found && line.find(exe_path) != std::string::npos && line.find("r--p") != std::string::npos) {
            std::string base_addr = line.substr(0, line.find('-'));
            std::cout << "Runtime start base: 0x" << base_addr << std::endl;
            std::cout << "So virtual address of variable is 0x" << std::hex << (unsigned long long) &global_variable - std::stoull(base_addr, nullptr, 16) << std::endl;
            start_found = true;
        }
    }

    maps.close();
    return 0;
}
