# Host side build: this device's pure logic and its tests, with no pico-sdk
# present. The firmware itself is built by cmake under firmware/ (see
# README.md); this file never touches it. The shape follows the Flipper's
# Makefile so a contributor moving between the two directories meets one set
# of targets. The protocol library, the fuzz harness, the test harness and
# the QR encoder live under ../shared/ and are compiled from there; their own
# suites and the fuzz run come from ../shared/Makefile, and test-shared runs
# the shared suites under this device's host flag set as well (peripheral
# spec Part 6).

# make predefines CC as "cc", which the MinGW distribution on the Windows host
# does not provide, so only a caller supplied CC survives; the default is gcc.
ifeq ($(origin CC),default)
CC := gcc
endif
PYTHON ?= python3

# The Flipper's warning set, kept identical here so the pure logic modules
# read and compile the same in both. The firmware build applies the same set
# to this project's own sources (firmware/CMakeLists.txt); vendored code is
# held to the base set without the two extras, exactly as there.
BASE_WARNINGS := -std=gnu2x -Wall -Wextra -Werror -Wstrict-prototypes -Wredundant-decls \
                 -Wdouble-promotion -Wundef
HOST_WARNINGS := $(BASE_WARNINGS) -Wshadow -Wconversion
# The shared code, reached by relative path and by no other means
# (peripheral spec 2.4).
SHARED_DIR := ../shared
HOST_CFLAGS := $(HOST_WARNINGS) -O1 -g -I$(SHARED_DIR)/lib/qrcodegen
VENDORED_CFLAGS := $(BASE_WARNINGS) -O1 -g
SANITISER_CFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
# Heap functions are wrapped in every test binary so a test can prove that
# the code under it did not allocate (Flipper 0.10). The wrappers live in
# ../shared/tests/test_support.h.
ALLOCATION_WRAP_LDFLAGS := -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free

BUILD_DIR := build/host

PROTOCOL_SOURCES := $(SHARED_DIR)/protocol/remote_protocol_tables.c $(SHARED_DIR)/protocol/remote_protocol.c

# Every pure logic module, compiled into every suite. Adding a module here is
# the only change needed for the tests to see it.
PURE_LOGIC_SOURCES := remote_input/remote_input_model.c \
                      remote_display/remote_bitmap.c \
                      remote_display/remote_font.c \
                      remote_display/remote_display_layout.c \
                      remote_display/remote_display_fixtures.c \
                      remote_display/remote_refresh_policy.c \
                      $(PROTOCOL_SOURCES) \
                      peer/development_peer_core.c \
                      session/remote_session.c \
                      transport/remote_link_edge.c \
                      remote_display/remote_qr.c
PURE_LOGIC_HEADERS := $(wildcard remote_input/*.h remote_display/*.h $(SHARED_DIR)/protocol/*.h peer/*.h session/*.h transport/*.h $(SHARED_DIR)/lib/qrcodegen/*.h)

# The vendored encoder is compiled once per warning set, held to the base
# set as on the device (PROVENANCE.md at the root).
VENDORED_SOURCES := $(SHARED_DIR)/lib/qrcodegen/qrcodegen.c
VENDORED_OBJECT := $(BUILD_DIR)/vendored_qrcodegen.o
VENDORED_SANITISED_OBJECT := $(BUILD_DIR)/vendored_qrcodegen_sanitised.o

# The development peer shell is POSIX (termios), so it builds on Linux and
# WSL, not under MinGW. It is a host tool, never part of the firmware.
PEER_SOURCES := peer/development_peer_shell.c peer/development_peer_core.c $(PROTOCOL_SOURCES)

# One binary per suite, named after its source.
TEST_SOURCES := $(wildcard tests/test_*.c)
TEST_HEADERS := $(SHARED_DIR)/tests/test_support.h
TEST_BINARIES := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SOURCES))
SANITISED_TEST_BINARIES := $(patsubst tests/%.c,$(BUILD_DIR)/%_sanitised,$(TEST_SOURCES))

.PHONY: test test-shared test-sanitise peer check-typography check-protocol-tables check-protocol-definition check clean

test: $(TEST_BINARIES)
	@for suite in $(TEST_BINARIES); do echo "== $$suite"; $$suite || exit 1; done

# The shared suites under this device's host flag set, built into this
# device's build directory so the two trees never share an object.
test-shared:
	$(MAKE) -C $(SHARED_DIR) test CC="$(CC)" HOST_CFLAGS="$(HOST_WARNINGS) -O1 -g -Ilib/qrcodegen" BUILD_DIR="$(CURDIR)/$(BUILD_DIR)/shared"

test-sanitise: $(SANITISED_TEST_BINARIES)
	@for suite in $(SANITISED_TEST_BINARIES); do echo "== $$suite"; $$suite || exit 1; done

$(VENDORED_OBJECT): $(VENDORED_SOURCES) $(PURE_LOGIC_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(VENDORED_CFLAGS) -c -o $@ $(VENDORED_SOURCES)

$(VENDORED_SANITISED_OBJECT): $(VENDORED_SOURCES) $(PURE_LOGIC_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(VENDORED_CFLAGS) $(SANITISER_CFLAGS) -c -o $@ $(VENDORED_SOURCES)

$(BUILD_DIR)/%: tests/%.c $(PURE_LOGIC_SOURCES) $(PURE_LOGIC_HEADERS) $(TEST_HEADERS) $(VENDORED_OBJECT)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) -o $@ $< $(PURE_LOGIC_SOURCES) $(VENDORED_OBJECT) $(ALLOCATION_WRAP_LDFLAGS)

$(BUILD_DIR)/%_sanitised: tests/%.c $(PURE_LOGIC_SOURCES) $(PURE_LOGIC_HEADERS) $(TEST_HEADERS) $(VENDORED_SANITISED_OBJECT)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) $(SANITISER_CFLAGS) -o $@ $< $(PURE_LOGIC_SOURCES) $(VENDORED_SANITISED_OBJECT) $(ALLOCATION_WRAP_LDFLAGS)

peer: $(BUILD_DIR)/development_peer

$(BUILD_DIR)/development_peer: $(PEER_SOURCES) $(PURE_LOGIC_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) -o $@ $(PEER_SOURCES)

# The tree wide scan and the protocol checks live under ../shared/ and run
# over the whole tree; these targets exist so the commands this device's
# documents name still work from this directory.
check-typography:
	$(MAKE) -C $(SHARED_DIR) check-typography PYTHON="$(PYTHON)"

check-protocol-tables:
	$(MAKE) -C $(SHARED_DIR) check-protocol-tables PYTHON="$(PYTHON)"

check-protocol-definition:
	$(MAKE) -C $(SHARED_DIR) check-protocol-definition PYTHON="$(PYTHON)"

check: check-typography check-protocol-tables check-protocol-definition test test-shared

clean:
	rm -rf build
