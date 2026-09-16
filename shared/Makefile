# The host way into shared/: the protocol library, the fuzz harness and the
# shared host tests, built with gcc and make and nothing else (peripheral
# spec 0.16). The device images compile these same sources under their own
# toolchains from their own directories; this file never touches either.

# make predefines CC as "cc", which the MinGW distribution on the Windows host
# does not provide, so only a caller supplied CC survives; the default is gcc.
ifeq ($(origin CC),default)
CC := gcc
endif
PYTHON ?= python3

# The host warning set both devices' Makefiles use, which is also the union
# of every warning flag either device build enables (evaluation log 4.1), so
# a clean compile here is a clean compile under both devices' warning flags.
# What it cannot see is the devices' code generation: see DEVICE_* below.
BASE_WARNINGS := -std=gnu2x -Wall -Wextra -Werror -Wstrict-prototypes -Wredundant-decls \
                 -Wdouble-promotion -Wundef
HOST_WARNINGS := $(BASE_WARNINGS) -Wshadow -Wconversion
HOST_CFLAGS := $(HOST_WARNINGS) -O1 -g -Ilib/qrcodegen
VENDORED_CFLAGS := $(BASE_WARNINGS) -O1 -g
SANITISER_CFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
# Heap functions are wrapped in every test binary so a test can prove that
# the code under it did not allocate (Flipper 0.10). The wrappers live in
# tests/test_support.h.
ALLOCATION_WRAP_LDFLAGS := -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free

# The cheap approximation to each device build (peripheral spec 4.5): the
# host compiler with that device's warning flags, its optimisation level and
# -fshort-enums, which is the ABI default of both ARM toolchains and the one
# code generation difference that has produced a device only warning (a bounds
# check on a one byte enum, Flipper evaluation log FE3). The device jobs are
# the proof under the real toolchains; this catches the enum class on the
# host. Warning sets, optimisation and NDEBUG as read from the device builds
# in evaluation log 4.1; NDEBUG matters because it removes the vendored
# encoder's asserts, which is where the enum warning otherwise surfaces.
DEVICE_FLIPPER_CFLAGS := -Wall -Wextra -Werror -Wno-error=deprecated-declarations -Wno-address-of-packed-member \
                         -Wredundant-decls -Wdouble-promotion -Wundef -Os -fshort-enums -DNDEBUG
DEVICE_PICO_CFLAGS := -Wall -Wextra -Werror -Wstrict-prototypes -Wredundant-decls -Wdouble-promotion -Wundef \
                      -Wshadow -Wconversion -O3 -fshort-enums -DNDEBUG

BUILD_DIR := build/host

