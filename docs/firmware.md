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
-DRK_NUBE_URL=\"https://ifbotech.com/rootkit\"   ; la versión de prueba en el VPS (por defecto)
-DRK_NUBE_URL=\"http://192.168.0.20:8080\"       ; o la PC con root-lab
-DRK_APP_URL=\"\"                                 ; base del QR, si es otra
```

`RK_NUBE_URL` es con quién sincroniza el aparato. `RK_APP_URL` es a dónde
lleva el QR: normalmente la misma. Por defecto apunta al VPS
(`https://ifbotech.com/rootkit`): el QR sale como
`HTTPS://IFBOTECH.COM/ROOTKIT/V/<código>` (en mayúsculas, para el modo
alfanumérico del QR) y abre una app con HTTPS, que se puede instalar y
recibe notificaciones. Con root-lab en la PC la app abre por HTTP y no se
instala; para eso, `RK_APP_URL` con la URL de un túnel HTTPS mientras la
placa sigue hablando con la PC por la red local. El servidor también se
puede cambiar sin recompilar, desde el portal ("Avanzado").

La placa verifica el certificado del servidor contra raíces fijadas: ver
"Seguridad" más abajo.

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

Los Rooties son filas de `core/persona.c`: familia de ojos, proporciones en
centésimas del lado de la pantalla, cejas, boca, adornos, **accesorio** y
paleta. Cambiar una proporción es editar un número; la artista no necesita
tocar código.

Los **accesorios** son parte fija del personaje, con cualquier ánimo: hoy
`RK_ACC_LENTES` (Chica Chill: anteojos redondos más grandes que el ojo, para no
taparle los párpados, con un reflejo en cada vidrio) y `RK_ACC_CURITA` (Chico
Malo: una curita cruzada en el cachete). Se dibujan después de ojos, cejas y
boca y antes de los adornos.

Los colores de Chico Malo y Chica Chill salen de sus paletas, las mismas que
pintan ROOTLAB cuando salen del cofre (`root-lab/docs/paletas.md`).

```bash
make sheet       # 8 Rooties × 11 ánimos
make despertar   # el despertar, por personaje
make pantallas   # QR, dormida, despertar y cara, en los dos paneles
make golden      # después de un cambio visual intencional
```

## Seguridad

- El token del aparato es HMAC-SHA256 del secreto de fábrica. Si la placa no
  tiene secreto (desarrollo), genera uno y la nube lo acepta la primera vez
  (confianza al primer uso). En producción, el secreto lo graba la estación
  de fábrica y la nube lo rechaza si no está registrado.
- **La nube por HTTPS, verificando el certificado.** La placa confía sólo en
  las raíces de las dos autoridades que usa el Caddy del servidor:
  **Let's Encrypt** (ISRG Root X1 y X2, y las de la generación siguiente,
  Root YE y YR) y **ZeroSSL** (USERTrust ECC y RSA), en
  `esp32/certificados.h` con sus huellas SHA-256. Un certificado de cualquier
  otra autoridad no alcanza para hacerse pasar por la nube. mbedTLS en el
  ESP32 no valida fechas (`MBEDTLS_HAVE_TIME_DATE` apagado), así que no hace
  falta hora antes de conectarse. Para un servidor con CA propia,
  `-DRK_NUBE_CA=...`; sólo en desarrollo, contra un túnel,
  `-DRK_NUBE_INSEGURO=1` vuelve a `setInsecure()`.
- La clave del wifi se guarda en NVS sin cifrar. El cifrado de flash del
  ESP32 es tarea de la PCB de producción.
