# Host side build: this device's pure logic and its tests, with no SDK
# present. The FAP itself is built by ufbt (see README.md); this file never
# touches it. The protocol library, the fuzz harness, the test harness and
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

# The SDK's own warning set (sdk.opts in the deployed SDK, recorded in
# docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md 4.1), which the device build
# applies to everything including vendored code, plus the two extras this
# project adds for its own sources. Vendored code is held to the SDK set on
# the host too, exactly as on the device.
SDK_WARNINGS := -std=gnu2x -Wall -Wextra -Werror -Wstrict-prototypes -Wredundant-decls \
                -Wdouble-promotion -Wundef
HOST_WARNINGS := $(SDK_WARNINGS) -Wshadow -Wconversion
# The shared code, reached by relative path and by no other means
# (peripheral spec 2.4).
SHARED_DIR := ../shared
HOST_CFLAGS := $(HOST_WARNINGS) -O1 -g -I$(SHARED_DIR)/lib/qrcodegen
VENDORED_CFLAGS := $(SDK_WARNINGS) -O1 -g
SANITISER_CFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
# Heap functions are wrapped in every test binary so a test can prove that
# the code under it did not allocate (specification 0.10, FE3). The wrappers
# live in ../shared/tests/test_support.h.
ALLOCATION_WRAP_LDFLAGS := -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free

BUILD_DIR := build/host

PROTOCOL_SOURCES := $(SHARED_DIR)/protocol/remote_protocol_tables.c $(SHARED_DIR)/protocol/remote_protocol.c

# Every pure logic module, compiled into every suite. Adding a module here is
# the only change needed for the tests to see it.
PURE_LOGIC_SOURCES := remote_input/remote_input_model.c \
                      remote_display/remote_display_layout.c \
                      remote_display/remote_display_fixtures.c \
                      remote_display/remote_font_metrics.c \
                      remote_display/remote_font_measure.c \
                      remote_display/remote_qr.c \
                      remote_display/remote_ndef.c \
                      $(PROTOCOL_SOURCES) \
                      $(SHARED_DIR)/peer/development_peer_core.c \
                      $(SHARED_DIR)/session/remote_session.c \
                      session_device/remote_session_device.c
PURE_LOGIC_HEADERS := $(wildcard remote_input/*.h remote_display/*.h $(SHARED_DIR)/protocol/*.h $(SHARED_DIR)/peer/*.h $(SHARED_DIR)/session/*.h session_device/*.h $(SHARED_DIR)/lib/qrcodegen/*.h)

# The vendored encoder is a private library on the device (application.fam)
# and is compiled once per warning set here.
VENDORED_SOURCES := $(SHARED_DIR)/lib/qrcodegen/qrcodegen.c
VENDORED_OBJECT := $(BUILD_DIR)/vendored_qrcodegen.o
VENDORED_SANITISED_OBJECT := $(BUILD_DIR)/vendored_qrcodegen_sanitised.o

# One binary per suite, named after its source.
TEST_SOURCES := $(wildcard tests/test_*.c)
TEST_HEADERS := $(SHARED_DIR)/tests/test_support.h
TEST_BINARIES := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SOURCES))
SANITISED_TEST_BINARIES := $(patsubst tests/%.c,$(BUILD_DIR)/%_sanitised,$(TEST_SOURCES))

.PHONY: test test-shared test-sanitise peer check-typography check-protocol-tables check clean

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

# The development peer lives under ../shared/ and builds from there (POSIX,
# so Linux and WSL); this target exists so the command this device's
# documents name still works from this directory.
peer:
	$(MAKE) -C $(SHARED_DIR) peer CC="$(CC)"

# The tree wide scan and the protocol checks live under ../shared/ and run
# over the whole tree; these targets exist so the commands this device's
# documents name still work from this directory.
check-typography:
	$(MAKE) -C $(SHARED_DIR) check-typography PYTHON="$(PYTHON)"

check-protocol-tables:
	$(MAKE) -C $(SHARED_DIR) check-protocol-tables PYTHON="$(PYTHON)"

check: check-typography check-protocol-tables test test-shared

clean:
	rm -rf build
