# Arquitectura

Un ROOTKIT es **un aparato en una maceta**: una placa, sus sensores y una
carcasa impresa en 3D que le da la cara. Se compra en caja ciega, como un
Smiski: cinco modelos a la vista y un secreto.

```
   ┌─────────────────────┐   telemetría 26 B   ┌──────────┐
   │      ROOTKIT        │ ──────────────────▶ │   APP    │
   │  ESP32-C3           │                     │   PWA    │
   │  TFT 1,44" 128x128  │ ◀────────────────── │  celular │
   │  18650              │   config 32 B       └──────────┘
   │  + carcasa impresa  │                      el tablero
   └─────────────────────┘                      el historial
    mide                                        la colección
    evalúa su propia maceta                          │
    dibuja UNA CARA                                  ▼ al registrar
                                               API de visión
```

## La idea que ordena todo: la variedad es física, la cara es digital

Lo único que se diseña en pixeles es **la cara**. Lo que cambia entre un
modelo y otro es la **carcasa impresa**: su cresta, su pelo, su visera. La
carcasa es el personaje; la cara le hace juego.

Es mejor reparto de esfuerzo que el anterior. Imprimir una carcasa nueva
cuesta filamento y unas horas de modelado; dibujar y animar un cuerpo nuevo
cuesta semanas. Y una cara sola en 128×128 tiene muchos más pixeles por rasgo
que un cuerpo entero, así que se expresa mejor, no peor.

### El ánimo dice qué siente; la persona dice cómo lo muestra

```
  core/mood.c     estado de la planta   ->  QUÉ siente
  art/look.c      expresión semántica   ->  ojo entrecerrado, boca abajo
  core/persona.c  carácter de la carcasa->  CÓMO se dibuja eso
  art/face.c      los cruza y dibuja
```

| | Cresta | Kawaii | Visor | Ciclope | Hongo |
|---|---|---|---|---|---|
| **sed** | ceño apretado, dientes | ojos llorosos, lágrima | la onda se quiebra en picos | la pupila se contrae | los párpados caen más |

Seis modelos por once ánimos son **sesenta y seis caras**, y salen todas del
mismo código porque la cara es procedural y no sprites. Agregar un modelo es
agregar una fila a `rk_persona_table`; agregar un ánimo es agregar una fila a
`LOOKS`. Ninguna de las dos tablas conoce a la otra.

### Por qué procedural y no sprites

A este tamaño no es una concesión sino una ventaja. Un ojo ocupa unos 35 px de
ancho: suficiente para que una curva calculada se vea deliberada. Y se gana lo
que un flipbook no da —la pupila mira a cualquier lado, el párpado cierra a
cualquier altura, el parpadeo cae cuando tiene que caer.

Dibujar sesenta y seis caras a mano sería un mes de trabajo y un archivo de
datos enorme, y agregar un modelo costaría once caras más.

## La rareza es física

Ya no sale de la dificultad de la planta: sale de qué carcasa te tocó en la
caja. Eso mantiene el producto fuera del terreno regulado de las cajas de
botín por una razón más sólida que antes: **no hay compra aleatoria dentro de
un software, hay un juguete en una caja** — que es exactamente lo que hacen
los Smiski y los Sonny Angel desde hace años.

Lo que se gana cuidando la planta ya no es el personaje sino cómo se ve: los
días sanos desbloquean capas cosméticas sobre la cara (brillos a los 30 días,
aura a los 90, corona a los 180). El modelo te toca por azar; **el aura se
gana y no se compra**.

## El aparato dibuja una cara; la app tiene el tablero

La pantalla del aparato antes mostraba nombre, wifi, batería, una frase, tres
medidores y una tira selectora. Todo eso se mudó a la app.

El aparato está en la maceta, se mira de reojo al pasar y desde un metro:
contesta una sola pregunta —¿cómo está mi planta?— y la contesta con una cara.
Los números los va a buscar alguien que ya decidió preocuparse, y esa persona
tiene el teléfono en la mano.

Quedan dos elementos además de la cara, y los dos aparecen sólo cuando hacen
falta:

- un **pictograma** en la esquina si hay algo que pedir (gota, sol, copo). La
  cara dice que algo anda mal; el pictograma dice qué, sin idioma.
- un **aviso de batería** cuando la celda está por terminarse. Es lo único que
  la app no puede resolver sola, porque para cambiarla hay que ir.

## Cada aparato evalúa su propia maceta

> Corre `core/mood.c` con los umbrales de su especie, que le llegaron en la
> trama de configuración, y muestra el ánimo **sin preguntarle a nadie**.

Un aparato que necesita la red para saber qué cara poner se queda mudo justo
cuando más importa. Y quien recibe la telemetría **no recalcula**: copia el
ánimo del paquete. Si lo recalculara, su histéresis acumulada y su conteo de
muestras oscuras serían distintos, y el usuario vería una cara en la maceta y
otra en el teléfono sin forma de saber cuál le miente. Hay un test en la suite
"kit y enlace" que manda una telemetría cuyos números gritarían `THIRSTY` y
cuyo campo de ánimo dice `HAPPY`, y verifica que se respeta el ánimo.

## Capas del firmware

