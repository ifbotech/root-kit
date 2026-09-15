# Pruebas

```bash
make test        # todo: 752 comprobaciones de firmware + 53 del Hub
make firmware    # sólo el firmware (no necesita SDL ni hardware)
make hub         # sólo el Hub (necesita Node)
make verify      # lo que corre CI, incluida la frescura del arte
```

El firmware se compila en WSL y el Hub en Node sobre Windows. `make test`
funciona desde los dos lados: si falta Node, el target del Hub avisa en vez de
romper.

## Qué cubre cada suite

| Suite | Comprobaciones | Qué protege |
|---|---:|---|
| `animo` | 28 | Prioridad entre necesidades, ciclo día/noche, histéresis, nodo caído |
| `protocolo` | 60 | Ida y vuelta de las tramas, CRC, códec de luz, rechazo de corrupción |
| `nodo` | 63 | Calibración de suelo, fallas eléctricas, curva de batería, muestreo |
| `graficos` | 39 | Recorte de primitivas, sprites, tipografía, seno y hash |
| `render prime` | 88 | Determinismo, zona táctil, tamaño físico, regresión visual |
| `coleccion` | 57 | Rarezas, asignación determinista, crecimiento, ceremonia |
| `mini y brote` | 321 | Tablas de arte, paletas, etapas, frases que entran, regresión visual |
| `kit y enlace` | 96 | Roster, salud del enlace, configuración con umbrales, vuelta completa |
| `hub` | 53 | Formato, orden, validación, vínculo, contrato de la API |

## Las pruebas que valen más que su tamaño

**Que el Prime no recalcula el ánimo.** Es la más importante de todas, porque
protege una regla de arquitectura y no una función. Se manda una telemetría
cuyos números gritarían `THIRSTY` y cuyo campo de ánimo dice `HAPPY`, y se
verifica que el Prime respeta el ánimo. Si algún día alguien agrega un
`rk_mood_eval` "por las dudas" en el camino de recepción, este test lo atrapa.
Sin él, la maceta y el escritorio pueden mostrar caras distintas de la misma
planta y no hay forma de saber cuál miente.

**La vuelta completa del kit.** El Prime arma la configuración desde la especie,
la codifica, el Mini la decodifica, reconstruye su especie, mide, evalúa con
esos umbrales, emite, y el Prime aplica el resultado al nodo correcto. Es un
solo test que atraviesa `core`, `net` y `nodo`, y es el que garantiza que el
sistema funcione sin un Prime mirando.

**Que los once ánimos se vean distintos, en los dos paneles.** Se renderizan las
once pantallas completas y se comparan por hash, todas contra todas. Si dos
coinciden, el producto no comunica nada: el usuario mira y no sabe si la planta
tiene sed o frío. Este test ya encontró, en su versión anterior, que THIRSTY,
COLD y HOT compartían dibujo, y después que DROWNING y DARK se parecían
demasiado.

**Que las cinco etapas se vean distintas.** Mismo método, aplicado al
crecimiento. Si dos etapas producen el mismo cuadro, el crecimiento no existe
para el usuario por más que el contador avance en NVS. Es lo que obligó a
mostrar el crecimiento con hojas alrededor en vez de con el tamaño del cuerpo:
a 32×32 un cuerpo 15% más grande da el mismo hash redondeado.

**Que los doce simbiontes se vean distintos.** Comparten silueta a propósito, así
que la única garantía de que la colección signifique algo es que sus paletas y
sus copetes los separen. Se comparan las doce paletas entrada por entrada y los
doce brotes renderizados.

**El tamaño físico de la criatura.** `rk_panel_decimas_mm` convierte pixeles a
milímetros con el paso real de cada panel, y hay tests que fijan el adulto en
21,9 mm y el brote en 12,9 mm, más uno que exige que el adulto mida al menos
1,6 veces el brote. Esos números son los que decidieron qué pantallas comprar:
si alguien cambia una escala de arte, se entera acá y no cuando le llegan cien
unidades.

**Que las frases entren en el renglón.** Cada frase del catálogo se mide contra
el ancho de los dos paneles. Una frase cortada deja al bicho balbuceando, y es
un bug que sólo se ve en la placa.

**Los flips de un bit.** `test_proto.c` da vuelta cada bit de una trama de
telemetría, uno por vez, y verifica que el CRC los detecte todos. Es la garantía
que justifica gastar dos bytes en el CRC.

