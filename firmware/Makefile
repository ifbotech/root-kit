# ROOTKIT — build del firmware, los tests y el simulador de escritorio.
#
#   make test     corre todas las suites (no necesita SDL ni hardware)
#   make sim      los seis modelos en una ventana, en vivo
#   make sheet    los 6 modelos x 11 animos  -> build/sheet.bmp
#   make etapas   las 5 etapas de crecimiento -> build/etapas.bmp
#   make despertar los ojos se abren          -> build/despertar.bmp
#   make pantallas QR, dormida, despertar y cara -> build/pantallas.bmp
#   make wasm     el renderer para la app web -> build/rootkit_caras.wasm
#   make bench    mide el costo de renderizar una cara
#   make golden   regenera los hashes de referencia visual
#   make clean
#
# En Windows: correr dentro de WSL. Ver docs/entorno.md.

CC      ?= cc
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -O2
BUILD    = build

CORE     = core/species.c core/mood.c core/vinculo.c core/persona.c core/node.c \
           core/sha256.c core/codigo.c core/enlace.c
GFX      = gfx/fb.c gfx/aa.c gfx/font.c gfx/panel.c
ART      = art/look.c art/face.c
UI       = ui/cara.c ui/despertar.c ui/qr.c
NET      = net/json.c net/nube.c
NODO     = nodo/soil.c nodo/power.c nodo/sampler.c nodo/sensores.c nodo/historial.c
TERCEROS = third_party/qrcodegen/qrcodegen.c

# Todo lo que no depende de SDL ni de una pantalla.
LIB      = $(CORE) $(GFX) $(ART) $(UI) $(NET) $(NODO) $(TERCEROS)

TESTSRC  = test/rk_test.c test/golden_util.c \
           test/test_mood.c test/test_nodo.c test/test_sensores.c \
           test/test_gfx.c test/test_render.c test/test_persona.c \
           test/test_enlace.c test/test_red.c test/test_qr.c test/main.c

HEADERS  = $(wildcard core/*.h gfx/*.h art/*.h ui/*.h net/*.h nodo/*.h third_party/*/*.h)

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs   sdl2 2>/dev/null)
ifneq ($(strip $(SDL_LIBS)),)
  SDL_DEF = -DRK_WITH_SDL
endif

.PHONY: all test sim sheet etapas despertar pantallas wasm bench golden clean

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

etapas: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --etapas $(BUILD)/etapas.bmp

despertar: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --despertar $(BUILD)/despertar.bmp

pantallas: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --pantallas $(BUILD)/pantallas.bmp

# ---- las caras para la app web ------------------------------------------------
# El mismo renderer, compilado a WebAssembly sin libc. Necesita clang y lld
# (apt install clang lld). La app de root-lab lo carga desde public/caras/.
WASM_SRC = gfx/fb.c gfx/aa.c gfx/font.c art/look.c art/face.c core/persona.c \
           core/mood.c core/species.c core/vinculo.c core/sha256.c core/codigo.c \
           core/enlace.c nodo/power.c ui/cara.c ui/despertar.c ui/qr.c \
           third_party/qrcodegen/qrcodegen.c wasm/caras.c

wasm: $(BUILD)/rootkit_caras.wasm

$(BUILD)/rootkit_caras.wasm: $(WASM_SRC) $(HEADERS)
	@mkdir -p $(BUILD)
	clang --target=wasm32 -std=c99 -O2 -Wall -ffreestanding -nostdlib \
	    -Iwasm/include -Wl,--no-entry -Wl,--strip-all -o $@ $(WASM_SRC)
	@ls -la $@

bench: $(BUILD)/rootkit_sim
	@./$(BUILD)/rootkit_sim --bench

clean:
	rm -rf $(BUILD)
