# Flight Booking System (v1)
# Build: GNU Make 3.81+, Apple clang 14 (or any clang++ supporting C++20)

SHELL := /bin/sh

CXX ?= clang++
AR  ?= ar

BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
LIB_DIR   := $(BUILD_DIR)/lib
BIN_DIR   := bin

CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -g -MMD -MP
CPPFLAGS += -Iinclude
LDFLAGS  += -pthread -lsqlite3

# -------------------------
# GoogleTest (fetched via git; built via CMake)
# -------------------------
GTEST_TAG := v1.14.0

GTEST_DIR := $(BUILD_DIR)/_deps/googletest
GTEST_SRC := $(GTEST_DIR)/src
GTEST_BLD := $(GTEST_DIR)/build

GTEST_LIB      := $(GTEST_BLD)/lib/libgtest.a
GTEST_MAIN_LIB := $(GTEST_BLD)/lib/libgtest_main.a
GTEST_INCLUDE  := $(GTEST_SRC)/googletest/include

# -------------------------
# Static libraries (ar rcs)
# -------------------------
DOMAIN_LIB      := $(LIB_DIR)/libflight_domain.a
APPLICATION_LIB := $(LIB_DIR)/libflight_application.a
INFRA_LIB       := $(LIB_DIR)/libflight_infrastructure.a
UTILS_LIB       := $(LIB_DIR)/libflight_utils.a

# -------------------------
# Sources (current project layout)
# -------------------------
DOMAIN_SOURCES := \
  src/domain/domain.cpp

INFRA_SOURCES := \
  src/infrastructure/in_memory_flight_repository.cpp \
  src/infrastructure/in_memory_reservation_repository.cpp \
  src/infrastructure/sqlite_flight_repository.cpp \
  src/infrastructure/flight_repository_factory.cpp

# If you don't have application/utils sources yet, leave them empty.
# Archives will still be created as valid empty .a files.
APPLICATION_SOURCES := src/application/application.cpp

# UTILS_SOURCES := src/util/strong_id.cpp
UTILS_SOURCES :=

CLI_SOURCES := \
  src/cli/main.cpp

TEST_SOURCES := \
  tests/booking_concurrency_test.cpp \
  tests/smoke_test.cpp \
  tests/sqlite_smoke_test.cpp \
  tests/sqlite_flight_repository_regression_test.cpp

# -------------------------
# Objects
# -------------------------
DOMAIN_OBJECTS      := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(DOMAIN_SOURCES))
INFRA_OBJECTS       := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(INFRA_SOURCES))
APPLICATION_OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(APPLICATION_SOURCES))
UTILS_OBJECTS       := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(UTILS_SOURCES))

CLI_OBJECTS  := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CLI_SOURCES))
TEST_OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TEST_SOURCES))

APP_BIN  := $(BIN_DIR)/flight_cli
TEST_BIN := $(BIN_DIR)/flight_tests

OPTIONAL_LIBS :=
OPTIONAL_LDLIBS :=

ifneq ($(strip $(APPLICATION_OBJECTS)),)
  OPTIONAL_LIBS += $(APPLICATION_LIB)
  OPTIONAL_LDLIBS += -lflight_application
endif

ifneq ($(strip $(UTILS_OBJECTS)),)
  OPTIONAL_LIBS += $(UTILS_LIB)
  OPTIONAL_LDLIBS += -lflight_utils
endif

# -------------------------
# Phony targets
# -------------------------
.PHONY: all app libs tests test run clean distclean

all: libs app tests

app: $(APP_BIN)

tests: $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

run: $(APP_BIN)
	./$(APP_BIN)

# -------------------------
# Directories
# -------------------------
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	@mkdir -p $(LIB_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# -------------------------
# Compile rule
# -------------------------
$(OBJ_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Only test objects need gtest headers. Also ensure gtest exists before compiling tests.
$(TEST_OBJECTS): CPPFLAGS += -I$(GTEST_INCLUDE)
$(TEST_OBJECTS): $(GTEST_LIB)

# -------------------------
# Build static libraries with ar rcs
# -------------------------
libs: $(DOMAIN_LIB) $(INFRA_LIB) $(OPTIONAL_LIBS)

# Domain library
$(DOMAIN_LIB): $(DOMAIN_OBJECTS) | $(LIB_DIR)
	$(AR) rcs $@ $^

# Infrastructure library
$(INFRA_LIB): $(INFRA_OBJECTS) | $(LIB_DIR)
	$(AR) rcs $@ $^

# Application library (may be empty initially)
$(APPLICATION_LIB): $(APPLICATION_OBJECTS) | $(LIB_DIR)
	$(AR) rcs $@ $^

# Utils library (may be empty initially)
$(UTILS_LIB): $(UTILS_OBJECTS) | $(LIB_DIR)
	$(AR) rcs $@ $^


# -------------------------
# Link CLI against libraries
# -------------------------
$(APP_BIN): $(CLI_OBJECTS) libs | $(BIN_DIR)
	$(CXX) $(CLI_OBJECTS) -L$(LIB_DIR) \
  	$(OPTIONAL_LDLIBS) -lflight_infrastructure -lflight_domain \
  	-o $@ $(LDFLAGS)

# -------------------------
# Link tests against libraries + GoogleTest
# -------------------------
$(TEST_BIN): $(TEST_OBJECTS) libs $(GTEST_MAIN_LIB) $(GTEST_LIB) | $(BIN_DIR)
	$(CXX) $(TEST_OBJECTS) -L$(LIB_DIR) \
  	$(OPTIONAL_LDLIBS) -lflight_infrastructure -lflight_domain \
  	$(GTEST_MAIN_LIB) $(GTEST_LIB) \
  	-o $@ $(LDFLAGS)

# -------------------------
# Fetch + build GoogleTest
# -------------------------
$(GTEST_SRC):
	@mkdir -p $(GTEST_DIR)
	@echo "Fetching GoogleTest into $(GTEST_SRC)..."
	@rm -rf "$(GTEST_SRC)"
	@git clone --depth 1 --branch $(GTEST_TAG) https://github.com/google/googletest.git "$(GTEST_SRC)"

$(GTEST_LIB): | $(GTEST_SRC)
	@mkdir -p $(GTEST_BLD)
	@echo "Configuring GoogleTest with CMake..."
	cmake -S "$(GTEST_SRC)" -B "$(GTEST_BLD)" -DCMAKE_BUILD_TYPE=Release -DBUILD_GMOCK=OFF
	@echo "Building GoogleTest..."
	cmake --build "$(GTEST_BLD)" --config Release -j

$(GTEST_MAIN_LIB): $(GTEST_LIB)
	@true

# -------------------------
# Cleaning
# -------------------------
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

distclean: clean
	rm -rf $(BUILD_DIR)/_deps

DEPS := $(APP_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d) \
        $(DOMAIN_OBJECTS:.o=.d) $(INFRA_OBJECTS:.o=.d) \
        $(APPLICATION_OBJECTS:.o=.d) $(UTILS_OBJECTS:.o=.d)
-include $(DEPS)
