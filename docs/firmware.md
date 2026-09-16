# Firmware

Cómo está armado, cómo se compila y se flashea, y qué hace el aparato desde
que se enciende.

## Dos mitades

```
firmware/
  core/  gfx/  art/  ui/  nodo/  net/  third_party/     C99 portable
  esp32/                                                Arduino, sólo hardware
  sim/  test/  wasm/                                    escritorio y navegador
```

**El núcleo portable decide todo**: qué pantalla mostrar, qué cara poner,
cuándo medir, cuándo hablar con la nube, qué hacer con la respuesta. No
conoce ni un pin. Se compila igual en la placa, en las pruebas del escritorio
(`make test`), en el simulador (`make sim`) y en el navegador (`make wasm`).

**`esp32/` sólo traduce**: lee bytes de los sensores, se los pasa al núcleo y
hace lo que el núcleo pide. Si algo en `esp32/` toma una decisión, está en el
lugar equivocado.

| Carpeta | Qué hay |
|---|---|
| `core/` | ánimo, especies, personajes, vínculo, **enlace** (la máquina de estados del flujo), **código** de vinculación, SHA-256 |
| `gfx/` | framebuffer RGB565, **antialiasing en punto fijo**, tipografía |
| `art/` | la tabla de expresiones y el rig de caras |
| `ui/` | la **cara**, la cara **dormida**, el **despertar** y la pantalla del **QR** |
| `nodo/` | **sensores** (bytes → unidades), calibración de suelo, batería, muestreo adaptativo, **historial** |
| `net/` | **JSON** y el contrato con la **nube** |
| `esp32/` | `main.cpp`, pantalla, sensores, almacenamiento, portal, red, energía |
| `wasm/` | la entrada para compilar el núcleo a WebAssembly |
| `third_party/` | qrcodegen de Nayuki (MIT) |

## Compilar y flashear

