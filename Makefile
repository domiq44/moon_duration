# Makefile — build du CLI et du module WebAssembly, tests, lint et couverture.

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -Iinclude
LDFLAGS = -lm

# Version : etrique Git la plus proche (release GitHub), sinon valeur de repli.
# Un build depuis une archive sans depot Git obtient la valeur de repli.
VERSION ?= $(shell git describe --tags --dirty --match 'v[0-9]*' 2>/dev/null)
ifeq ($(strip $(VERSION)),)
VERSION := 0.0.0-dev
endif

# Repertoires de sortie
BUILD_DIR   = build
OBJ_DIR     = $(BUILD_DIR)/obj
BIN_DIR     = $(BUILD_DIR)/bin
COV_DIR     = coverage
COV_OBJ_DIR = $(COV_DIR)/obj
COV_BIN_DIR = $(COV_DIR)/bin
COV_REPORT  = $(COV_DIR)/report

# Installation
PREFIX      ?= /usr/local
DESTDIR     ?=
BIN_INSTALL_DIR = $(DESTDIR)$(PREFIX)/bin

# Sources
LIB_SRCS = src/datetime.c src/clock.c src/astro.c src/sun_position.c \
           src/moon_position.c src/moon_events.c src/moon_duration.c \
           src/moon_phase.c src/moon_phase_name.c src/moon_orientation.c \
           src/moon_ascii.c src/i18n.c src/cli.c src/report.c src/web_api.c
SRCS = $(LIB_SRCS) src/main.c
LIB_OBJS = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
OBJS = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Tests
TEST_SRCS = tests/test_datetime.c tests/test_clock.c tests/test_astro.c \
            tests/test_moon_position.c tests/test_sun_position.c \
            tests/test_moon_events.c tests/test_moon_duration.c \
            tests/test_moon_duration_reference.c \
            tests/test_moon_phase.c tests/test_moon_orientation.c \
            tests/test_moon_ascii.c tests/test_cli.c \
            tests/test_i18n.c tests/test_report.c tests/test_web_api.c \
            tests/test_web_pages.c tests/test_integration.c
