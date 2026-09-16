# ROOTKIT — entrada única al proyecto.
#
#   make test     todas las pruebas: firmware y Hub
#   make firmware pruebas del firmware
#   make hub      pruebas del Hub
#   make sim      los seis modelos en una ventana, en vivo
#   make bench    medición del costo de renderizar una cara
#   make sheet    los 6 modelos x 11 animos
#   make etapas   las 5 etapas de crecimiento, por modelo
#   make revelado el primer encendido, por modelo
#   make catalogo sincroniza el catalogo del Hub con el del firmware
#   make serve    servidor de desarrollo del Hub
#   make verify   lo que corre CI: pruebas + arte y referencias al día
#   make clean

.PHONY: all test firmware hub sim bench sheet etapas revelado golden catalogo serve verify clean

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

etapas:
	@$(MAKE) -C firmware --no-print-directory etapas

revelado:
	@$(MAKE) -C firmware --no-print-directory revelado

catalogo:
	@python3 tools/sync_catalog.py

golden:
	@$(MAKE) -C firmware --no-print-directory golden

serve:
	@node hub/dev-server.mjs

# Lo mismo que corre CI. Además de las pruebas verifica que los hashes de
# regresión visual estén commiteados al día: si alguien toca el rig de caras
# y se olvida de regenerarlos, acá salta en vez de descubrirse semanas
# después con una captura vieja.
verify: test
	@echo
	@echo "  verificando que el catalogo de la app siga al firmware"
	@python3 tools/sync_catalog.py --check
	@echo "  verificando que las referencias visuales esten al dia"
	@$(MAKE) -C firmware --no-print-directory golden > /dev/null
	@# git tiene que poder LEER el repositorio, y hay que comprobarlo mirando
	@# su SALIDA y no su codigo de retorno: desde WSL sobre /mnt/c, git falla
	@# con "dubious ownership", lo escribe en stderr y aun asi devuelve 0. Con
	@# la salida vacia, el chequeo de abajo lo leia como "no hay cambios" y
	@# pasaba en verde sin haber mirado nada.
	@if [ -z "$$(git rev-parse --show-toplevel 2>/dev/null)" ]; then \
	    echo "  FALLA: git no puede leer este repositorio, asi que no hay"; \
	    echo "  forma de saber si el arte generado esta al dia."; \
	    echo "  Desde WSL sobre /mnt/c hace falta, una sola vez:"; \
	    echo "    git config --global --add safe.directory $$(pwd)"; \
	    exit 1; \
	 fi
	@if [ -n "$$(git status --porcelain firmware/art firmware/test/golden.h)" ]; then \
	    echo "  FALLA: hay que regenerar y commitear:"; \
	    git status --porcelain firmware/art firmware/test/golden.h; \
	    exit 1; \
	 else \
	    echo "  arte y referencias al dia"; \
	 fi

clean:
	@$(MAKE) -C firmware --no-print-directory clean
