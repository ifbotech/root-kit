# ROOTKIT — entrada única al proyecto.
#
#   make test       las pruebas del firmware (no necesitan placa)
#   make sim        los cinco Rooties en una ventana, en vivo
#   make sheet      los 5 Rooties x 11 animos
#   make pieles     las 3 pieles de cada Rooti (comun, rara, epica)
#   make etapas     las 5 etapas de crecimiento, por personaje
#   make despertar  los ojos se abren, por personaje
#   make transicion el cambio de animo, cuadro a cuadro
#   make pantallas  QR, dormida, despertar y cara
#   make capturas   regenera las imagenes de tools/preview
#   make wasm       el renderer compilado para la app (root-lab)
#   make pcb        verifica el sustrato y regenera plantilla, netlist y STL
#   make placa      compila el producto (c3-144) y el banco (devkit-144)
#   make bench      costo de renderizar una cara
#   make golden     regenera las referencias visuales
#   make verify     lo que corre CI: pruebas + referencias al dia
#   make clean
#
# La app, la nube y el emulador viven en github.com/ifbotech/root-lab.

.PHONY: all test sim sheet pieles etapas despertar pantallas capturas wasm placa pcb bench golden verify clean

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
	@cd firmware && python3 -m platformio run -e c3-144 -e devkit-144

# El sustrato impreso: verifica separaciones, anchos, conectividad y la zona
# libre de la antena, y reescribe lo que se genera del dato (la plantilla de
# la cinta, el diagrama de conexiones y la netlist que mira `make test`).
# El STL necesita OpenSCAD; sin el, el resto se genera igual.
pcb:
	@python3 tools/pcb.py --generar
	@if command -v openscad > /dev/null 2>&1; then \
	    python3 tools/pcb.py --stl; \
	 else \
	    echo "  (sin OpenSCAD: el STL queda como estaba)"; \
	 fi

# Lo mismo que corre CI. Además de las pruebas verifica que los hashes de
# regresión visual estén commiteados al día: si alguien toca el rig de caras
# y se olvida de regenerarlos, acá salta en vez de descubrirse semanas
# después con una captura vieja.
verify: test
	@echo
	@echo "  verificando el sustrato impreso"
	@python3 tools/pcb.py --verificar
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
	@python3 tools/pcb.py --generar > /dev/null
	@if [ -n "$$(git status --porcelain firmware/test/redes.h docs/conexiones.md hardware/pcb/generado)" ]; then \
	    echo "  FALLA: el sustrato cambio y falta correr 'make pcb' y commitear:"; \
	    git status --porcelain firmware/test/redes.h docs/conexiones.md hardware/pcb/generado; \
	    exit 1; \
	 else \
	    echo "  el sustrato y sus generados estan al dia"; \
	 fi

clean:
	@$(MAKE) -C firmware --no-print-directory clean
