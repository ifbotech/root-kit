# ROOTKIT — build del firmware, los tests y el simulador de escritorio.
#
#   make test     corre todas las suites (no necesita SDL ni hardware)
#   make sim      el kit entero en una ventana: Prime + Minis, en vivo
#   make sheet    hoja de contacto del Prime  -> build/sheet.bmp
#   make minis    hoja de contacto del Mini   -> build/minis.bmp
#   make brotes   los 12 brotes x 5 etapas    -> build/brotes.bmp
#   make ceremonia  la apertura por rareza    -> build/gacha.bmp
#   make bench    mide el costo de renderizar un cuadro en los dos paneles
#   make golden   regenera los hashes de referencia visual
#   make art      regenera el arte desde tools/gen_art.py
#   make clean
#
# En Windows: correr dentro de WSL. Ver docs/entorno.md.

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2
BUILD    = build

CORE     = core/species.c core/mood.c core/companion.c core/node.c
GFX      = gfx/fb.c gfx/font.c gfx/panel.c
ART      = art/sprites.c art/look.c art/adulto.c art/brote.c
UI       = ui/prime.c ui/mini.c ui/gacha.c
NET      = net/proto.c net/link.c
NODO     = nodo/soil.c nodo/power.c nodo/sampler.c

# Todo lo que no depende de SDL ni de una pantalla.
LIB      = $(CORE) $(GFX) $(ART) $(UI) $(NET) $(NODO)

TESTSRC  = test/rk_test.c test/golden_util.c \
           test/test_mood.c test/test_proto.c test/test_nodo.c \
           test/test_gfx.c test/test_render.c test/test_companion.c \
           test/test_mini.c test/test_link.c test/main.c

HEADERS  = $(wildcard core/*.h gfx/*.h art/*.h ui/*.h net/*.h nodo/*.h)

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs   sdl2 2>/dev/null)
ifneq ($(strip $(SDL_LIBS)),)
  SDL_DEF = -DRK_WITH_SDL
endif

.PHONY: all test sim sheet minis brotes ceremonia bench golden art clean

all: test

# ---- pruebas ----------------------------------------------------------------
test: $(BUILD)/rk_test
	@./$(BUILD)/rk_test

$(BUILD)/rk_test: $(LIB) $(TESTSRC) $(HEADERS) test/golden.h test/rk_test.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(LIB) $(TESTSRC)

# ---- referencias visuales ----------------------------------------------------
# golden.h se genera, pero se versiona: el diff es la revision del cambio visual.
test/golden.h: | $(BUILD)/gen_golden
	@./$(BUILD)/gen_golden test/golden.h

golden: $(BUILD)/gen_golden
	@./$(BUILD)/gen_golden test/golden.h

$(BUILD)/gen_golden: $(LIB) test/golden_util.c test/gen_golden.c $(HEADERS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(LIB) test/golden_util.c test/gen_golden.c

# ---- simulador ----------------------------------------------------------------
sim: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim

$(BUILD)/rootkit_sim: $(LIB) sim/main.c $(HEADERS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(SDL_DEF) $(SDL_CFLAGS) -o $@ \
	    $(LIB) sim/main.c $(SDL_LIBS)

sheet: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --sheet $(BUILD)/sheet.bmp

minis: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --minis $(BUILD)/minis.bmp

brotes: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --brotes $(BUILD)/brotes.bmp

ceremonia: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --gacha $(BUILD)/gacha.bmp

bench: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --bench

# ---- arte -----------------------------------------------------------------------
art:
	python3 ../tools/gen_art.py

clean:
	rm -rf $(BUILD)
