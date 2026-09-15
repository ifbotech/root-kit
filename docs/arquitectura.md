# Arquitectura

Un ROOTKIT es **un Prime y hasta cinco Minis**. Cada uno es una maceta con
sensores y pantalla; el Prime además va enchufado, tiene la pantalla grande y
sirve el Hub.

```
   ┌───────────────┐   telemetría 26 B   ┌───────────────┐   HTTP/JSON  ┌────────┐
   │     MINI      │ ──────────────────▶ │     PRIME     │ ◀──────────▶ │  HUB   │
   │  ESP32-C3     │                     │  ESP32-C3     │  red local   │  PWA   │
   │  TFT 1,44"    │ ◀────────────────── │  TFT 2,2"     │              │ celu   │
   │  128x128      │   config 32 B       │  240x320      │              └────────┘
   │  18650        │                     │  enchufado    │
   └───────────────┘                     └───────────────┘
    mide, evalúa                          mide, evalúa                registra
    dibuja su BROTE                       dibuja el ADULTO
    13 mm                                 22 mm
                                          junta el kit
                                          abre las cápsulas
                                          sirve el Hub
                                                │
                                                ▼ sólo al registrar una planta
                                          API de visión
```

## La idea que ordena todo: dos escalas del mismo organismo

El mismo simbionte se ve **adulto en el Prime y brote en el Mini**. No son dos
criaturas: son la misma, mirada a dos distancias. Cuando seleccionás un Mini en
el Prime, el brote de 13 mm que vive en esa maceta aparece grande, a 22 mm, con
su nombre, su rareza y su vínculo.

Eso le da al Prime una razón de producto que no es "una pantalla más grande":
**el Prime es donde tus criaturas están grandes.** Y le da a cada Mini una razón
que no es "un sensor más": es otra maceta donde el organismo se propagó.

Técnicamente el brote no es el adulto reducido. Invierte sus proporciones:
donde el adulto es caparazón con una cabeza asomando, el brote es cabeza con un
caparazón asomando. Cabeza grande, ojos grandes y bajos, cuerpo chico. Esa
inversión es la gramática visual de "cría", y es lo que hace que 32×32 se lean
como la cría del mismo bicho y no como otro bicho más chico.

## El cambio de regla que trajo esta arquitectura

La versión anterior tenía un Spore **deliberadamente tonto**: medía, empaquetaba
y dormía, y toda la interpretación vivía en la Terminal. Tres razones lo
justificaban —batería, actualizaciones centralizadas, una sola implementación de
la máquina de estados.

**Desde que cada nodo tiene pantalla, esa regla ya no se sostiene.** Un nodo
que necesita la red para saber qué cara poner se queda mudo justo cuando más
importa: cuando el Prime está apagado, reiniciándose o fuera de alcance. Así
que ahora:

> **Cada nodo evalúa su propia maceta.** Corre el mismo `core/mood.c`, con los
> umbrales de su especie que le llegaron en la trama de configuración, y manda
> el ánimo **ya resuelto**.

Las tres razones originales siguen respetadas, y así es como:

1. **Batería.** Evaluar el ánimo son unas pocas comparaciones enteras sobre
   datos que el nodo ya tiene en RAM. No agrega un solo despertar. Lo que
   consume es la pantalla, y eso se paga igual la decida quien la decida.
2. **Actualizaciones.** Los umbrales no están compilados en el Mini: viajan en
   `CONFIG`. Corregir una especie sigue siendo tocar un solo lugar, y el Prime
   reenvía la configuración.
3. **Una sola implementación.** `core/mood.c` sigue existiendo una única vez.
   No es duplicación: es el mismo archivo compilado dos veces.

Y el Prime **no recalcula** lo que recibe: copia el ánimo del paquete. Si lo
recalculara, su histéresis acumulada y su conteo de muestras oscuras serían
distintos de los del Mini, y el usuario vería una cara en la maceta y otra en el
escritorio sin forma de saber cuál le miente. Hay un test en la suite "kit y
enlace" que manda una telemetría cuyos números gritarían `THIRSTY` y cuyo campo
de ánimo dice `HAPPY`, y verifica que el Prime respeta el ánimo.

