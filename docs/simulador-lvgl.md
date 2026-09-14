# Simulador de escritorio

El objetivo es escribir y animar toda la interfaz de la Terminal en la PC, y
recién flashear cuando llegue la Guition. Dos etapas: primero el núcleo, que
no necesita nada, y después LVGL con SDL.

## Etapa 1 — el núcleo, hoy mismo

`firmware/core/` es C99 puro: sin LVGL, sin ESP-IDF, sin punto flotante.
Ahí vive la lógica que decide qué cara pone el simbionte, y es lo único que
conviene tener bien resuelto antes de dibujar un solo pixel.

El entorno ya está instalado y verificado:

| Componente | Versión |
|---|---|
| WSL2 + Ubuntu | 24.04 LTS |
| gcc | 13.3.0 |
| GNU Make | 4.3 |
| CMake | 3.28.3 |
| SDL2 | 2.30.0 |
| WSLg | montado, driver x11, `DISPLAY=:0` |

Para correr los tests:

```bash
cd /mnt/c/Users/ifbar/Documents/rootkit/firmware
make test
```

> **La imagen de Ubuntu viene con el índice de apt desactualizado.** Si instalás
> algo y apt tira `404 Not Found` sobre paquetes de glibc, es eso: corré
> `sudo apt update` primero. Le pasa a `build-essential` en una imagen recién
> creada, porque el índice apunta a versiones que ya salieron del pool.

Los tests cubren los casos que importan: prioridad del agua sobre todo lo
demás, ciclo día/noche, histéresis en los bordes y detección de Spore caído.
Si tocás los umbrales de `core/mood.c`, corré `make test` antes de seguir.
Hoy son 19 comprobaciones y compila sin un solo warning.

## Etapa 2 — LVGL con SDL

Las dependencias (`cmake`, `libsdl2-dev`, `git`, `pkg-config`) ya están
instaladas. Falta clonar el port:

```bash
cd ~
git clone --recursive https://github.com/lvgl/lv_port_pc_vscode.git
cd lv_port_pc_vscode
```

WSLg muestra la ventana SDL directamente en el escritorio de Windows, sin
servidor X. **Ya lo probamos**: un programa SDL de 320×480 abrió ventana,
dibujó y cerró correctamente usando el driver x11.

Ajustes que hay que hacer en `lv_conf.h` para que el simulador se parezca a la
Terminal real:

| Parámetro | Valor | Por qué |
|---|---|---|
| Resolución | **320 × 480** | Igual que la Guition, en vertical |
| `LV_COLOR_DEPTH` | **16** | RGB565, como el panel |
| `LV_COLOR_16_SWAP` | probar ambos | El AXS15231B puede pedir bytes invertidos |
| Modo de refresco | **full** | El panel no soporta render parcial |

Diseñá el arte en un lienzo lógico de **160 × 240** y escalalo 2× con enteros.
La resolución nativa de la pantalla es exactamente el doble, así que los
pixeles quedan nítidos sin interpolación, y 160 × 240 está muy cerca de los
256 × 224 de la SNES.

Para las animaciones del simbionte usá **`lv_animimg`**: es un widget pensado
para ciclar una secuencia de imágenes, o sea exactamente un sprite animado.
Con 8 MB de PSRAM entran holgadamente todos los cuadros en memoria.

## Lo que NO se puede probar en el simulador

Anotarlo ahora evita sorpresas cuando llegue la placa:

- **Framerate real.** El simulador corre en una PC; el S3 con full-refresh de
  307.200 bytes por cuadro es otra cosa. Medir el día uno.
- **Táctil.** El mouse no reproduce la latencia ni la precisión del panel
  capacitivo.
- **Colores.** El IPS de la Guition no tiene el mismo gamma que tu monitor.
  El pixel art con paletas saturadas es especialmente sensible a esto.
- **Bytes invertidos.** Si al flashear los colores salen raros, es
  `LV_COLOR_16_SWAP`.
