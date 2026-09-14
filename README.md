# ROOTKIT

Ecosistema cyber-botánico: sensores en las macetas, un simbionte pixel-art en
el escritorio que reacciona en tiempo real a lo que miden.

## Las tres piezas

| Pieza | Qué es | Hardware |
|---|---|---|
| **Terminal** | El objeto de escritorio. Muestra al simbionte a color y es el servidor local del sistema. | Guition JC3248W535 — ESP32-S3, 3,5" 320×480 IPS, táctil capacitivo. Enchufada, sin batería. |
| **Spore** | Un nodo por maceta. Mide y reporta. | ESP32-C3 + capacitivo v2.0 + AHT21 + BH1750 + 18650. Deep sleep, meses de autonomía. |
| **Hub** | PWA servida por la Terminal. Registrar plantas con la cámara del celular, ver la colección. | Ninguno. |

## Estructura

```
firmware/
  core/          C99 puro: la lógica del simbionte. Sin LVGL, sin ESP-IDF.
    telemetry.h  Lo que un Spore reporta.
    species.*    Rangos de confort por especie.
    mood.*       Telemetría -> estado de ánimo. El corazón del producto.
  test/          Tests del núcleo. Corren sin hardware ni SDL.
  Makefile
docs/
  decisiones.md      Qué se decidió y por qué.
  simulador-lvgl.md  Cómo levantar el entorno de desarrollo.
hub/               (vacío todavía)
```

## Empezar

```bash
cd firmware
make test
```

Necesita un compilador de C. En Windows, dentro de WSL — ver
[docs/simulador-lvgl.md](docs/simulador-lvgl.md).

## Estado

- [x] Núcleo: telemetría, especies, máquina de estados de ánimo con tests
- [x] Compila limpio con `-Wall -Wextra -Wpedantic`; 19/19 tests en verde
- [x] Entorno de desarrollo: WSL2 + Ubuntu 24.04 + gcc 13.3 + SDL 2.30 sobre WSLg
- [ ] Simulador LVGL con SDL
- [ ] Sprites y animación de `Tuga.exe`
- [ ] Hub: registro de plantas con foto e identificación por IA
- [ ] Firmware del Spore
- [ ] Protocolo Spore ↔ Terminal
- [ ] Carcasas

## Por qué el núcleo no depende de nada

`firmware/core/` se compila idéntico en la PC, en los tests y en el ESP32.
Eso permite desarrollar y probar toda la lógica del simbionte antes de tener
hardware, y garantiza que lo que se validó en el simulador es exactamente lo
que corre en la Terminal. Todo lo que dependa de LVGL, de Wi-Fi o de un sensor
va afuera de `core/`.