## Por qué el Prime va enchufado y los Minis no

Con alimentación fija la pantalla del Prime puede quedar siempre encendida —un
simbionte que hay que despertar tocando es peor mascota que uno que está siempre
vivo. La batería queda sólo donde hace falta, en las macetas lejos del enchufe,
y ahí la pantalla se enciende por movimiento o por toque.

Ese reparto es lo que hace viable el producto entero. Ninguna pantalla sobrevive
siempre encendida a batería: una IPS a color da entre 0,9 y 3 días. Con la
pantalla apagada salvo cuando alguien mira, el Mini vive casi dos años.

## El Hub es una vista, no una aplicación

La foto obliga a tener el celular, pero el celular no obliga a tener una app
nativa: una página web abre la cámara con `<input capture>`. Sin App Store no
hay cuota anual, ni revisión de actualizaciones, ni —sobre todo— nadie revisando
las mecánicas de colección contra las políticas de cajas de botín.

El Prime sirve la PWA por HTTP en la red local. No puede servirse desde
internet: una página HTTPS tiene prohibido pedirle datos a una IP privada, y no
hay forma limpia de esquivarlo.

## Capas del firmware

```
firmware/
  core/    C99 puro. Sin ESP-IDF, sin punto flotante.
           telemetría, especies, ánimos, colección y el roster del kit.
  net/     Protocolo binario y el pegamento con el roster. Sin dependencias.
  nodo/    Suelo, batería y muestreo adaptativo. Sin dependencias.
  gfx/     Framebuffer RGB565, tipografía y los dos paneles como datos.
  art/     Sprites generados, la tabla de carácter y los dos rigs.
  ui/      Composición de cada pantalla. Funciones puras de (estado, tiempo).
  sim/     Host SDL de escritorio. La única capa que sabe de un sistema
           operativo, y la única que no va al dispositivo.
```

La dirección de las dependencias es hacia adentro: `ui` usa `gfx` y `core`,
nunca al revés. Nada de `core`, `net` ni `nodo` incluye una cabecera de sistema
más allá de `stdint`, `stdbool`, `stddef` y `string`.

**Una excepción declarada:** `ui/prime.c` y `ui/mini.c` incluyen `nodo/power.h`
para dibujar la pila. Podrían reimplementar la curva de descarga, pero tener una
sola curva importa más que un diagrama de capas prolijo.

### Dónde vive cada cosa, y por qué ahí

| Pieza | Archivo | Por qué |
|---|---|---|
| Qué cara poner en cada ánimo | `art/look.c` | Compartido entre los dos rigs. Si viviera duplicado, el Prime y el Mini dirían cosas distintas de la misma planta. |
| El arte del adulto | `art/sprites.c` + `art/adulto.c` | Una silueta, doce paletas. |
| El arte del brote | `art/sprites.c` + `art/brote.c` | Doce cuerpos de 32×32: mismo esqueleto, copete y punteado propios. |
| Los tamaños de los paneles | `gfx/panel.c` | Ninguna otra parte del código tiene constantes de pantalla escritas a mano. |
| El kit | `core/node.c` | El Prime dibuja esto; el Hub sirve esto. |
| Ida y vuelta por radio | `net/link.c` | La capa que el ESP32 llama desde sus callbacks. |

## Decisiones que conviene no revisitar sin leer esto

### Renderer propio en vez de LVGL

1. El pixel art necesita **escalado por enteros con vecino más cercano**. LVGL
   escala pensando en suavizado, que es justo lo que no queremos.
2. Un buffer RGB565 plano es exactamente lo que espera
   `esp_lcd_panel_draw_bitmap()`.
3. Hay **dos paneles de tamaños muy distintos** y LVGL pesa lo mismo en los
   dos. Acá el Mini paga exactamente las primitivas que usa.

