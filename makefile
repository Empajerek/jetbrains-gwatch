CXX := g++
CXXFLAGS := -std=c++23 -O3 -Wall -Wextra -Wpedantic

TARGET := gwatch

TESTDIR := tests
BUILDDIR := build
GTEST_DIR := third_party/googletest

SRCS := main.cpp \
        parser.cpp \
        dbgreg_helpers.cpp \
        symbols_helpers.cpp \
        syscalls_helpers.cpp

TEST_SRCS := $(wildcard $(TESTDIR)/*.cpp)

FIXTURE_SRCS := tests/fixtures/sample.c tests/fixtures/test.c tests/fixtures/speed.c
FIXTURE_BINDIR := $(BUILDDIR)/tests/fixtures
FIXTURE_PIE_BINS := $(patsubst tests/fixtures/%.c,$(FIXTURE_BINDIR)/%_pie,$(FIXTURE_SRCS))
FIXTURE_NOPIE_BINS := $(patsubst tests/fixtures/%.c,$(FIXTURE_BINDIR)/%_nopie,$(FIXTURE_SRCS))
FIXTURE_BINS := $(FIXTURE_PIE_BINS) $(FIXTURE_NOPIE_BINS)

GTEST_SRCS := $(GTEST_DIR)/googletest/src/gtest-all.cc
GTEST_OBJS := $(patsubst $(GTEST_DIR)/%.cc,$(BUILDDIR)/%.o,$(GTEST_SRCS))

OBJS := $(patsubst %.cpp,$(BUILDDIR)/%.o,$(SRCS))
TEST_OBJS := $(patsubst $(TESTDIR)/%.cpp,$(BUILDDIR)/tests/%.o,$(TEST_SRCS))
DEPS := $(OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(GTEST_OBJS:.o=.d)

TARGET_BIN := $(TARGET)
TEST_BIN := $(BUILDDIR)/tests/runTests

.PHONY: all

all: $(TARGET_BIN)

$(TARGET_BIN): $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TEST_BIN) $(FIXTURE_BINS) $(TARGET)
	@echo "Running unit tests..."
	./$(TEST_BIN)

$(TEST_BIN): $(OBJS) $(TEST_OBJS) $(GTEST_OBJS) 
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(GTEST_DIR)/googletest/include -pthread -o $@ $(filter-out $(BUILDDIR)/main.o,$^)

$(BUILDDIR)/%.o: $(GTEST_DIR)/%.cc
	@mkdir -p $(dir $@)
	# Add $(GTEST_DIR)/googletest to include path so gtest-all's internal
	# includes like "src/gtest-assertion-result.cc" resolve correctly.
	$(CXX) $(CXXFLAGS) -I$(GTEST_DIR)/googletest/include -I$(GTEST_DIR) -I$(GTEST_DIR)/googletest -MMD -MP -c $< -o $@

$(BUILDDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/tests/%.o: $(TESTDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(GTEST_DIR)/googletest/include -MMD -MP -c $< -o $@

$(FIXTURE_BINDIR)/%_pie: tests/fixtures/%.c
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -fPIE -pie -o $@ $<

$(FIXTURE_BINDIR)/%_nopie: tests/fixtures/%.c
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -fno-pie -no-pie -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(BUILDDIR)