```
firmware/
  core/    C99 puro. Sin ESP-IDF, sin punto flotante.
           telemetría, especies, ánimos, vínculo, modelos y el kit.
  net/     Protocolo binario y el pegamento con el roster. Sin dependencias.
  nodo/    Suelo, batería y muestreo adaptativo. Sin dependencias.
  gfx/     Framebuffer RGB565, elipses, arcos, trazos y tipografía.
  art/     La tabla de expresiones y el rig procedural de caras.
  ui/      La pantalla y el primer encendido. Funciones puras.
  sim/     Host SDL de escritorio. La única capa que sabe de un sistema
           operativo, y la única que no va al dispositivo.
```

La dirección de las dependencias es hacia adentro: `ui` usa `gfx` y `core`,
nunca al revés. Nada de `core`, `net` ni `nodo` incluye una cabecera de sistema
más allá de `stdint`, `stdbool`, `stddef` y `string`.

**Una excepción declarada:** `ui/cara.c` incluye `nodo/power.h` para dibujar el
aviso de batería. Podría reimplementar la curva de descarga, pero tener una
sola curva importa más que un diagrama de capas prolijo.

### Dónde vive cada cosa, y por qué ahí

| Pieza | Archivo | Por qué |
|---|---|---|
| Qué expresión pide cada ánimo | `art/look.c` | La mitad semántica: independiente del modelo. |
| El carácter de cada carcasa | `core/persona.c` | Una fila por modelo. Es donde arte ajusta números sin tocar lógica. |
| El rig que los cruza | `art/face.c` | Seis familias de ojos, cuatro de cejas, cinco de bocas. |
| Los tamaños del panel | `gfx/panel.c` | Ninguna otra parte tiene constantes de pantalla a mano. |
| El vínculo y las etapas | `core/vinculo.c` | Sobrevivió intacto al cambio de producto. |
| Ida y vuelta por radio | `net/link.c` | La capa que el ESP32 llama desde sus callbacks. |

## Decisiones que conviene no revisitar sin leer esto

### Renderer propio en vez de LVGL

La cara es procedural: elipses, arcos y trazos calculados. LVGL trae un motor
de widgets que acá no se usaría nunca, y un buffer RGB565 plano es exactamente
lo que espera `esp_lcd_panel_draw_bitmap()`. El renderer propio son unas 400
líneas en `gfx/`, y el mismo código corre en el simulador, en los tests y en la
placa.

### Resolución nativa, sin lienzo lógico

128×128 son 32 KB: entran holgados en los 400 KB de SRAM de un ESP32-C3, con
lugar de sobra para un segundo buffer y mandar por DMA mientras se dibuja el
siguiente. Se dibuja directo y desaparece una capa de conversión de
coordenadas.

Los rasgos se miden en **centésimas del ancho del panel**, no en pixeles, así
que la misma tabla de modelos sirve para cualquier panel futuro sin tocar un
número.

### El bus manda, no la CPU

`make bench` mide las dos cosas. Rasterizar una cara cuesta centésimas de
milisegundo en un x86 y quizá unas décimas en el C3. Empujar 32.768 bytes por
SPI a 40 MHz son **6,25 ms**, o sea 160 fps de techo. Sobra: el margen está en
bajar el reloj para ahorrar, no en subirlo.

### Binario en vez de JSON

Una trama de telemetría son 26 bytes; el mismo contenido en JSON son unos 190.
Cada byte es tiempo de radio encendida, y la radio es cerca de la mitad del
presupuesto energético.

La configuración son 32 bytes y lleva los umbrales de la especie más el índice
de la carcasa. No pesa: viaja una sola vez al emparejar y cada vez que cambia
la especie o el modelo.

## Presupuesto energético

Salido de `make test`, no de una planilla: las cifras se recalculan en cada
corrida y el test falla si alguien cambia un parámetro sin querer.

| Perfil | µAh/día | Autonomía (18650 2200 mAh, −20%) | Durmiendo |
|---|---:|---:|---:|
| Ingenuo — LED puesto, DHCP, transmite siempre | 15.206 | **115 días** | 47% |
| Fijo — optimizado, cada 15 min | 3.672 | **479 días** | 26% |
| Adaptativo — mide seguido, emite poco | 1.900 | **926 días** | 50% |

Estas cifras son **sin pantalla**. La TFT de 1,44" con la retroiluminación al
máximo pide unos 32 mA; encendida dos minutos por día eso son 1.067.000 nAh
diarios sobre los 1.900.000 del perfil adaptativo, y la autonomía cae a unos
**595 días**. Sigue siendo año y medio, y es el número que hay que confirmar
con un multímetro apenas lleguen las placas.

## Lo que falta para el primer prototipo

- Capa HAL del ESP32: drivers de ADC, I2C, SPI y Wi-Fi bajo las interfaces que
  el núcleo ya define.
- Transporte real detrás de `net/link.h`.
- Servidor HTTP que implemente `hub/API.md`.
- Persistencia en NVS: calibración, planta, carcasa, vínculo y colección.
- **Las seis carcasas.** Ver [carcasas.md](carcasas.md), que tiene el
  envolvente, las tolerancias y el brief de cada modelo.
