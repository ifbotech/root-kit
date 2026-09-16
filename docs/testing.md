# Pruebas

```bash
make test        # 1449 comprobaciones del firmware, sin placa ni SDL
make verify      # lo que corre CI: pruebas y referencias visuales al día
make placa       # compila las cuatro variantes con PlatformIO
```

ROOTLAB (la app, la nube y el emulador) tiene sus propias 300 pruebas en
[root-lab](https://github.com/ifbotech/root-lab) (`npm test`), incluido el
flujo completo de punta a punta.

## Qué cubre cada suite

| Suite | Comprobaciones | Qué protege |
|---|---:|---|
| `animo` | 28 | Prioridad entre necesidades, ciclo día/noche, histéresis, nodo caído |
| `nodo` | 91 | Calibración de suelo, fallas eléctricas, el riego que se escurre, curva de batería, muestreo adaptativo |
| `sensores e historial` | 71 | AHT20, BH1750 y DS18B20 con los vectores de las hojas de datos, CRC, riel y USB, sensores caídos, historial en flash |
| `graficos` | 63 | Recorte, tipografía, **antialiasing**: cobertura, bordes mezclados, triángulos en cualquier orden, alfa |
| `cara` | 371 | Determinismo, batería sin íconos, cara dormida que no delata, despertar, regresión visual de las 88 caras, la transición entre ánimos (extremos idénticos a las caras fijas, el medio distinto, el reloj con desborde), la cara de mimos (en 0 la del ánimo, en 100 otra, ronronea), la mirada dirigida y la preocupación |
| `modelos y caras` | 634 | Tabla de Rooties (con accesorios), la caja ciega, que los 8 y los 11 se distingan, centinelas del framebuffer |
| `pantalla del QR` | 19 | Que el QR dibujado se lea módulo por módulo en los dos paneles, también con la URL del VPS |
| `identidad y vinculo` | 91 | SHA-256 y HMAC con vectores oficiales, código y token, el flujo completo del enlace y sus caminos feos |
| `nube` | 71 | JSON hostil o cortado, el cuerpo del pedido, respuestas incoherentes que no se aplican |

## Las pruebas que valen más que su tamaño

**Que los ocho modelos se vean distintos.** Es la que sostiene el producto: si
dos carcasas dan la misma cara, la caja ciega vende dos veces lo mismo y no hay
colección que juntar. Se renderizan los ocho y se comparan por hash, todos
contra todos.

**Que los once ánimos se distingan DENTRO de cada modelo.** Son 55 pares por
modelo, 330 comparaciones. Un modelo que pone la misma cara para sed y para
frío no comunica nada, y con un rig procedural es fácil que dos ánimos colapsen
en la misma forma sin que nadie lo note. La versión anterior de este test ya
había encontrado tres estados compartiendo dibujo.

**Que el QR se lea.** Se dibuja la pantalla y se lee el framebuffer módulo
por módulo contra la matriz del QR, en 128×128 y en 240×320. Un corrimiento
de un pixel en el layout deja un QR que se ve perfecto y que ningún teléfono
lee; eso se descubre acá y no con la caja abierta.

**Que la placa y el servidor deriven el mismo código.** Los vectores de
[nube.md](nube.md) se calcularon aparte con Python y se verifican en C y en
Node. Si un lado cambia la derivación, ningún QR vincula nada.

**Los caminos feos del vínculo.** Contraseña mal tipeada, router que se
reinicia, nube que contesta antes que el wifi, cofre abierto con la maceta sin
red, desvincular a mitad del despertar, botón largo desde la cara. Cada uno es
un test que recorre la máquina de estados.

**Que un portal de hotel no desvincule una maceta.** Una respuesta HTML, sin
`"ok": true`, o cortada a la mitad, no se aplica.

**Que un sensor caído no invente un problema.** Sin AHT20 la temperatura
queda en cero; el test verifica que la planta no tenga "frío".

**Que la caja ciega tenga la forma que dice la caja.** Cinco modelos a la vista
y exactamente un secreto. Si esa proporción cambia sin querer, lo que está
impreso en el packaging deja de ser cierto.

**Que la cara dormida no delate al personaje.** Antes del cofre la maceta
duerme; si usara la piel de su personaje, la sorpresa se arruinaría. Se
verifica que ningún pixel tenga el color de fondo de ningún modelo.

**Que el despertar tenga su segundo intento.** Los ojos se abren, se vuelven a
cerrar y recién después se abren del todo. El test exige ese tramo de subida.

**Que las etapas se vean distintas.** Si dos etapas producen el mismo cuadro, el
crecimiento no existe para el usuario por más que el contador avance en NVS.

**Los centinelas del framebuffer.** El rig dibuja elipses y arcos con radios que
salen de una tabla editable a mano. Un radio de más escribe fuera del buffer, y
en el ESP32 eso no tira excepción: corrompe lo que haya al lado y aparece tres
días después como un bug imposible. Se barren los ocho modelos en los once
ánimos con todos los adornos, contra un buffer rodeado de guardas.

**El historial sobrevive un corte de luz.** Un byte corrupto o un archivo
cortado a la mitad se descartan enteros en vez de devolver lecturas falsas.

**La monotonía de la batería.** Se recorre la curva de 2,8 a 4,3 V verificando
que nunca baje. Sin eso, el ruido del ADC puede hacer que la batería "suba".

**La semana simulada.** Siete días de muestreo con una planta que se seca y se
riega, verificando que el muestreo adaptativo ahorre al menos la mitad de las
transmisiones — pero también que no ahorre de más, que sería perder eventos.

**Que un aparato caído no acumule días sanos.** Es lo que impide que
desenchufarlo haga crecer el vínculo gratis.

**Que el secreto no se liste hasta que sale.** Mostrarlo en gris ya le contaría
al usuario que existe.

**Que el mismo síntoma con distinta tierra dé causas distintas.** Hojas
amarillas con la tierra encharcada, seca o en rango son tres problemas
diferentes. Si las tres dieran lo mismo, la telemetría no estaría aportando
nada al diagnóstico y la foto sería decoración.

**Que marchita con la tierra mojada no se lea como sed.** Es el error más caro
que puede cometer alguien que cuida plantas: ve la planta caída, la riega, y le
termina de pudrir las raíces. El test exige que la acción diga "no riegues".

**Que la XP no se pueda acelerar.** Un mes con tres plantas no puede dar el
nivel máximo. Si se pudiera, el número mediría entusiasmo en vez de jardinería.

**Que una tarea marcada como hecha vuelva si no se resolvió.** El tilde esconde
la tarea dos horas; si la planta sigue seca después, reaparece.

## Regresión visual

`test/golden.h` guarda un FNV-1a del framebuffer de **cada modelo en cada
ánimo**: 88 hashes. Si un cambio altera cualquier pixel, la suite `cara` lo
marca.

Son 88 y no 11 a propósito. El rig es procedural y cada familia de ojos toma un
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

- **Nada toca hardware real.** Las conversiones de los sensores se prueban con
  los bytes de las hojas de datos, pero el I2C, el ADC, el SPI y el wifi de
  `esp32/` sólo se compilan: se prueban en la Fase 1 del [roadmap](roadmap.md).
- **El consumo es un modelo, no una medición.** Las cifras de autonomía salen de
  correr las funciones de `nodo/power.c` con parámetros que todavía no se
  verificaron con un multímetro. El reposo real del C3 y la corriente de cada
  retroiluminación son los dos números que pueden cambiar todo.
- **La regresión visual compara hashes, no aspecto.** Detecta que algo cambió;
  no dice si quedó mejor o peor. Para eso están `make sheet`, `make etapas`,
  `make despertar` y `make pantallas`, que hay que mirar con los ojos.
- **Nada verifica que una cara sea linda, ni que haga juego con su carcasa.**
  Eso se mira en la lámina y se ajusta en `core/persona.c`.
- **La interfaz de la app** se prueba en root-lab.
- **Las carcasas no están modeladas todavía.** `docs/carcasas.md` tiene el
  envolvente y las tolerancias, pero nada verifica que una pieza impresa
  encastre: eso se prueba imprimiendo.
- **El portal cautivo** se compila, pero que la página abra sola en cada
  teléfono sólo se ve con teléfonos reales.

## Cómo agregar una suite

1. Un archivo `test/test_<nombre>.c` con una función `suite_<nombre>(void)`.
2. Declararla en `test/rk_test.h` y llamarla en `test/main.c`.
3. Agregarla a `TESTSRC` en `firmware/Makefile`.

Los `CHECK_*` están en `test/rk_test.h`. La etiqueta de cada uno se lee en la
salida cuando falla, así que conviene que diga qué se rompió y no qué se
comparó: `"la cara dormida no usa la piel de ningún personaje"` sirve;
`"hash != 0"` no.