TEST_RUNNER_SRC = tests/support/test_runner.c
TEST_RUNNER_OBJ = $(OBJ_DIR)/test_runner.o
TEST_BINS = $(patsubst tests/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

# WebAssembly
EMCC ?= emcc
NODE_CMD ?= node
PYTHON_CMD ?= python3
WEB_PORT ?= 8000
WASM_MODULE := web/js/moon_core.mjs
WASM_SOURCES := src/datetime.c src/astro.c src/sun_position.c \
                src/moon_position.c src/moon_events.c src/moon_duration.c \
                src/moon_phase.c src/moon_orientation.c src/moon_ascii.c \
                src/web_api.c
# Emscripten n'exporte par defaut que les symboles de l'utilisateur visibles ;
# malloc et free doivent etre listes explicitement pour l'allocation des
# pointeurs de sortie (lever, coucher) cote JavaScript.
WASM_EXPORTS := _moon_web_julian_day,_moon_web_duration_hours,_moon_web_duration_mode,_moon_web_event_julian_day,_moon_web_window,_moon_web_phase,_moon_web_phase_at,_moon_web_illumination,_moon_web_illumination_at,_moon_web_altitude,_moon_web_ascii_size,_moon_web_ascii_into,_moon_web_bright_limb_angle,_moon_web_bright_limb_angle_at,_malloc,_free
WASM_RUNTIME_METHODS := UTF8ToString,getValue

# Exécutable principal
TARGET = $(BIN_DIR)/moon_duration

# Couverture de code
COVERAGE_FLAGS = --coverage -O0 -g
COV_OBJS = $(patsubst src/%.c,$(COV_OBJ_DIR)/%.o,$(LIB_SRCS))
COV_MAIN_CLI_DIR = $(COV_OBJ_DIR)/cli_main
COV_TEST_RUNNER_OBJ = $(COV_OBJ_DIR)/test_runner.o
COV_TEST_BINS = $(patsubst tests/%.c,$(COV_BIN_DIR)/%_cov,$(TEST_SRCS))
COVERAGE_FORMATTER = scripts/coverage_report.awk
COVERAGE_CHECKER = scripts/coverage_check.awk
# Seuils de couverture (pourcentages) verifies par coverage-check.
COVERAGE_MIN_LINES ?= 90
COVERAGE_MIN_BRANCHES ?= 75

# Sanitaires (ASan + UBSan) et fuzzing
SAN_DIR = $(BUILD_DIR)/asan
SAN_OBJ_DIR = $(SAN_DIR)/obj
SAN_BIN_DIR = $(SAN_DIR)/bin
SAN_FLAGS = -fsanitize=address,undefined -fno-sanitize-recover=all \
            -fno-omit-frame-pointer -g -O1
SAN_OBJS = $(patsubst src/%.c,$(SAN_OBJ_DIR)/%.o,$(LIB_SRCS))
SAN_MAIN_OBJ = $(SAN_OBJ_DIR)/main.o
SAN_MAIN_CLI_OBJ = $(SAN_OBJ_DIR)/main_cli.o
SAN_TEST_RUNNER_OBJ = $(SAN_OBJ_DIR)/test_runner.o
SAN_TARGET = $(SAN_BIN_DIR)/moon_duration
SAN_TEST_BINS = $(patsubst tests/%.c,$(SAN_BIN_DIR)/%,$(TEST_SRCS))
FUZZ_SRC = tests/fuzz/fuzz_smoke.c
FUZZ_BIN = $(SAN_BIN_DIR)/fuzz_smoke
FUZZ_ITERS ?= 5000

.PHONY: all clean test run lint cppcheck format format-check coverage install uninstall FORCE

# FORCE est declare avant all : sans cette directive, make ne construirait rien.
.DEFAULT_GOAL := all

FORCE:
.PHONY: web_wasm test_web_wasm test_date_time test_chart test_theme test_city_picker test_preview_offsets run_web
.PHONY: check-test-sources
.PHONY: test_sanitize fuzz
.PHONY: coverage-check

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Seul main.c affiche la version : elle est injectee a la compilation.
# Le fichier de version n'est recopie que si la valeur a change, afin de ne
# recompiler main.c qu'apres une nouvelle etrique.
$(BUILD_DIR)/version.txt: FORCE | $(BUILD_DIR)
	@echo "$(VERSION)" | cmp -s - $@ 2>/dev/null || echo "$(VERSION)" > $@

$(OBJ_DIR)/main.o: src/main.c $(BUILD_DIR)/version.txt | $(OBJ_DIR)
	$(CC) $(CFLAGS) -DMOON_VERSION=\"$(VERSION)\" -c $< -o $@

$(TEST_RUNNER_OBJ): $(TEST_RUNNER_SRC) tests/support/test_runner.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%: tests/%.c $(LIB_OBJS) $(TEST_RUNNER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_OBJS) $(TEST_RUNNER_OBJ) $(LDFLAGS)

$(OBJ_DIR)/main_cli.o: src/main.c $(BUILD_DIR)/version.txt | $(OBJ_DIR)
	$(CC) $(CFLAGS) -DMOON_VERSION=\"$(VERSION)\" -Dmain=moon_program_main -c $< -o $@

$(BIN_DIR)/test_cli: tests/test_cli.c $(LIB_OBJS) $(TEST_RUNNER_OBJ) $(OBJ_DIR)/main_cli.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -DMOON_VERSION=\"$(VERSION)\" -o $@ $< $(LIB_OBJS) $(OBJ_DIR)/main_cli.o $(TEST_RUNNER_OBJ) $(LDFLAGS)

$(BIN_DIR)/test_integration: tests/test_integration.c $(LIB_OBJS) $(TEST_RUNNER_OBJ) $(TARGET) | $(BIN_DIR)
	$(CC) $(CFLAGS) -DMOON_DURATION_BIN=\"$(CURDIR)/$(TARGET)\" -o $@ $< $(LIB_OBJS) $(TEST_RUNNER_OBJ) $(LDFLAGS)

$(OBJ_DIR) $(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

# Tests unitaires et integration
# Verifie que chaque fichier de test C est bien compile par la suite : un
# fichier absent de TEST_SRCS n'est jamais execute et sa couverture est nulle.
check-test-sources:
	@echo "Checking test sources are wired into TEST_SRCS..."
	@missing=0; \
	for src in tests/test_*.c; do \
		if ! echo " $(TEST_SRCS) " | grep -q " $$src "; then \
			echo "Erreur: $$src absent de TEST_SRCS" >&2; \
			missing=1; \
		fi; \
	done; \
	if [ $$missing -ne 0 ]; then \
		exit 1; \
	fi

test: check-test-sources $(TEST_BINS)
	@echo "Running all tests..."
	@for t in $(TEST_BINS); do \
		echo "=== $$t"; \
		./$$t || exit 1; \
	done

run: $(TARGET)
	./$(TARGET)

# WebAssembly et page web
$(WASM_MODULE): $(WASM_SOURCES) include/datetime.h include/astro.h \
                include/sun_position.h include/moon_position.h \
                include/moon_orientation.h \
                include/moon_events.h include/moon_duration.h \
                include/moon_phase.h include/moon_ascii.h include/web_api.h
	@command -v $(EMCC) >/dev/null || { echo "Erreur: emcc est requis (Emscripten)." ; exit 1; }
	@mkdir -p $(@D)
	@$(EMCC) -std=c99 -O2 -Iinclude $(WASM_SOURCES) -lm \
		-sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node \
		-sALLOW_MEMORY_GROWTH=1 \
		-sEXPORTED_FUNCTIONS=$(WASM_EXPORTS) \
		-sEXPORTED_RUNTIME_METHODS=$(WASM_RUNTIME_METHODS) \
		-o $@

web_wasm: $(WASM_MODULE)

test_web_wasm: $(WASM_MODULE)
	@$(NODE_CMD) --test tests/web/test_web_wasm.mjs

test_date_time:
	@$(NODE_CMD) --test tests/web/test_date_time.mjs

test_format:
	@$(NODE_CMD) --test tests/web/test_format.mjs

test_values:
	@$(NODE_CMD) --test tests/web/test_values.mjs

test_cities:
	@$(NODE_CMD) --test tests/web/test_cities.mjs

test_city_picker:
	@$(NODE_CMD) --test tests/web/test_city_picker.mjs

test_settings:
	@$(NODE_CMD) --test tests/web/test_settings.mjs

test_preview: $(WASM_MODULE)
	@$(NODE_CMD) --test tests/web/test_preview.mjs

test_preview_offsets:
	@$(NODE_CMD) --test tests/web/test_preview_offsets.mjs

test_app: $(WASM_MODULE)
	@$(NODE_CMD) --test tests/web/test_app.mjs

test_chart:
	@$(NODE_CMD) --test tests/web/test_chart.mjs

test_theme:
	@$(NODE_CMD) --test tests/web/test_theme.mjs

run_web: web_wasm
	@echo "Serveur web disponible sur http://localhost:$(WEB_PORT) (Ctrl+C pour arrêter)"
	@$(PYTHON_CMD) -m http.server $(WEB_PORT) --directory web

# Couverture de code (gcov)
coverage: $(COV_TEST_BINS)
	@echo "Running tests with coverage instrumentation..."
	@for t in $(COV_TEST_BINS); do \
		./$$t > /dev/null || exit 1; \
	done
	@mkdir -p $(COV_REPORT)
	@LC_ALL=C gcov -b -c $(COV_OBJS) $(COV_MAIN_CLI_DIR)/main_cli.o > $(COV_REPORT)/summary.txt 2>/dev/null
	@mv -f ./*.gcov $(COV_REPORT)/ 2>/dev/null || true
	@echo ""
	@LC_ALL=C awk -f $(COVERAGE_FORMATTER) $(COV_REPORT)/summary.txt
	@echo ""
	@echo "Rapport detaille : $(COV_REPORT)/"

$(COV_OBJ_DIR):
	mkdir -p $(COV_OBJ_DIR)

$(COV_OBJ_DIR)/%.o: src/%.c | $(COV_OBJ_DIR)
	$(CC) $(COVERAGE_FLAGS) $(CFLAGS) -c $< -o $@

$(COV_TEST_RUNNER_OBJ): $(TEST_RUNNER_SRC) tests/support/test_runner.h | $(COV_OBJ_DIR)
	$(CC) $(COVERAGE_FLAGS) $(CFLAGS) -c $< -o $@

$(COV_BIN_DIR)/%_cov: tests/%.c $(COV_OBJS) $(COV_TEST_RUNNER_OBJ) | $(COV_BIN_DIR)
	$(CC) $(COVERAGE_FLAGS) $(CFLAGS) -o $@ $< $(COV_OBJS) $(COV_TEST_RUNNER_OBJ) $(LDFLAGS)

$(COV_MAIN_CLI_DIR)/main_cli.o: src/main.c $(BUILD_DIR)/version.txt | $(COV_MAIN_CLI_DIR)
	$(CC) $(COVERAGE_FLAGS) $(CFLAGS) -DMOON_VERSION=\"$(VERSION)\" -Dmain=moon_program_main -c $< -o $@

$(COV_MAIN_CLI_DIR):
	mkdir -p $@

$(COV_BIN_DIR)/test_cli_cov: tests/test_cli.c $(COV_OBJS) $(COV_TEST_RUNNER_OBJ) $(COV_MAIN_CLI_DIR)/main_cli.o | $(COV_BIN_DIR)
	$(CC) $(COVERAGE_FLAGS) $(CFLAGS) -DMOON_VERSION=\"$(VERSION)\" -o $@ $< $(COV_OBJS) $(COV_MAIN_CLI_DIR)/main_cli.o $(COV_TEST_RUNNER_OBJ) $(LDFLAGS)

$(COV_BIN_DIR)/test_integration_cov: tests/test_integration.c $(COV_OBJS) $(COV_TEST_RUNNER_OBJ) $(TARGET) | $(COV_BIN_DIR)
	$(CC) $(COVERAGE_FLAGS) $(CFLAGS) -DMOON_DURATION_BIN=\"$(CURDIR)/$(TARGET)\" -o $@ $< $(COV_OBJS) $(COV_TEST_RUNNER_OBJ) $(LDFLAGS)

$(COV_BIN_DIR):
	mkdir -p $(COV_BIN_DIR)

# Sanitaires : memes suites que make test, instrumentees ASan + UBSan.
$(SAN_OBJ_DIR) $(SAN_BIN_DIR):
	mkdir -p $@

$(SAN_OBJ_DIR)/%.o: src/%.c | $(SAN_OBJ_DIR)
	$(CC) $(SAN_FLAGS) $(CFLAGS) -c $< -o $@

$(SAN_MAIN_OBJ): src/main.c $(BUILD_DIR)/version.txt | $(SAN_OBJ_DIR)
	$(CC) $(SAN_FLAGS) $(CFLAGS) -DMOON_VERSION=\"$(VERSION)\" -c $< -o $@

$(SAN_TEST_RUNNER_OBJ): $(TEST_RUNNER_SRC) tests/support/test_runner.h | $(SAN_OBJ_DIR)
	$(CC) $(SAN_FLAGS) $(CFLAGS) -c $< -o $@

$(SAN_TARGET): $(SAN_OBJS) $(SAN_MAIN_OBJ) | $(SAN_BIN_DIR)
	$(CC) $(SAN_FLAGS) -o $@ $(SAN_OBJS) $(SAN_MAIN_OBJ) $(LDFLAGS)

$(SAN_MAIN_CLI_OBJ): src/main.c $(BUILD_DIR)/version.txt | $(SAN_OBJ_DIR)
	$(CC) $(SAN_FLAGS) $(CFLAGS) -DMOON_VERSION=\"$(VERSION)\" -Dmain=moon_program_main -c $< -o $@

$(SAN_BIN_DIR)/%: tests/%.c $(SAN_OBJS) $(SAN_TEST_RUNNER_OBJ) | $(SAN_BIN_DIR)
	$(CC) $(SAN_FLAGS) $(CFLAGS) -o $@ $< $(SAN_OBJS) $(SAN_TEST_RUNNER_OBJ) $(LDFLAGS)

$(SAN_BIN_DIR)/test_cli: tests/test_cli.c $(SAN_OBJS) $(SAN_TEST_RUNNER_OBJ) $(SAN_MAIN_CLI_OBJ) | $(SAN_BIN_DIR)
	$(CC) $(SAN_FLAGS) $(CFLAGS) -DMOON_VERSION=\"$(VERSION)\" -o $@ $< $(SAN_OBJS) $(SAN_MAIN_CLI_OBJ) $(SAN_TEST_RUNNER_OBJ) $(LDFLAGS)

$(SAN_BIN_DIR)/test_integration: tests/test_integration.c $(SAN_OBJS) $(SAN_TEST_RUNNER_OBJ) $(SAN_TARGET) | $(SAN_BIN_DIR)
	$(CC) $(SAN_FLAGS) $(CFLAGS) -DMOON_DURATION_BIN=\"$(CURDIR)/$(SAN_TARGET)\" -o $@ $< $(SAN_OBJS) $(SAN_TEST_RUNNER_OBJ) $(LDFLAGS)

$(FUZZ_BIN): $(FUZZ_SRC) $(SAN_OBJS) | $(SAN_BIN_DIR)
	$(CC) $(SAN_FLAGS) $(CFLAGS) -o $@ $< $(SAN_OBJS) $(LDFLAGS)

# Execute les suites et le harnais sous sanitaires.
test_sanitize: $(SAN_TEST_BINS) $(FUZZ_BIN)
	@echo "Running sanitizer-instrumented tests..."
	@for t in $(SAN_TEST_BINS); do \
		echo "=== $$t"; \
		./$$t || exit 1; \
	done
	@echo "=== fuzzing"
	@./$(FUZZ_BIN) $(FUZZ_ITERS)

# Fuzzing seul (graine fixe par defaut : FUZZ_SEED pour reproduire un echec).
fuzz: $(FUZZ_BIN)
	@./$(FUZZ_BIN) $(FUZZ_ITERS)

# Analyse statique
lint:
	@echo "Running clang-tidy..."
	clang-tidy $(SRCS) $(TEST_SRCS) $(TEST_RUNNER_SRC) $(FUZZ_SRC) -- $(CFLAGS)

cppcheck:
	@echo "Running cppcheck..."
	cppcheck --enable=all --inconclusive --std=c99 --suppress=missingIncludeSystem --suppress=staticFunction -I include src/ tests/

# Formatage du code
format:
	@echo "Running clang-format..."
	clang-format -i $(SRCS) $(TEST_SRCS) $(TEST_RUNNER_SRC) $(FUZZ_SRC)

format-check:
	@echo "Checking clang-format..."
	clang-format --dry-run --Werror $(SRCS) $(TEST_SRCS) $(TEST_RUNNER_SRC) $(FUZZ_SRC)

clean:
	rm -rf $(BUILD_DIR) $(COV_DIR)
	rm -f ./*.gcov ./*.gcda ./*.gcno
	rm -f $(WASM_MODULE) web/js/moon_core.wasm

install: $(TARGET)
	@echo "Installation de moon_duration dans $(BIN_INSTALL_DIR)"
	mkdir -p $(BIN_INSTALL_DIR)
	install -m 0755 $(TARGET) $(BIN_INSTALL_DIR)/

uninstall:
	@echo "Désinstallation de moon_duration depuis $(BIN_INSTALL_DIR)"
	rm -f $(BIN_INSTALL_DIR)/moon_duration

# Échoue si la couverture globale passe sous les seuils.
coverage-check:
	@LC_ALL=C awk -v min_lines=$(COVERAGE_MIN_LINES) \
	    -v min_branches=$(COVERAGE_MIN_BRANCHES) \
	    -f $(COVERAGE_CHECKER) $(COV_REPORT)/summary.txt

# Dependances
$(OBJ_DIR)/datetime.o: src/datetime.c include/datetime.h
$(OBJ_DIR)/clock.o: src/clock.c include/clock.h include/datetime.h
$(OBJ_DIR)/astro.o: src/astro.c include/astro.h
$(OBJ_DIR)/sun_position.o: src/sun_position.c include/sun_position.h
$(OBJ_DIR)/moon_position.o: src/moon_position.c include/moon_position.h include/astro.h include/sun_position.h
$(OBJ_DIR)/moon_events.o: src/moon_events.c include/moon_events.h include/moon_position.h
$(OBJ_DIR)/moon_duration.o: src/moon_duration.c include/moon_duration.h include/moon_events.h
$(OBJ_DIR)/moon_phase.o: src/moon_phase.c include/moon_phase.h
$(OBJ_DIR)/moon_phase_name.o: src/moon_phase_name.c include/moon_phase_name.h include/moon_phase.h include/i18n.h
$(OBJ_DIR)/moon_orientation.o: src/moon_orientation.c include/moon_orientation.h include/astro.h include/moon_phase.h include/moon_position.h include/sun_position.h
$(OBJ_DIR)/moon_ascii.o: src/moon_ascii.c include/moon_ascii.h include/moon_phase.h
$(OBJ_DIR)/i18n.o: src/i18n.c include/i18n.h include/cli_status.h include/moon_phase.h
$(OBJ_DIR)/cli.o: src/cli.c include/cli.h include/cli_status.h include/datetime.h include/i18n.h
$(OBJ_DIR)/report.o: src/report.c include/report.h include/datetime.h include/moon_phase.h include/moon_orientation.h include/moon_ascii.h include/moon_phase_name.h include/i18n.h
$(OBJ_DIR)/main.o: src/main.c include/cli.h include/cli_status.h include/clock.h include/datetime.h include/moon_duration.h include/moon_phase.h include/moon_orientation.h include/report.h
$(OBJ_DIR)/web_api.o: src/web_api.c include/web_api.h include/datetime.h include/moon_ascii.h include/moon_duration.h include/moon_phase.h include/moon_orientation.h include/moon_position.h
$(COV_OBJ_DIR)/datetime.o: src/datetime.c include/datetime.h
$(COV_OBJ_DIR)/clock.o: src/clock.c include/clock.h include/datetime.h
$(COV_OBJ_DIR)/astro.o: src/astro.c include/astro.h
$(COV_OBJ_DIR)/sun_position.o: src/sun_position.c include/sun_position.h
$(COV_OBJ_DIR)/moon_position.o: src/moon_position.c include/moon_position.h include/astro.h include/sun_position.h
$(COV_OBJ_DIR)/moon_events.o: src/moon_events.c include/moon_events.h include/moon_position.h
$(COV_OBJ_DIR)/moon_duration.o: src/moon_duration.c include/moon_duration.h include/moon_events.h
$(COV_OBJ_DIR)/moon_phase.o: src/moon_phase.c include/moon_phase.h
$(COV_OBJ_DIR)/moon_phase_name.o: src/moon_phase_name.c include/moon_phase_name.h include/moon_phase.h include/i18n.h
$(COV_OBJ_DIR)/moon_orientation.o: src/moon_orientation.c include/moon_orientation.h include/astro.h include/moon_phase.h include/moon_position.h include/sun_position.h
$(COV_OBJ_DIR)/moon_ascii.o: src/moon_ascii.c include/moon_ascii.h include/moon_phase.h
$(COV_OBJ_DIR)/i18n.o: src/i18n.c include/i18n.h include/cli_status.h include/moon_phase.h
$(COV_OBJ_DIR)/cli.o: src/cli.c include/cli.h include/cli_status.h include/datetime.h include/i18n.h
$(COV_OBJ_DIR)/report.o: src/report.c include/report.h include/datetime.h include/moon_phase.h include/moon_orientation.h include/moon_ascii.h include/moon_phase_name.h include/i18n.h
$(COV_OBJ_DIR)/web_api.o: src/web_api.c include/web_api.h include/datetime.h include/moon_ascii.h include/moon_duration.h include/moon_phase.h include/moon_orientation.h include/moon_position.h
$(SAN_OBJ_DIR)/datetime.o: src/datetime.c include/datetime.h
$(SAN_OBJ_DIR)/clock.o: src/clock.c include/clock.h include/datetime.h
$(SAN_OBJ_DIR)/astro.o: src/astro.c include/astro.h
$(SAN_OBJ_DIR)/sun_position.o: src/sun_position.c include/sun_position.h
$(SAN_OBJ_DIR)/moon_position.o: src/moon_position.c include/moon_position.h include/astro.h include/sun_position.h
$(SAN_OBJ_DIR)/moon_events.o: src/moon_events.c include/moon_events.h include/moon_position.h
$(SAN_OBJ_DIR)/moon_duration.o: src/moon_duration.c include/moon_duration.h include/moon_events.h
$(SAN_OBJ_DIR)/moon_phase.o: src/moon_phase.c include/moon_phase.h
$(SAN_OBJ_DIR)/moon_phase_name.o: src/moon_phase_name.c include/moon_phase_name.h include/moon_phase.h include/i18n.h
$(SAN_OBJ_DIR)/moon_orientation.o: src/moon_orientation.c include/moon_orientation.h include/astro.h include/moon_phase.h include/moon_position.h include/sun_position.h
$(SAN_OBJ_DIR)/moon_ascii.o: src/moon_ascii.c include/moon_ascii.h include/moon_phase.h
$(SAN_OBJ_DIR)/i18n.o: src/i18n.c include/i18n.h include/cli_status.h include/moon_phase.h
$(SAN_OBJ_DIR)/cli.o: src/cli.c include/cli.h include/cli_status.h include/datetime.h include/i18n.h
$(SAN_OBJ_DIR)/report.o: src/report.c include/report.h include/datetime.h include/moon_phase.h include/moon_orientation.h include/moon_ascii.h include/moon_phase_name.h include/i18n.h
$(SAN_OBJ_DIR)/web_api.o: src/web_api.c include/web_api.h include/datetime.h include/moon_ascii.h include/moon_duration.h include/moon_phase.h include/moon_orientation.h include/moon_position.h
$(SAN_MAIN_OBJ): src/main.c include/cli.h include/cli_status.h include/clock.h include/datetime.h include/moon_duration.h include/moon_phase.h include/moon_orientation.h include/report.h include/version.h
$(SAN_MAIN_CLI_OBJ): src/main.c include/cli.h include/cli_status.h include/clock.h include/datetime.h include/moon_duration.h include/moon_phase.h include/moon_orientation.h include/report.h include/version.h
$(TEST_RUNNER_OBJ): tests/support/test_runner.c tests/support/test_runner.h
$(COV_TEST_RUNNER_OBJ): tests/support/test_runner.c tests/support/test_runner.h