# Every shared module compiled into every suite. Adding a module here is the
# only change needed for the tests to see it.
PROTOCOL_SOURCES := protocol/remote_protocol_tables.c protocol/remote_protocol.c
LINK_SOURCES := link/remote_link_edge.c
PEER_CORE_SOURCES := peer/development_peer_core.c
SHARED_SOURCES := $(PROTOCOL_SOURCES) $(LINK_SOURCES) $(PEER_CORE_SOURCES)
SHARED_HEADERS := $(wildcard protocol/*.h link/*.h peer/*.h lib/qrcodegen/*.h)

# The vendored encoder is compiled once per warning set, held to the base
# set as on both devices (PROVENANCE.md at the root).
VENDORED_SOURCES := lib/qrcodegen/qrcodegen.c
VENDORED_OBJECT := $(BUILD_DIR)/vendored_qrcodegen.o
VENDORED_SANITISED_OBJECT := $(BUILD_DIR)/vendored_qrcodegen_sanitised.o

# The fuzz driver: the protocol library plus the harness, run for a fixed
# number of deterministic inputs. FUZZ_ITERATIONS sets how many.
FUZZ_SOURCE := fuzz/fuzz_remote_protocol.c
FUZZ_ITERATIONS ?= 200000

# The development peer: the core (pure logic, tested) plus the POSIX shell
# (termios), so it builds on Linux and WSL, not under MinGW. It is a host
# tool that stands in for the appliance and is never part of a device build.
PEER_SOURCES := peer/development_peer_shell.c $(PEER_CORE_SOURCES) $(PROTOCOL_SOURCES)

# One binary per suite, named after its source.
TEST_SOURCES := $(wildcard tests/test_*.c)
TEST_HEADERS := tests/test_support.h
TEST_BINARIES := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SOURCES))
SANITISED_TEST_BINARIES := $(patsubst tests/%.c,$(BUILD_DIR)/%_sanitised,$(TEST_SOURCES))

.PHONY: test test-sanitise fuzz fuzz-sanitise peer check-device-flags check-typography check-protocol-tables check-protocol-definition check-one-copy check-includes check clean

test: $(TEST_BINARIES)
	@for suite in $(TEST_BINARIES); do echo "== $$suite"; $$suite || exit 1; done

test-sanitise: $(SANITISED_TEST_BINARIES)
	@for suite in $(SANITISED_TEST_BINARIES); do echo "== $$suite"; $$suite || exit 1; done

$(VENDORED_OBJECT): $(VENDORED_SOURCES) $(SHARED_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(VENDORED_CFLAGS) -c -o $@ $(VENDORED_SOURCES)

$(VENDORED_SANITISED_OBJECT): $(VENDORED_SOURCES) $(SHARED_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(VENDORED_CFLAGS) $(SANITISER_CFLAGS) -c -o $@ $(VENDORED_SOURCES)

$(BUILD_DIR)/%: tests/%.c $(SHARED_SOURCES) $(SHARED_HEADERS) $(TEST_HEADERS) $(VENDORED_OBJECT)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) -o $@ $< $(SHARED_SOURCES) $(VENDORED_OBJECT) $(ALLOCATION_WRAP_LDFLAGS)

$(BUILD_DIR)/%_sanitised: tests/%.c $(SHARED_SOURCES) $(SHARED_HEADERS) $(TEST_HEADERS) $(VENDORED_SANITISED_OBJECT)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) $(SANITISER_CFLAGS) -o $@ $< $(SHARED_SOURCES) $(VENDORED_SANITISED_OBJECT) $(ALLOCATION_WRAP_LDFLAGS)

fuzz: $(BUILD_DIR)/fuzz_remote_protocol
	$(BUILD_DIR)/fuzz_remote_protocol $(FUZZ_ITERATIONS)

fuzz-sanitise: $(BUILD_DIR)/fuzz_remote_protocol_sanitised
	$(BUILD_DIR)/fuzz_remote_protocol_sanitised $(FUZZ_ITERATIONS)

$(BUILD_DIR)/fuzz_remote_protocol: $(FUZZ_SOURCE) $(PROTOCOL_SOURCES) $(SHARED_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) -o $@ $(FUZZ_SOURCE) $(PROTOCOL_SOURCES)

$(BUILD_DIR)/fuzz_remote_protocol_sanitised: $(FUZZ_SOURCE) $(PROTOCOL_SOURCES) $(SHARED_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) $(SANITISER_CFLAGS) -o $@ $(FUZZ_SOURCE) $(PROTOCOL_SOURCES)

peer: $(BUILD_DIR)/development_peer

$(BUILD_DIR)/development_peer: $(PEER_SOURCES) $(SHARED_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(HOST_CFLAGS) -o $@ $(PEER_SOURCES)

# Compile only: the objects are discarded, the exit status is the result.
check-device-flags: $(SHARED_SOURCES) $(VENDORED_SOURCES) $(SHARED_HEADERS)
	@mkdir -p $(BUILD_DIR)/device_flags
	@for source in $(SHARED_SOURCES) $(VENDORED_SOURCES); do \
		echo "== flipper flags: $$source"; \
		$(CC) $(DEVICE_FLIPPER_CFLAGS) -Ilib/qrcodegen -c -o $(BUILD_DIR)/device_flags/flipper.o $$source || exit 1; \
	done
	@for source in $(SHARED_SOURCES); do \
		echo "== pico flags: $$source"; \
		$(CC) $(DEVICE_PICO_CFLAGS) -Ilib/qrcodegen -c -o $(BUILD_DIR)/device_flags/pico.o $$source || exit 1; \
	done
	@for source in $(VENDORED_SOURCES); do \
		echo "== pico flags, vendored (base set): $$source"; \
		$(CC) $(filter-out -Wshadow -Wconversion,$(DEVICE_PICO_CFLAGS)) -Ilib/qrcodegen -c -o $(BUILD_DIR)/device_flags/pico.o $$source || exit 1; \
	done

# The tree wide scans, run from here over the whole repository.
check-typography:
	$(PYTHON) scripts/check_typography.py

check-one-copy:
	$(PYTHON) scripts/check_one_copy.py

check-includes:
	$(PYTHON) scripts/check_includes.py

# The generated parser and encoder tables must match protocol.json exactly,
# and protocol.json must be the appliance's frozen definition.
check-protocol-tables:
	$(PYTHON) scripts/generate_protocol_tables.py --check

check-protocol-definition:
	$(PYTHON) scripts/check_protocol_definition.py

check: check-typography check-one-copy check-includes check-protocol-tables check-protocol-definition test fuzz check-device-flags

clean:
	rm -rf build
