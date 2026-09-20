# Pruebas

```bash
make test        # 2225 comprobaciones del firmware, sin placa ni SDL
make verify      # lo que corre CI: pruebas, referencias visuales y sustrato
make pcb         # verifica el sustrato impreso y regenera lo que sale de él
make placa       # compila el producto (c3-144) y el banco (devkit-144)
```

ROOTLAB (la app, la nube y el emulador) tiene sus propias 535 pruebas en
[root-lab](https://github.com/ifbotech/root-lab) (`npm test`), incluido el
flujo completo de punta a punta.

## Qué cubre cada suite

| Suite | Comprobaciones | Qué protege |
|---|---:|---|
| `animo` | 28 | Prioridad entre necesidades, ciclo día/noche, histéresis, nodo caído |
| `nodo` | 91 | Calibración de suelo, fallas eléctricas, el riego que se escurre, curva de batería, muestreo adaptativo |
| `sensores e historial` | 77 | AHT20, BH1750 y DS18B20 con los vectores de las hojas de datos, CRC, riel y USB, sensores caídos, historial en flash |
| `graficos` | 63 | Recorte, tipografía, **antialiasing**: cobertura, bordes mezclados, triángulos en cualquier orden, alfa |
| `cara` | 414 | Determinismo, batería sin íconos, cara dormida que no delata la piel, despertar con la piel, regresión visual de las 165 caras, la transición entre ánimos (extremos idénticos a las caras fijas, el medio distinto, el reloj con desborde), la cara de mimos (en 0 la del ánimo, en 100 otra, ronronea), la mirada dirigida y la preocupación |
| `rooties y caras` | 1023 | La tabla de los cinco Rooties con sus tres pieles y colores, las rarezas (ids, nombres, parseo), que los 5, las 15 pieles y los 11 ánimos se distingan, el guiño, adornos por etapa y por piel, centinelas del framebuffer |
| `pantalla del QR` | 19 | Que el QR dibujado se lea módulo por módulo (en 128×128 y en un lienzo más grande), también con la URL del VPS |
| `identidad y vinculo` | 91 | SHA-256 y HMAC con vectores oficiales, código y token, el flujo completo del enlace y sus caminos feos |
| `nube` | 98 | JSON hostil o cortado, el cuerpo del pedido, respuestas incoherentes que no se aplican; a qué URL se le puede mandar el token (sólo `https://` exacto en el producto, sin usuario ni caracteres raros) |
| `sustrato y placa.h` | 217 | Que el sustrato impreso y el firmware digan lo mismo: cada GPIO en su red, los analógicos en el ADC1, el toque en un pin que despierta, los tres pines de arranque en alto, el 1-Wire y su pull-up en el mismo riel, el divisor del riel, y las trampas del banco (ADC2, ext0, pines sólo de entrada) |
| `ota y fabrica` | 104 | Versiones, hex y base64; manifiestos hostiles o a medias; cuándo se baja una versión (batería, tres intentos, volver atrás) y el arranque a prueba; el cuerpo del sync con `ota` y `lote`; la línea de fábrica: secretos cortos o en cero, Rooties que no existen, lotes raros, el log que no es una orden |

## El sustrato también se verifica, aparte

`make test` cruza el sustrato con `placa.h`, pero la **geometría** de la PCB
impresa se verifica en Python, porque es geometría:

```bash
python3 tools/pcb.py --verificar     # y dentro de `make verify` y de CI
```

Revisa que ninguna canaleta quede a menos de 0,8 mm de otra (dos
extrusiones: menos que eso no se imprime), que cada red quede en **una sola
pieza** contando pistas y puentes, que nada de cobre entre en la ventana, los
recortes o los tornillos, que se respete la zona libre de la antena y que el
texto grabado no muerda una pista. Y CI falla si los archivos que salen del
dato —la plantilla de corte, el diagrama de conexiones y `test/redes.h`— no
están commiteados al día, por la misma razón que `golden.h`: una plantilla
vieja se imprime igual de bien y arma una placa que no anda.

## Las pruebas que valen más que su tamaño

**Que un manifiesto raro nunca haga bajar algo.** Una versión que no es una
versión, una URL que no es http, un hash corto, una firma diminuta, un tamaño
que no entra en la partición: el manifiesto queda en cero y el resto del sync
se aplica igual. Y una versión que ya falló tres veces no se vuelve a bajar.

**Que la fábrica no le cambie la identidad a una maceta con cualquier cosa.**
La línea del puerto serie se valida entera antes de tocar la NVS, y un
aparato vinculado la rechaza.

**Que los cinco Rooties y sus quince pieles se vean distintos.** Es la que sostiene el producto: si
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

**Que las rarezas sean las que publica la app.** Tres pieles por Rooti, en el
orden común, rara, épica, con los ids que manda la nube (`comun`, `raro`,
`epico`); un valor desconocido no pinta nada raro. En root-lab, el sorteo
respeta 70 / 25 / 5 en veinte mil tiradas.

**Que la cara dormida no delate la piel.** Antes del cofre la maceta duerme
en gris; si usara los colores de una piel, la sorpresa se arruinaría. Se
verifica que ningún pixel tenga el color de fondo de ninguna piel.

**Que el despertar tenga su segundo intento.** Los ojos se abren, se vuelven a
cerrar y recién después se abren del todo. El test exige ese tramo de subida.

**Que las etapas se vean distintas.** Si dos etapas producen el mismo cuadro, el
crecimiento no existe para el usuario por más que el contador avance en NVS.

**Los centinelas del framebuffer.** El rig dibuja elipses y arcos con radios que
salen de una tabla editable a mano. Un radio de más escribe fuera del buffer, y
en el ESP32 eso no tira excepción: corrompe lo que haya al lado y aparece tres
días después como un bug imposible. Se barren los cinco Rooties con sus tres
pieles en los once ánimos con todos los adornos, contra un buffer rodeado de guardas.

**El historial sobrevive un corte de luz.** Un byte corrupto o un archivo
cortado a la mitad se descartan enteros en vez de devolver lecturas falsas.

**La monotonía de la batería.** Se recorre la curva de 2,8 a 4,3 V verificando
que nunca baje. Sin eso, el ruido del ADC puede hacer que la batería "suba".

**La semana simulada.** Siete días de muestreo con una planta que se seca y se
riega, verificando que el muestreo adaptativo ahorre al menos la mitad de las
transmisiones — pero también que no ahorre de más, que sería perder eventos.

**Que un aparato caído no acumule días sanos.** Es lo que impide que
desenchufarlo haga crecer el vínculo gratis.

**Que las siluetas se puedan imprimir.** En root-lab, `test/cuerpo.test.mjs`
mide el voladizo de cada silueta sobre la curva dibujada (45° como máximo),
la base plana, el centro de masa y que la ventana del TFT entre con su bisel.

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

`test/golden.h` guarda un FNV-1a del framebuffer de **cada Rooti con cada piel
en cada ánimo**: 165 hashes. Si un cambio altera cualquier pixel, la suite
`cara` lo marca.

Son 165 y no 11 a propósito. El rig es procedural y cada familia de ojos toma
un camino distinto, así que un cambio puede romper la medialuna del Musgo sin
tocar los ojos redondos del Brote, y una piel con aura sin tocar la común.
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