### Resolución nativa, sin lienzo lógico

La versión anterior rasterizaba a 160×240 y escalaba 2× al presentar, porque el
panel de la Terminal era de 320×480 y 300 KB de framebuffer no entraban en la
SRAM interna. Con 240×320 el cuadro son **150 KB** y con 128×128 son **32 KB**:
los dos entran holgados en los 400 KB de un ESP32-C3. Así que se dibuja directo
en resolución nativa y desaparece una capa entera de conversión de coordenadas.
El arte se sigue escalando por enteros, que es lo que de verdad importaba.

**La consecuencia tipográfica, que no es obvia:** al dibujar nativo, un glifo de
5×7 a escala 1 mide 0,76 × 1,06 mm en el Prime. Ilegible. Por eso el texto del
Prime arranca en **escala 2** y los títulos van en 3.

### El bus manda, no la CPU

`make bench` mide las dos cosas. Rasterizar un cuadro del Prime cuesta décimas
de milisegundo en un x86 y quizá unos pocos en el C3. Empujar 153.600 bytes por
SPI a 40 MHz son **29 ms**, y a 80 MHz **15 ms**. El bus es un orden de magnitud
más caro que dibujar.

La consecuencia práctica: seguir optimizando el rasterizado rinde poco. El
margen está en subir el reloj del SPI y en solapar el envío por DMA con el
dibujo del cuadro siguiente. El Mini, con 32 KB por cuadro, llega a 160 fps de
techo: ahí no hay problema.

### Binario en vez de JSON entre los nodos

Una trama de telemetría son 26 bytes. El mismo contenido en JSON son unos 190.
Cada byte es tiempo de radio encendida, y la radio es la mitad del presupuesto
energético de un Mini.

La configuración creció a 32 bytes para llevar los umbrales de la especie. No
pesa: viaja una sola vez al emparejar y cada vez que cambia la especie, en
sentido Prime → Mini, sobre un nodo que ya está despierto.

### Nanoamperios-hora en el modelo de consumo

Una medición cuesta 0,25 µAh. En enteros de microamperios-hora eso se redondea a
cero y el término desaparece del modelo. Con nAh los tres términos conviven sin
punto flotante, que el firmware no quiere.

## Presupuesto energético del Mini

Salido de `make test`, no de una planilla: las cifras se recalculan en cada
corrida y el test falla si alguien cambia un parámetro sin querer.

| Perfil | µAh/día | Autonomía (18650 2200 mAh, −20%) | Durmiendo |
|---|---:|---:|---:|
| Ingenuo — LED puesto, DHCP, transmite siempre | 15.206 | **115 días** | 47% |
| Fijo — optimizado, cada 15 min | 3.672 | **479 días** | 26% |
| Adaptativo — mide seguido, emite poco | 1.900 | **926 días** | 50% |

Estas cifras son **sin pantalla**. La TFT de 1,44" con la retroiluminación al
máximo pide unos 32 mA; encendida dos minutos por día eso son 1.067.000 nAh
diarios sobre los 1.900.000 del perfil adaptativo, y la autonomía cae de 926 a
unos **595 días**. Sigue siendo año y medio, y es el número que hay que
confirmar con un multímetro apenas lleguen las placas.

Con el perfil adaptativo y sin pantalla, dormir y transmitir quedan casi
empatados. Eso significa que el firmware ya dio lo que tenía: **el próximo
microamperio hay que ir a buscarlo al hardware**, desoldando el LED de
alimentación y eligiendo un LDO de bajo reposo.

## Lo que falta para el primer prototipo

- Capa HAL del ESP32: drivers de ADC, I2C, SPI y Wi-Fi bajo las interfaces que
  el núcleo ya define.
- Transporte real entre nodos (ESP-NOW) detrás de `net/link.h`.
- Servidor HTTP en el Prime que implemente `hub/API.md`.
- Persistencia en NVS: calibración, nodos del kit, vínculos y colección.
- Carcasas.
