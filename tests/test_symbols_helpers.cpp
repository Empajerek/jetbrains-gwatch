#include <gtest/gtest.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include "../symbols_helpers.hpp"

TEST(SymbolsHelpers, GetSymbolFromElfFindsGlobalPIE) {
    char exe_path[] = "build/tests/fixtures/sample_pie";
    MySymbol s = get_symbol_from_elf(exe_path, "global_variable");
    // hardcoded from readelf -a output
    // 32: 0000000000004024     4 OBJECT  GLOBAL DEFAULT   26 global_variable
    EXPECT_EQ(s.size, 4u) << "read global_variable size = " << s.size;
    EXPECT_EQ(s.is_pie, true) << "read global_variable PIE = " << s.is_pie;
    EXPECT_EQ(s.address, 0x4024ULL) << "read global_variable address = " << s.address;
}

TEST(SymbolsHelpers, GetSymbolFromElfFindsGlobalNonPIE) {
    char exe_path[] = "build/tests/fixtures/sample_nopie";
    MySymbol s = get_symbol_from_elf(exe_path, "global_variable");
    // hardcoded from readelf -a output
    // 32: 000000000040401c     4 OBJECT  GLOBAL DEFAULT   25 global_variable
    EXPECT_EQ(s.size, 4u) << "read global_variable size = " << unsigned(s.size);
    EXPECT_EQ(s.is_pie, false) << "read global_variable PIE = " << s.is_pie;
    EXPECT_EQ(s.address, 0x404024ULL) << "read global_variable address = " << std::hex << s.address;
}

TEST(SymbolsHelpers, RuntimePrintedAddressMatchesElfPlusBase) {
    const char exe_path[] = "build/tests/fixtures/sample_pie";

    int pfd[2];
    ASSERT_EQ(pipe(pfd), 0) << "pipe failed";

    pid_t child = fork();
    ASSERT_GE(child, 0) << "fork failed";

    if (child == 0) {
        // Child: redirect stdout to pipe and exec the fixture
        close(pfd[0]);
        if (dup2(pfd[1], STDOUT_FILENO) == -1) _exit(127);
        close(pfd[1]);
        execl(exe_path, exe_path, (char*)nullptr);
        _exit(127);
    }

    usleep(1e5);

    MySymbol sym = get_symbol_from_elf(exe_path, "global_variable");
    uint64_t base = get_offset_from_maps(child, exe_path);
    uint64_t expected = base + sym.address;

    close(pfd[1]);
    FILE* f = fdopen(pfd[0], "r");
    ASSERT_NE(f, nullptr) << "fdopen failed";

    char line[256];
    bool parsed = false;
    if (fgets(line, sizeof(line), f) != nullptr) {
        errno = 0;
        char* endptr = nullptr;
        uint64_t printed = strtoull(line, &endptr, 0);
        if (errno == 0) {
            EXPECT_EQ(printed, expected)
                << "printed=" << std::hex << printed << " expected=" << expected;
            parsed = true;
        }
    }

    fclose(f);

    kill(child, SIGTERM);
    int status;
    waitpid(child, &status, 0);

    EXPECT_TRUE(parsed) << "Failed to read/parse printed address from child";
}
