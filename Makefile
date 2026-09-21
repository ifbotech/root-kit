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
#   make pcb        verifica el sustrato y la estampadora, y regenera todo
#   make rutear     vuelve a rutear la placa (despues de mover un modulo)
#   make cupon      el cupon de prueba de canaletas y su estampadora
#   make placa      compila el producto (c3-144) y el banco (devkit-144)
#   make bench      costo de renderizar una cara
#   make golden     regenera las referencias visuales
#   make verify     lo que corre CI: pruebas + referencias al dia
#   make clean
#
# La app, la nube y el emulador viven en github.com/ifbotech/root-lab.

# OpenSCAD se llama distinto segun desde donde se corra: `openscad` en
# Linux, `openscad.exe` desde WSL sobre una instalacion de Windows, y ahi
# ademas no esta en el PATH. Se resuelve una sola vez aca y se pasa por el
# entorno, en vez de que cada objetivo adivine. Sin esto, `make pcb` desde
# WSL decia "sin OpenSCAD" y seguia como si nada: los STL quedaban viejos
# y nadie se enteraba.
OPENSCAD := $(shell command -v openscad 2> /dev/null \
                 || command -v openscad.exe 2> /dev/null \
                 || ls "/mnt/c/Program Files/OpenSCAD/openscad.exe" 2> /dev/null \
                 || ls "/c/Program Files/OpenSCAD/openscad.exe" 2> /dev/null)
export OPENSCAD

.PHONY: all test sim sheet pieles etapas despertar pantallas capturas wasm placa pcb rutear cupon bench golden verify clean

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
	@if [ -n "$(OPENSCAD)" ]; then \
	    python3 tools/pcb.py --stl; \
	 else \
	    echo "  (sin OpenSCAD: los STL y la prueba de encaje quedan como estaban)"; \
	 fi

# El cupon de prueba: media hora de impresora que contesta las tres
# preguntas del proceso de la cinta que no se pueden contestar en la
# computadora (ver hardware/pcb/cupon.scad). Se exporta y ademas se le corre
# la misma prueba de encaje que a la placa grande: la interseccion de las dos
# piezas tiene que dar vacia, y las nervaduras tienen que llegar al fondo.
cupon:
	@[ -n "$(OPENSCAD)" ] || { echo "  hace falta OpenSCAD"; exit 1; }
	@"$(OPENSCAD)" -D 'pieza="sustrato"' --export-format binstl 	    -o hardware/pcb/generado/cupon-sustrato.stl hardware/pcb/cupon.scad 2> /dev/null
	@"$(OPENSCAD)" -D 'pieza="estampadora"' --export-format binstl 	    -o hardware/pcb/generado/cupon-estampadora.stl hardware/pcb/cupon.scad 2> /dev/null
	@python3 tools/pcb.py --canonizar \
	    hardware/pcb/generado/cupon-sustrato.stl \
	    hardware/pcb/generado/cupon-estampadora.stl > /dev/null
	@echo "  STL: hardware/pcb/generado/cupon-sustrato.stl"
	@echo "  STL: hardware/pcb/generado/cupon-estampadora.stl"
	@rm -f hardware/pcb/generado/cupon-choque.stl hardware/pcb/generado/cupon-presencia.stl
	@"$(OPENSCAD)" -D 'pieza="choque"' --export-format binstl 	    -o hardware/pcb/generado/cupon-choque.stl hardware/pcb/cupon.scad 2> /dev/null || true
	@"$(OPENSCAD)" -D 'pieza="presencia"' --export-format binstl 	    -o hardware/pcb/generado/cupon-presencia.stl hardware/pcb/cupon.scad 2> /dev/null || true
	@if [ -s hardware/pcb/generado/cupon-choque.stl ]; then 	    echo "  FALLA: la estampadora del cupon choca con el cupon"; exit 1; 	 fi
	@if [ ! -s hardware/pcb/generado/cupon-presencia.stl ]; then 	    echo "  FALLA: las nervaduras del cupon no llegan al fondo"; exit 1; 	 fi
	@rm -f hardware/pcb/generado/cupon-choque.stl hardware/pcb/generado/cupon-presencia.stl
	@echo "  encaje: entra sin tocar y llega al fondo de las canaletas"

# Rutear es otra cosa que generar: el ruteo se guarda commiteado en
# generado/ruteo.json y no se rehace en cada build. Se vuelve a correr a mano
# cuando se mueve un módulo, se cambia una red o se toca una regla, y `make
# pcb` lo revisa entero con la geometría de siempre: un ruteo viejo no pasa.
rutear:
	@python3 tools/pcb.py --rutear
	@$(MAKE) --no-print-directory pcb

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
