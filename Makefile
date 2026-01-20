# Flight Booking System (v1)
# Build: GNU Make 3.81+, Apple clang 14 (or any clang++ supporting C++20)

SHELL := /bin/sh

CXX ?= clang++
AR  ?= ar

BUILD_DIR := build
BIN_DIR   := bin

CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -g
CPPFLAGS += -Iinclude
LDFLAGS  += -pthread

# ------- GoogleTest (fetched on demand; built with CMake) -------
GTEST_TAG := v1.14.0

GTEST_DIR := $(BUILD_DIR)/_deps/googletest
GTEST_SRC := $(GTEST_DIR)/src
GTEST_BLD := $(GTEST_DIR)/build

GTEST_LIB      := $(GTEST_BLD)/lib/libgtest.a
GTEST_MAIN_LIB := $(GTEST_BLD)/lib/libgtest_main.a
GTEST_INCLUDE  := $(GTEST_SRC)/googletest/include

# ------- Sources (matches flight_booking_v1_fixed.tar.gz) -------
APP_SOURCES := \
  src/cli/main.cpp \
  src/infrastructure/in_memory_flight_repository.cpp \
  src/infrastructure/in_memory_reservation_repository.cpp

TEST_SOURCES := \
  tests/booking_concurrency_test.cpp \
  src/infrastructure/in_memory_flight_repository.cpp \
  src/infrastructure/in_memory_reservation_repository.cpp \
  tests/smoke_test.cpp

APP_OBJECTS  := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(APP_SOURCES))
TEST_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_SOURCES))

APP_BIN  := $(BIN_DIR)/flight_cli
TEST_BIN := $(BIN_DIR)/flight_tests

.PHONY: all clean distclean test run
all: $(APP_BIN) $(TEST_BIN)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# -------- Compile rules --------
# Default compilation rule (NO gtest include here; app should not depend on gtest)
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Only test objects need gtest headers. Also ensure gtest is built before compiling tests.
$(TEST_OBJECTS): CPPFLAGS += -I$(GTEST_INCLUDE)
$(TEST_OBJECTS): $(GTEST_LIB)

# -------- App --------
$(APP_BIN): $(APP_OBJECTS) | $(BIN_DIR)
	$(CXX) $^ -o $@ $(LDFLAGS)

run: $(APP_BIN)
	./$(APP_BIN)

# -------- Tests --------
$(TEST_BIN): $(TEST_OBJECTS) $(GTEST_MAIN_LIB) $(GTEST_LIB) | $(BIN_DIR)
	$(CXX) $(TEST_OBJECTS) $(GTEST_MAIN_LIB) $(GTEST_LIB) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

# -------- Fetch + build GoogleTest --------
$(GTEST_SRC):
	@mkdir -p $(GTEST_DIR)
	@echo "Fetching GoogleTest into $(GTEST_SRC)..."
	@rm -rf "$(GTEST_SRC)"
	@git clone --depth 1 --branch $(GTEST_TAG) https://github.com/google/googletest.git "$(GTEST_SRC)"

$(GTEST_LIB): | $(GTEST_SRC)
	@mkdir -p $(GTEST_BLD)
	@echo "Configuring GoogleTest with CMake..."
	cmake -S "$(GTEST_SRC)" -B "$(GTEST_BLD)" -DCMAKE_BUILD_TYPE=Release -DBUILD_GMOCK=OFF -DBUILD_GTEST=ON
	@echo "Building GoogleTest..."
	cmake --build "$(GTEST_BLD)" --config Release -j

$(GTEST_MAIN_LIB): $(GTEST_LIB)
	@true

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# If you want to keep build/ but remove only dependencies:
distclean:
	rm -rf $(BUILD_DIR)/_deps