**Los centinelas del framebuffer.** `test_gfx.c` y `test_mini.c` reservan el
buffer con guardas a cada lado y llaman a las primitivas con coordenadas
imposibles. Un blit sin recorte no tira excepción en el ESP32: corrompe lo que
haya al lado, y eso aparece tres días después como un bug imposible. El blit
escalado tiene su propio recorte —distinto del normal, porque trabaja en
coordenadas del sprite— así que tiene su propio centinela.

**Que el índice del simbionte siga alineado con el arte.** El índice en
`rk_companion_table` *es* la clave con la que se elige paleta y cuerpo. Si
alguien reordena la tabla sin regenerar el arte, cada bicho sale con la paleta
del vecino y nada falla ruidosamente. El test compara las dos tablas posición
por posición.

**La monotonía de la batería.** Se recorre la curva de 2,8 a 4,3 V verificando
que nunca baje. Sin eso, el ruido del ADC puede hacer que la batería "suba" en
pantalla.

**La semana simulada.** Siete días de muestreo con una planta que se seca y se
riega, verificando que el muestreo adaptativo ahorre al menos la mitad de las
transmisiones — pero también que no ahorre de más, que sería estar perdiendo
eventos.

**Que un nodo caído no acumule días sanos.** Es lo que impide que desenchufar un
Mini haga crecer al simbionte gratis.

**El desbloqueo determinista, cincuenta veces seguidas.** Es el test que sostiene
la posición legal del producto: el simbionte que sale depende de la especie y de
nada más.

**La escalera de etapas, de los dos lados.** `0, 7, 30, 90, 180` vive en
`firmware/core/companion.c` y en `hub/lib/model.mjs`. Es la única regla duplicada
a propósito del sistema —el Hub necesita dibujar la barra sin un viaje más— y
hay tests en los dos lados que fallan si se separan.

## Regresión visual

`test/golden.h` guarda un FNV-1a del framebuffer de **cada panel** para cada
estado de ánimo, con un kit fijo. Si un cambio altera cualquier pixel, la suite
correspondiente lo marca.

Son dos columnas y no una a propósito. Un cambio en el rig del brote no mueve un
solo pixel del Prime y viceversa, así que un hash global diría "algo cambió" sin
decir dónde — que es justamente la información que hace falta al revisar un
cambio de arte. Si sólo se mueve la columna del Mini, el Prime quedó intacto y no
hay que volver a mirarlo.

Cuando el cambio es intencional:

```bash
make golden      # regenera firmware/test/golden.h
```

El archivo se versiona en vez de ignorarse porque **el diff del archivo generado
es la revisión del cambio visual**.

`make verify` —y CI— regeneran el arte y los hashes y fallan si el árbol queda
sucio: si alguien toca `gen_art.py` y se olvida de regenerar, salta ahí en vez de
descubrirse semanas después con una captura vieja.

## Qué NO cubre

Conviene tenerlo escrito, porque la cobertura alta invita a confiar de más:

- **Nada toca hardware real.** No hay pruebas del ADC, del I2C, del SPI ni de la
  radio. Todo eso sigue siendo hipótesis hasta que lleguen las placas.
- **El consumo es un modelo, no una medición.** Las cifras de autonomía salen de
  correr las funciones de `nodo/power.c` con parámetros que todavía no se
  verificaron con un multímetro. El reposo real del C3 y la corriente de cada
  retroiluminación son los dos números que pueden cambiar todo.
- **La regresión visual compara hashes, no aspecto.** Detecta que algo cambió;
  no dice si quedó mejor o peor. Para eso están `make sheet`, `make minis` y
  `make brotes`, que hay que mirar con los ojos.
- **No hay pruebas de la interfaz del Hub en un navegador.** Se prueba la lógica
  pura y el contrato de la API, no el DOM.
- **El transporte de radio no existe todavía.** `net/link.c` se prueba pasando
  paquetes de una estructura a otra en memoria; ESP-NOW está sin escribir.

## Cómo agregar una suite

1. Un archivo `test/test_<nombre>.c` con una función `suite_<nombre>(void)`.
2. Declararla en `test/rk_test.h` y llamarla en `test/main.c`.
3. Agregarla a `TESTSRC` en `firmware/Makefile`.

Los `CHECK_*` están en `test/rk_test.h`. La etiqueta de cada uno se lee en la
salida cuando falla, así que conviene que diga qué se rompió y no qué se
comparó: `"el adulto entra a lo alto en la banda de escena"` sirve;
`"alto <= 176"` no.
