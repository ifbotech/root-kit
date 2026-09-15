# ROOTKIT — entrada única al proyecto.
#
#   make test     todas las pruebas: firmware y Hub
#   make firmware pruebas del firmware
#   make hub      pruebas del Hub
#   make sim      el kit entero en una ventana: Prime + Minis
#   make bench    medición del rasterizado en los dos paneles
#   make sheet    hoja de contacto del Prime, todos los animos
#   make minis    hoja de contacto del Mini, todos los animos
#   make brotes   los 12 simbiontes en sus 5 etapas de crecimiento
#   make ceremonia  la apertura de capsula, por rareza
#   make catalogo sincroniza el catalogo del Hub con el del firmware
#   make serve    servidor de desarrollo del Hub
#   make verify   lo que corre CI: pruebas + arte y referencias al día
#   make clean

.PHONY: all test firmware hub sim bench sheet minis brotes ceremonia golden art catalogo serve verify clean

all: test

test: firmware hub

firmware:
	@$(MAKE) -C firmware --no-print-directory test

# El firmware se compila en WSL y el Hub corre en Node. Si Node no está en el
# mismo entorno, avisamos en vez de romper: `make test` tiene que servir desde
# los dos lados.
hub:
	@echo
	@echo "  ROOTKIT — Hub"
	@echo "  ============="
	@if command -v node > /dev/null 2>&1; then \
	    node --test hub/test/hub.test.mjs; \
	 else \
	    echo "  node no esta en este entorno: corre 'make hub' desde Windows"; \
	    echo "  (o instalalo en WSL con: sudo apt install nodejs)"; \
	 fi

sim:
	@$(MAKE) -C firmware --no-print-directory sim

bench:
	@$(MAKE) -C firmware --no-print-directory bench

sheet:
	@$(MAKE) -C firmware --no-print-directory sheet

minis:
	@$(MAKE) -C firmware --no-print-directory minis

brotes:
	@$(MAKE) -C firmware --no-print-directory brotes

ceremonia:
	@$(MAKE) -C firmware --no-print-directory ceremonia

catalogo:
	@python3 tools/sync_catalog.py

golden:
	@$(MAKE) -C firmware --no-print-directory golden

art:
	@python3 tools/gen_art.py

serve:
	@node hub/dev-server.mjs

# Lo mismo que corre CI. Además de las pruebas verifica que el arte generado y
# los hashes de referencia estén commiteados al día: si alguien toca
# gen_art.py y se olvida de regenerar, acá salta en vez de descubrirse
# semanas después con una captura vieja.
verify: test
	@echo
	@echo "  verificando que el catalogo del Hub siga al firmware"
	@python3 tools/sync_catalog.py --check
	@echo "  verificando que el arte y las referencias esten al dia"
	@python3 tools/gen_art.py > /dev/null
	@$(MAKE) -C firmware --no-print-directory golden > /dev/null
	@if [ -n "$$(git status --porcelain firmware/art firmware/test/golden.h)" ]; then \
	    echo "  FALLA: hay que regenerar y commitear:"; \
	    git status --porcelain firmware/art firmware/test/golden.h; \
	    exit 1; \
	 else \
	    echo "  arte y referencias al dia"; \
	 fi

clean:
	@$(MAKE) -C firmware --no-print-directory clean
