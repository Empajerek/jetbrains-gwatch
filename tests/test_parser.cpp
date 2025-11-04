#include <gtest/gtest.h>
#include "../parser.hpp"
#include <stdexcept>

TEST(ParseArguments, ThrowsOnTooFewArgs) {
    char* argv[] = { (char*)"gwatch" };
    EXPECT_THROW(parse_arguments(1, argv), std::invalid_argument);
}

TEST(ParseArguments, ParsesCorrectly) {
    char a0[] = "gwatch";
    char a1[] = "--var";
    char a2[] = "MYVAR";
    char a3[] = "--exec";
    char a4[] = "/bin/ls";
    char a5[] = "--";
    char a6[] = "ls";
    char a7[] = "-l";
    char* argv[] = {a0,a1,a2,a3,a4,a5,a6,a7,nullptr};
    int argc = 8;

    Arguments args = parse_arguments(argc, argv);

    EXPECT_EQ(args.var_name, "MYVAR");
    EXPECT_EQ(args.exec_name, "/bin/ls");
    ASSERT_NE(args.exec_argv, nullptr);
    EXPECT_STREQ(args.exec_argv[0], args.exec_name.c_str());
}

TEST(ParseArguments, MissingVarValueThrows) {
    char a0[] = "gwatch";
    char a1[] = "--var";
    char* argv[] = {a0, a1, nullptr};
    EXPECT_THROW(parse_arguments(2, argv), std::invalid_argument);
}

TEST(ParseArguments, MissingExecValueThrows) {
    char a0[] = "gwatch";
    char a1[] = "--var";
    char a2[] = "MYVAR";
    char a3[] = "--exec";
    char* argv[] = {a0,a1,a2,a3,nullptr};
    EXPECT_THROW(parse_arguments(4, argv), std::invalid_argument);
}
