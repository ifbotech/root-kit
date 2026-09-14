# ROOTKIT — build del núcleo, los tests y el simulador de escritorio.
#
#   make test     tests del núcleo (no necesita SDL ni hardware)
#   make sim      simulador interactivo, ventana 320x480
#   make sheet    hoja de contacto con todos los ánimos -> build/sheet.bmp
#   make clean
#
# En Windows: correr dentro de WSL. Ver docs/simulador-lvgl.md.

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2
BUILD    = build

CORE     = core/species.c core/mood.c
GFX      = gfx/fb.c gfx/font.c
ART      = art/tuga_data.c art/tuga.c
UI       = ui/screen.c
SIMSRC   = sim/main.c

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs   sdl2 2>/dev/null)
ifneq ($(strip $(SDL_LIBS)),)
  SDL_DEF = -DRK_WITH_SDL
endif

HEADERS = core/mood.h core/species.h core/telemetry.h \
          gfx/fb.h gfx/font.h art/tuga.h art/tuga_data.h ui/screen.h

.PHONY: all test sim sheet art clean

all: test sim

# ---- tests del núcleo: sin gráficos, sin dependencias -----------------------
test: $(BUILD)/test_mood
	@echo
	@./$(BUILD)/test_mood

$(BUILD)/test_mood: $(CORE) test/test_mood.c $(HEADERS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(CORE) test/test_mood.c

# ---- simulador ---------------------------------------------------------------
sim: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim

$(BUILD)/rootkit_sim: $(CORE) $(GFX) $(ART) $(UI) $(SIMSRC) $(HEADERS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(SDL_DEF) $(SDL_CFLAGS) -o $@ \
	    $(CORE) $(GFX) $(ART) $(UI) $(SIMSRC) $(SDL_LIBS)

sheet: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --sheet $(BUILD)/sheet.bmp

# ---- regenerar el arte a partir del script de autoría ------------------------
art:
	python3 ../tools/gen_art.py

clean:
	rm -rf $(BUILD)
