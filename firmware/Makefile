# Build del núcleo de ROOTKIT — sin LVGL, sin SDL, sin hardware.
# Sirve para desarrollar y testear la lógica del simbionte desde el día uno.
#
#   make test     compila y corre los tests
#   make clean
#
# En Windows: correr dentro de WSL (ver docs/simulador-lvgl.md).

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2
CORE     = core/species.c core/mood.c
BUILD    = build

.PHONY: all test clean

all: test

test: $(BUILD)/test_mood
	@echo
	@./$(BUILD)/test_mood

$(BUILD)/test_mood: $(CORE) test/test_mood.c core/mood.h core/species.h core/telemetry.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(CORE) test/test_mood.c

clean:
	rm -rf $(BUILD)
