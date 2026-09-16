# Pruebas

```bash
make test        # todo: 862 comprobaciones de firmware + 58 de la app
make firmware    # sólo el firmware (no necesita SDL ni hardware)
make hub         # sólo la app (necesita Node)
make verify      # lo que corre CI, incluida la frescura del arte
```

El firmware se compila en WSL y la app corre en Node sobre Windows. `make test`
funciona desde los dos lados: si falta Node, el target de la app avisa en vez
de romper.

## Qué cubre cada suite

| Suite | Comprobaciones | Qué protege |
|---|---:|---|
| `animo` | 28 | Prioridad entre necesidades, ciclo día/noche, histéresis, nodo caído |
| `protocolo` | 60 | Ida y vuelta de las tramas, CRC, códec de luz, rechazo de corrupción |
| `nodo` | 63 | Calibración de suelo, fallas eléctricas, curva de batería, muestreo |
| `graficos` | 33 | Recorte de primitivas, elipses, arcos, tipografía, seno y hash |
| `cara` | 79 | Determinismo, aviso de batería, regresión visual de las 66 caras |
| `modelos y caras` | 502 | Tabla de carcasas, la caja ciega, que los 6 y los 11 se distingan |
| `kit y enlace` | 97 | Roster, salud del enlace, configuración, vuelta completa |
| `app` | 58 | Formato, orden, validación, vínculo, colección, contrato de la API |

## Las pruebas que valen más que su tamaño

**Que los seis modelos se vean distintos.** Es la que sostiene el producto: si
dos carcasas dan la misma cara, la caja ciega vende dos veces lo mismo y no hay
colección que juntar. Se renderizan los seis y se comparan por hash, todos
contra todos.

**Que los once ánimos se distingan DENTRO de cada modelo.** Son 55 pares por
modelo, 330 comparaciones. Un modelo que pone la misma cara para sed y para
frío no comunica nada, y con un rig procedural es fácil que dos ánimos colapsen
en la misma forma sin que nadie lo note. La versión anterior de este test ya
había encontrado tres estados compartiendo dibujo.

**Que quien recibe no recalcula el ánimo.** Protege una regla de arquitectura y
no una función. Se manda una telemetría cuyos números gritarían `THIRSTY` y cuyo
campo de ánimo dice `HAPPY`, y se verifica que se respeta el ánimo. Sin esto, la
maceta y el teléfono pueden mostrar caras distintas de la misma planta y no hay
forma de saber cuál miente.

**La vuelta completa.** La app arma la configuración desde la especie, la
codifica, el aparato la decodifica, reconstruye su especie, mide, evalúa con
esos umbrales, sabe qué carcasa lleva, emite, y la app aplica el resultado al
nodo correcto. Un solo test que atraviesa `core`, `net` y `nodo`.

**Que la caja ciega tenga la forma que dice la caja.** Cinco modelos a la vista
y exactamente un secreto. Si esa proporción cambia sin querer, lo que está
impreso en el packaging deja de ser cierto.

**Que el índice del modelo sea su posición.** El aparato recibe un número, no un
nombre. Si la tabla se reordena sin regenerar el catálogo, cada maceta se pone
la cara de la vecina y nada falla ruidosamente. Se verifica de los dos lados.

**Que los pictogramas no se pisen.** La cara dice que algo anda mal; el
pictograma dice qué. Si dos necesidades dibujan el mismo icono, el usuario riega
una planta que tenía frío.

**Que las etapas se vean distintas.** Si dos etapas producen el mismo cuadro, el
crecimiento no existe para el usuario por más que el contador avance en NVS.

**Los centinelas del framebuffer.** El rig dibuja elipses y arcos con radios que
salen de una tabla editable a mano. Un radio de más escribe fuera del buffer, y
en el ESP32 eso no tira excepción: corrompe lo que haya al lado y aparece tres
días después como un bug imposible. Se barren los seis modelos en los once
ánimos con todos los adornos, contra un buffer rodeado de guardas.

**Los flips de un bit.** Se da vuelta cada bit de una trama, uno por vez, y se
verifica que el CRC los detecte todos.

**La monotonía de la batería.** Se recorre la curva de 2,8 a 4,3 V verificando
que nunca baje. Sin eso, el ruido del ADC puede hacer que la batería "suba".

**La semana simulada.** Siete días de muestreo con una planta que se seca y se
riega, verificando que el muestreo adaptativo ahorre al menos la mitad de las
transmisiones — pero también que no ahorre de más, que sería perder eventos.

**Que un aparato caído no acumule días sanos.** Es lo que impide que
desenchufarlo haga crecer el vínculo gratis.

**Que el secreto no se liste hasta que sale.** Mostrarlo en gris ya le contaría
al usuario que existe.

## Regresión visual

`test/golden.h` guarda un FNV-1a del framebuffer de **cada modelo en cada
ánimo**: 66 hashes. Si un cambio altera cualquier pixel, la suite `cara` lo
marca.

Son 66 y no 11 a propósito. El rig es procedural y cada familia de ojos toma un
camino distinto, así que un cambio puede romper el visor sin tocar al ciclope.
La tabla está ordenada por modelo, y eso hace que el diff diga qué pasó: once
filas seguidas son "se movió un modelo", una columna es "se movió un ánimo en
todos". Son dos revisiones distintas.

Cuando el cambio es intencional:

```bash
make golden      # regenera firmware/test/golden.h
```

El archivo se versiona en vez de ignorarse porque **el diff del archivo
generado es la revisión del cambio visual**.

`make verify` —y CI— regeneran los hashes y fallan si el árbol queda sucio.

## Qué NO cubre

Conviene tenerlo escrito, porque la cobertura alta invita a confiar de más:

- **Nada toca hardware real.** No hay pruebas del ADC, del I2C, del SPI ni de la
  radio. Todo eso sigue siendo hipótesis hasta que lleguen las placas.
- **El consumo es un modelo, no una medición.** Las cifras de autonomía salen de
  correr las funciones de `nodo/power.c` con parámetros que todavía no se
  verificaron con un multímetro. El reposo real del C3 y la corriente de cada
  retroiluminación son los dos números que pueden cambiar todo.
- **La regresión visual compara hashes, no aspecto.** Detecta que algo cambió;
  no dice si quedó mejor o peor. Para eso están `make sheet`, `make etapas` y
  `make revelado`, que hay que mirar con los ojos.
- **Nada verifica que una cara sea linda, ni que haga juego con su carcasa.**
  Eso se mira en la lámina y se ajusta en `core/persona.c`.
- **No hay pruebas de la interfaz de la app en un navegador.** Se prueba la
  lógica pura y el contrato de la API, no el DOM.
- **Las carcasas no están modeladas todavía.** `docs/carcasas.md` tiene el
  envolvente y las tolerancias, pero nada verifica que una pieza impresa
  encastre: eso se prueba imprimiendo.
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
