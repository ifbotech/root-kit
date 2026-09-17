# ROOTKIT — entrada única al proyecto.
#
#   make test       las pruebas del firmware (no necesitan placa)
#   make sim        los cinco Rooties en una ventana, en vivo
#   make sheet      los 5 Rooties x 11 animos
#   make pieles     las 3 pieles de cada Rooti (comun, rara, epica)
#   make etapas     las 5 etapas de crecimiento, por personaje
#   make despertar  los ojos se abren, por personaje
#   make transicion el cambio de animo, cuadro a cuadro
#   make pantallas  QR, dormida, despertar y cara en los dos paneles
#   make capturas   regenera las imagenes de tools/preview
#   make wasm       el renderer compilado para la app (root-lab)
#   make placa      compila las cuatro variantes con PlatformIO
#   make bench      costo de renderizar una cara
#   make golden     regenera las referencias visuales
#   make verify     lo que corre CI: pruebas + referencias al dia
#   make clean
#
# La app, la nube y el emulador viven en github.com/ifbotech/root-lab.

.PHONY: all test sim sheet pieles etapas despertar pantallas capturas wasm placa bench golden verify clean

all: test

test:
	@$(MAKE) -C firmware --no-print-directory test

sim sheet pieles etapas despertar pantallas wasm bench golden:
	@$(MAKE) -C firmware --no-print-directory $@

capturas:
	@$(MAKE) -C firmware --no-print-directory sheet pieles etapas despertar transicion pantallas
	@python3 tools/bmp2png.py firmware/build/sheet.bmp tools/preview/sheet.png
	@python3 tools/bmp2png.py firmware/build/pieles.bmp tools/preview/pieles.png
	@python3 tools/bmp2png.py firmware/build/etapas.bmp tools/preview/etapas.png
	@python3 tools/bmp2png.py firmware/build/despertar.bmp tools/preview/despertar.png
	@python3 tools/bmp2png.py firmware/build/transicion.bmp tools/preview/transicion.png
	@python3 tools/bmp2png.py firmware/build/pantallas.bmp tools/preview/pantallas.png

placa:
	@cd firmware && python3 -m platformio run -e c3-22 -e c3-144 -e devkit-22 -e devkit-144

# Lo mismo que corre CI. Además de las pruebas verifica que los hashes de
# regresión visual estén commiteados al día: si alguien toca el rig de caras
# y se olvida de regenerarlos, acá salta en vez de descubrirse semanas
# después con una captura vieja.
verify: test
	@echo
	@echo "  verificando que las referencias visuales esten al dia"
	@$(MAKE) -C firmware --no-print-directory golden > /dev/null
	@# git tiene que poder LEER el repositorio, y hay que comprobarlo mirando
	@# su SALIDA y no su codigo de retorno: desde WSL sobre /mnt/c, git falla
	@# con "dubious ownership", lo escribe en stderr y aun asi devuelve 0.
	@if [ -z "$$(git rev-parse --show-toplevel 2>/dev/null)" ]; then \
	    echo "  FALLA: git no puede leer este repositorio."; \
	    echo "  Desde WSL sobre /mnt/c hace falta, una sola vez:"; \
	    echo "    git config --global --add safe.directory $$(pwd)"; \
	    exit 1; \
	 fi
	@if [ -n "$$(git status --porcelain firmware/test/golden.h)" ]; then \
	    echo "  FALLA: hay que regenerar y commitear:"; \
	    git status --porcelain firmware/test/golden.h; \
	    exit 1; \
	 else \
	    echo "  referencias al dia"; \
	 fi

clean:
	@$(MAKE) -C firmware --no-print-directory clean
