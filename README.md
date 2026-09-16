# ROOTKIT

Una maceta con sensores y una pantalla que muestra **dos cosas: un QR al
principio y unos ojos después**. El personaje es la **carcasa impresa en 3D**;
la cara le hace juego y reacciona a lo que necesita la planta. Todo lo demás
—números, tareas, avisos, colección— vive en la app del teléfono.

![Los seis personajes en los once ánimos](tools/preview/sheet.png)

Del primer encendido a la cara: el QR, los ojos dormidos mientras esperás el
cofre, el despertar y la cara.

![Las pantallas del ROOTKIT en los dos tamaños](tools/preview/pantallas.png)

Cuando abrís el cofre en la app, la maceta abre los ojos:

![El despertar de cada personaje](tools/preview/despertar.png)

## Los repositorios

| | Qué hay |
|---|---|
| **rootkit** (este) | Firmware, hardware, carcasas y la documentación del aparato |
| [**root-lab**](https://github.com/ifbotech/root-lab) | La app, la nube, la IA, las notificaciones y un emulador del aparato |

## El hardware

| | ROOTKIT | ROOTKIT mini |
|---|---|---|
| Placa | ESP32-C3 SuperMini | ESP32-C3 SuperMini |
| Pantalla | TFT 2,2" 240×320 | TFT 1,44" 128×128 IPS |
| Sensores | suelo capacitivo, AHT20, BH1750, DS18B20, toque | suelo, AHT20, BH1750, toque |
| Energía | 18650 + USB-C | LiPo 1000 mAh + USB-C |

Por qué el C3 y no el ESP32 de 30 pines, la lista completa de sensores, el
circuito de carga y las conexiones: [docs/hardware.md](docs/hardware.md).

## Empezar

```bash
make test         # 1085 comprobaciones del firmware, sin placa
make sim          # los seis personajes en una ventana, en vivo
make sheet        # 6 personajes × 11 ánimos
make pantallas    # QR, dormida, despertar y cara en los dos paneles
make wasm         # el renderer para la app (necesita clang y lld)
make placa        # compila las cuatro variantes con PlatformIO
```

Flashear una placa:

```bash
cd firmware
pio run -e c3-22 -t upload && pio device monitor
```

Para ver el flujo completo sin placa, abrí el emulador en
**https://ifbotech.com/rootkit/emulador/** (o levantá **root-lab** en la
compu): corre este mismo firmware en el navegador y su QR abre la app en el
teléfono. Por defecto la placa también sincroniza con ese servidor.

En Windows el firmware se compila y prueba dentro de WSL. Ver
[docs/entorno.md](docs/entorno.md).

## Estructura

```
firmware/
  core/        ánimo, especies, personajes, vínculo, enlace, código, SHA-256
  gfx/         framebuffer, antialiasing en punto fijo, tipografía
  art/         expresiones y el rig de caras
  ui/          cara, cara dormida, despertar y QR
  nodo/        sensores, suelo, batería, muestreo adaptativo, historial
  net/         JSON y el contrato con la nube
  esp32/       la placa: pantalla, sensores, portal, red, energía
  wasm/        el núcleo compilado para el navegador
  sim/ test/   simulador y pruebas
  third_party/ qrcodegen (MIT)
docs/          arquitectura, hardware, firmware, nube, roadmap, decisiones
tools/         capturas y conversión de imágenes
```

## Documentación

| | |
|---|---|
| [roadmap.md](docs/roadmap.md) | **Checklist y roadmap**: qué está hecho y qué falta, por fase |
| [arquitectura.md](docs/arquitectura.md) | Las tres piezas, el flujo y quién decide qué |
| [hardware.md](docs/hardware.md) | Placa, pantallas, sensores, batería, carga y conexiones |
| [firmware.md](docs/firmware.md) | Cómo está armado, cómo compilar y qué hace al encender |
| [nube.md](docs/nube.md) | El contrato con la nube, con vectores de prueba |
| [carcasas.md](docs/carcasas.md) | Diseño e impresión de las carcasas |
| [decisiones.md](docs/decisiones.md) | Lo que se decidió, lo que se revisó y por qué |
| [testing.md](docs/testing.md) | Qué cubren las pruebas |
| [entorno.md](docs/entorno.md) | Herramientas y simulador |
