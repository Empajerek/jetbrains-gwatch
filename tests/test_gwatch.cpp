#include <gtest/gtest.h>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <limits.h>
#include <chrono>

std::string exec(const char* cmd) {
    std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    std::stringstream ss;
    std::array<char, 128> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        ss << buffer.data();
    }
    return ss.str();
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void RunGwatchTest(const std::string& input_file, const std::string& output_file) {
    std::stringstream cmd_builder;
    cmd_builder << "./gwatch --var z --exec build/tests/fixtures/test_pie < " << input_file;
    std::string cmd = cmd_builder.str();

    std::string expected_output = readFile(output_file);
    std::string actual_output = exec(cmd.c_str());

    ASSERT_EQ(actual_output, expected_output);
}

TEST(GwatchIntegrationTest, TestRW) {
    RunGwatchTest("tests/fixtures/testRW.in", "tests/fixtures/testRW.out");
}

TEST(GwatchIntegrationTest, TestRWA) {
    RunGwatchTest("tests/fixtures/testRWA.in", "tests/fixtures/testRWA.out");
}

TEST(GwatchOverheadTest, CompareExecutionTime) {
    const char* fixture_path = "build/tests/fixtures/speed_pie";

    auto start_direct = std::chrono::high_resolution_clock::now();
    exec(fixture_path);
    auto end_direct = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> direct_duration = end_direct - start_direct;

    std::stringstream gwatch_cmd_builder;
    gwatch_cmd_builder << "./gwatch --var z --exec " << fixture_path << " -- > /dev/null";
    std::string gwatch_cmd = gwatch_cmd_builder.str();

    auto start_gwatch = std::chrono::high_resolution_clock::now();
    exec(gwatch_cmd.c_str());
    auto end_gwatch = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> gwatch_duration = end_gwatch - start_gwatch;

    std::cout << std::endl
            << "[   INFO   ] " << "Execution time without gwatch: " << direct_duration.count() << " ms" << std::endl
            << "[   INFO   ] " << "Execution time with gwatch: " << gwatch_duration.count() << " ms" << std::endl
            << "[   INFO   ] " << "Overhead: " << gwatch_duration.count() - direct_duration.count() << " ms" << std::endl;

    SUCCEED() << "Overhead test completed. See console output for timings.";
}