Hace falta [PlatformIO](https://platformio.org/) (`pip install platformio`).

```bash
cd firmware
pio run -e c3-22                     # C3 SuperMini + TFT 2,2"
pio run -e c3-22 -t upload           # flashear
pio device monitor                   # log por USB
```

| Entorno | Placa | Pantalla |
|---|---|---|
| `c3-22` | ESP32-C3 SuperMini | TFT 2,2" 240×320 ILI9341 |
| `c3-144` | ESP32-C3 SuperMini | TFT 1,44" 128×128 ST7735S |
| `devkit-22` | ESP32 DevKit 30 pines | TFT 2,2" |
| `devkit-144` | ESP32 DevKit 30 pines | TFT 1,44" |

**Antes de flashear, apuntar a la nube.** En `platformio.ini`:

```ini
-DRK_NUBE_URL=\"http://192.168.0.20:8080\"   ; la PC con root-lab
-DRK_APP_URL=\"\"                             ; base del QR, si es otra
```

`RK_NUBE_URL` es con quién sincroniza el aparato. `RK_APP_URL` es a dónde
lleva el QR: normalmente la misma, pero para probar instalación y
notificaciones en un teléfono hace falta HTTPS, y ahí va la URL de un túnel
mientras la placa sigue hablando con la PC por la red local. El servidor
también se puede cambiar sin recompilar, desde el portal ("Avanzado").

Si la imagen del panel sale corrida o con colores cambiados:
`-DRK_TFT_OFS_X=2 -DRK_TFT_OFS_Y=1 -DRK_TFT_BGR=1 -DRK_TFT_INVERT=1`.

## Del encendido a la cara

La máquina de estados vive en `core/enlace.c` y está probada paso por paso,
incluidos los caminos feos, en `test/test_enlace.c`.

```
              ┌──────────── botón 10 s (borra todo) ◄────────────┐
              ▼                                                   │
        ┌──────────┐  portal recibió la red   ┌────────────┐     │
        │ SIN_WIFI │ ───────────────────────► │ CONECTANDO │     │
        │  QR +    │ ◄─────── 3 fallos ────── │     QR     │     │
        │  portal  │                          └─────┬──────┘     │
        └──────────┘                                │ wifi ok    │
                                                    ▼            │
                                            ┌──────────────┐     │
                          desvincular ────► │ SIN_VINCULO  │     │
                          (código nuevo)    │  QR, consulta│     │
                                            │  cada 3 s    │     │
                                            └──────┬───────┘     │
                                                   │ la app lo   │
                                                   ▼ reclamó     │
                                            ┌──────────────┐     │
                                            │ ESPERA_COFRE │     │
                                            │ ojos dormidos│     │
                                            └──────┬───────┘     │
                                                   │ cofre       │
                                                   ▼ abierto     │
                                            ┌──────────────┐     │
                                            │ DESPERTANDO  │     │
                                            │   2,6 s      │     │
                                            └──────┬───────┘     │
                                                   ▼             │
                                            ┌──────────────┐     │
                                            │    ACTIVO    │ ────┘
                                            │   la cara    │
                                            └──────────────┘
```

Tres reglas que salen de ahí:

1. **Un aparato que ya tenía cara arranca con la cara**, sin esperar al wifi
   ni a la nube.
2. **Una vez vinculado, nada de la red cambia la pantalla.** Un corte no es
   una desvinculación; sólo la nube, diciendo `vinculado: false`, devuelve el
   QR.
3. **Cada desvinculación sube la época**, y el código del QR sale de la
   época: el QR viejo deja de valer en ese momento.

## Qué pasa en cada vuelta del bucle

`esp32/main.cpp` no bloquea nunca:

1. **Botón.** Tocar prende la pantalla. Mantener: a los 2 s los ojos
   empiezan a cerrarse; a los 10 s se borra el vínculo y el wifi.
2. **Portal.** Si el enlace lo pide, levanta la red `ROOTKIT-XXXX` con DNS
   cautivo y la página para elegir el wifi.
3. **Wifi.** Conecta, reintenta, avisa al enlace.
4. **Medir**, cuando toca según el muestreo adaptativo: sensores → telemetría
   → ánimo (si hay especie) → historial en flash.
5. **Nube.** Arma el pedido con hasta 20 lecturas pendientes y lo manda en
   una tarea aparte, así la cara no se congela durante el TLS. Aplica la
   respuesta: vínculo, cofre, persona, nombre, especie, calibración, brillo,
   modo de pantalla, días sanos. Descarta las lecturas confirmadas.
6. **Enlace.** Avanza la máquina de estados.
7. **Guardar** en NVS lo que cambió.
8. **Energía.** Enchufado o configurando: pantalla prendida. A batería con
   cara: se apaga a los 20 s y, sin nada pendiente, deep sleep hasta la
   próxima medición o hasta que la toquen.
9. **Dibujar** a 30 fps enchufado o 15 fps a batería, mandando por SPI sólo
   las filas que cambiaron.

## Las caras

`art/face.c` dibuja con formas suavizadas en punto fijo (`gfx/aa.c`): elipses,
cápsulas, semiplanos y triángulos, combinados por intersección. Cada pixel del
borde mezcla color y fondo según cuánto lo cubre la forma, y eso es lo que
separa la ilustración del pixel art. Todo en enteros, porque el C3 no tiene
FPU.

El fondo es la piel del personaje, liso, y los párpados se pintan del mismo
color: un párpado que baja es piel que tapa el ojo. Por eso no hay degradé.

Los personajes son filas de `core/persona.c`: familia de ojos, proporciones
en centésimas del lado de la pantalla, cejas, boca, adornos y paleta. Cambiar
una proporción es editar un número; la artista no necesita tocar código.

```bash
make sheet       # 6 personajes × 11 ánimos
make despertar   # el despertar, por personaje
make pantallas   # QR, dormida, despertar y cara, en los dos paneles
make golden      # después de un cambio visual intencional
```

## Seguridad del prototipo

- El token del aparato es HMAC-SHA256 del secreto de fábrica. Si la placa no
  tiene secreto (desarrollo), genera uno y la nube lo acepta la primera vez
  (confianza al primer uso). En producción, el secreto lo graba la estación
  de fábrica y la nube lo rechaza si no está registrado.
- **Con HTTPS el prototipo no verifica el certificado del servidor**
  (`setInsecure()`): el canal va cifrado pero no autenticado. Definir
  `RK_NUBE_CA` con la raíz del proveedor antes de producción. Está en la
  Fase 3 del [roadmap](roadmap.md).
- La clave del wifi se guarda en NVS sin cifrar. El cifrado de flash del
  ESP32 es tarea de la PCB de producción.
