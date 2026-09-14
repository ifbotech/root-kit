# Pruebas

```bash
make test        # todo: 215 comprobaciones de firmware + 40 del Hub
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
| `animo` | 28 | Prioridad entre necesidades, ciclo día/noche, histéresis, Spore caído |
| `protocolo` | 60 | Ida y vuelta de las tramas, CRC, códec de luz, rechazo de corrupción |
| `spore` | 63 | Calibración de suelo, fallas eléctricas, curva de batería, muestreo |
| `graficos` | 39 | Recorte de primitivas, sprites, tipografía, seno y hash |
| `render` | 25 | Determinismo, zona táctil, regresión visual por hash |
| `hub` | 40 | Formato, orden, validación, contrato de la API |

## Las pruebas que valen más que su tamaño

**Los 192 flips de un bit.** `test_proto.c` da vuelta cada bit de una trama de
telemetría, uno por vez, y verifica que el CRC los detecte todos. Es la
garantía que justifica gastar dos bytes en el CRC: una trama corrupta que pase
por buena mueve al simbionte a un estado equivocado con total convicción.

**Los centinelas del framebuffer.** `test_gfx.c` reserva el buffer con guardas
de 64 pixeles a cada lado y llama a todas las primitivas con coordenadas
imposibles. Un blit sin recorte no tira excepción en el ESP32: corrompe lo que
haya al lado, y eso aparece tres días después como un bug imposible.

**La monotonía de la batería.** Se recorre la curva de 2,8 a 4,3 V verificando
que nunca baje. Sin eso, el ruido del ADC puede hacer que la batería "suba" en
pantalla.

**La semana simulada.** `test_spore.c` corre siete días de muestreo con una
planta que se seca y se riega, y verifica que el muestreo adaptativo ahorre al
menos la mitad de las transmisiones — pero también que no ahorre de más, que
sería estar perdiendo eventos.

**Los hashes de referencia visual.** Ver abajo.

## Regresión visual

`test/golden.h` guarda un FNV-1a del framebuffer de 160×240 para cada estado de
ánimo, con un escenario fijo. Si un cambio altera cualquier pixel, la suite
`render` lo marca.

Cuando el cambio es intencional:

```bash
make golden      # regenera test/golden.h
git diff firmware/test/golden.h
```

El diff muestra exactamente qué pantallas cambiaron. Esa es la revisión que
uno quiere hacer antes de commitear arte, y es el motivo de que el archivo
generado se versione en vez de ignorarse.

Esta suite ya encontró dos defectos reales:

- La respiración de HAPPY tenía amplitud 2 y el redondeo entero se la comía:
  dos instantes distintos producían exactamente el mismo cuadro. Se subió a 4.
- Confirmó que la optimización de los blits y del tinte es **pixel a pixel
  idéntica** al código anterior. Sin los hashes habría que haber comparado
  capturas a ojo.

## Revisión visual a ojo

Los hashes detectan que algo cambió, no si quedó lindo. Para eso:

```bash
make sheet                                        # los 11 ánimos en una hoja
python tools/bmp2png.py firmware/build/sheet.bmp preview.png
./firmware/build/rootkit_sim --shot f.bmp HAPPY 4248   # un cuadro puntual
make sim                                          # interactivo
```

## Medición de performance

```bash
make bench
```

Imprime el costo por ánimo y por etapa, y la cota del bus. El número absoluto
de una PC no importa; lo que importa es la proporción entre etapas, que dice
dónde optimizar, y la comparación antes/después de un cambio.

## Qué NO está cubierto

Vale la pena tenerlo escrito para no confundir verde con terminado:

- **Nada del hardware real.** No hay ADC, ni I2C, ni QSPI, ni Wi-Fi. La capa
  HAL del ESP32 todavía no existe.
- **El framerate en el dispositivo.** El bench mide un x86. La medición real
  hay que hacerla con la Guition en la mano.
- **El consumo real.** El modelo de `power.c` está testeado contra sí mismo.
  Los microamperios de verdad se miden con un multímetro en serie, y es lo
  primero que hay que hacer cuando lleguen las placas.
- **La interfaz del Hub en un navegador.** Se testea la lógica y el contrato
  de la API, no el DOM ni el service worker.
- **Los rangos por especie.** Los cinco de `species.c` son aproximaciones de
  guías de cuidado corrientes, no datos calibrados.
