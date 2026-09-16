# La app

Una web app que se instala desde un QR y abre a pantalla completa. Sin tienda,
sin build step, sin `node_modules`.

```bash
make serve     # http://localhost:8080
```

## El recorrido del usuario

```
  caja con QR  ->  se abre la web  ->  "agregar a inicio"  ->  ícono propio
                                                               pantalla completa
       |
       v
  escanear la planta  ->  la IA dice qué es  ->  ponerle nombre
                                                 decir qué carcasa salió
       |
       v
  el aparato recibe los umbrales de esa especie y su carcasa,
  y a partir de ahí pone cara sin preguntarle a nadie
```

## Las cuatro pantallas

**Hoy.** Lo primero que se ve. Tareas arriba, contadores en el medio, nivel
abajo. Ese orden no es decorativo: las tareas son lo único accionable, los
contadores contestan "¿está todo bien?" de un vistazo, y el nivel es
recompensa — poner la recompensa arriba la convertiría en el objetivo.

**Plantas.** La lista y el detalle. Acá viven los números, con el rango de la
especie al lado de cada uno.

**Escanear.** La cámara, en sus dos trabajos: identificar una planta nueva y
diagnosticar una que ya está registrada.

**Colección.** Las carcasas que ya tenés y los logros.

## Tres decisiones que explican el diseño

### Una tarea es un verbo, no un estado

"Regar la Monstera" es una tarea. "Monstera con sed" es un estado. Un tablero
que muestra estados le deja al usuario el trabajo de interpretarlos; una lista
de tareas ya hizo ese trabajo.

Y **cada tarea muestra el número que la justifica**: no "necesita agua" sino
"la tierra está al 22% y la Monstera quiere entre 25 y 60". Sin el número la
app pide fe. Con el número se le puede discutir, y alguien que puede discutirle
al aparato es alguien que le va a creer cuando tenga razón.

### Las tareas se cierran solas

Si regás, el sensor lo ve y la tarea desaparece sin que nadie toque un botón.
El tilde sólo la esconde dos horas mientras el sensor se pone al día; si
pasado ese rato la planta sigue seca, la tarea **vuelve** — y volver es
correcto, porque significa que el riego no alcanzó.

Una app de plantas que te hace tildar casilleros es una app de listas con
plantas de decoración.

### El diagnóstico vale cuando los sensores no ven nada

El sensor mide cuatro cosas. La planta tiene muchos más problemas que esos
cuatro: falta de nitrógeno, exceso de sales, araña roja, hongos, raíces
podridas. Nada de eso mueve una lectura y todo eso se ve en una hoja.

Por eso el mismo síntoma con distinta telemetría es un problema distinto:

| Se ve | El sensor dice | Conclusión |
|---|---|---|
| hojas amarillas | tierra encharcada | exceso de riego, raíces ahogadas |
| hojas amarillas | tierra seca | sequía sostenida |
| hojas amarillas | **tierra en su rango** | **falta de nutrientes** |

La tercera fila es la que justifica sacar la foto: un problema real que ningún
sensor va a ver, y que sin la cámara el usuario atribuiría al riego porque es
lo único que sabe mirar. La interfaz marca esos casos distinto —"la cámara ve
algo que los sensores no"— porque son los valiosos.

El caso más caro está cubierto aparte: **una planta marchita con la tierra
mojada no tiene sed, tiene las raíces podridas.** Parece lo mismo y la acción
es la contraria. Hay un test que lo fija.

## Gamificación: se gana cuidando, no usando

Se gana XP por días sanos, rachas y etapas del vínculo. Abrir la app, mirar un
gráfico o tocar un botón no dan nada. Si dieran, el número mediría enganche en
vez de jardinería, y un número que mide enganche empuja a la app a pedir
atención que no necesita.

De ahí sale una consecuencia incómoda y correcta: **no se puede acelerar.**
Alguien que quiera el último nivel tiene que mantener plantas vivas medio año.

Los logros de colección no dan XP: tener las seis carcasas demuestra que
compraste cajas, no que sepas regar.

## Por qué no hay React

No por purismo, por despliegue. Sin build step, publicar es copiar una carpeta,
`make serve` anda sin instalar nada, y no hay un `node_modules` que se pudra
entre una sesión y la siguiente. Para una app que mantiene una persona y que se
actualiza cuando esa persona tiene un rato, eso vale más que el azúcar
sintáctico.

Lo que sí hace falta de un framework —no escribir `document.createElement`
cuarenta veces— son las cuatro funciones de `lib/ui.mjs`. Si algún día el
proyecto necesita React de verdad, entra por CDN sin tocar el resto.

## Cómo se instala a pantalla completa

- **Manifest** con `display: standalone`, íconos de 192, 512 y uno *maskable*
  para el recorte circular de Android.
- **Android** dispara `beforeinstallprompt` y la app ofrece un botón.
- **iOS** no lo implementa: se detecta y se muestra la instrucción ("Compartir
  → Agregar a inicio"). iOS además ignora el manifest para esto y necesita sus
  propias etiquetas `apple-mobile-web-app-*` o la app abre dentro de Safari con
  la barra de direcciones puesta.
- `viewport-fit=cover` más los `env(safe-area-inset-*)` del CSS: sin eso, a
  pantalla completa en un iPhone la barra de pestañas queda debajo del
  indicador de inicio y no se puede tocar.

Los íconos se generan con `python tools/gen_icons.py` y dibujan la cara del
ROOTKIT, que es lo mismo que el usuario ya vio en la caja y en la maceta.

## El service worker

Hace dos cosas: que la app abra sin red y que se actualice sola.

**Los datos nunca se cachean.** Una lectura vieja mostrada como actual es peor
que no mostrar nada: el usuario decide si regar mirando ese número, y un "34%"
de ayer lo hace regar una planta que ya está mojada. Todo lo que cuelga de
`/api/` va directo a la red, y si no hay red la app lo dice en vez de inventar.

El armazón sí se cachea, porque no caduca. Al tocar cualquier archivo del
armazón hay que subir `CACHE` en `sw.js` o el teléfono se queda con la versión
vieja.

## Lo que falta

- **Decidir dónde vive la API.** Hoy la app habla con el contrato de
  `hub/API.md`, que puede servir tanto una nube como un concentrador local. La
  restricción a tener en cuenta: una página servida por HTTPS desde internet
  **no puede** pedirle datos a una IP privada, así que si la app se hospeda
  afuera, los aparatos tienen que publicar a un backend y no al revés.
- **Autenticación.** No hay ninguna todavía. El QR de la caja es el candidato
  natural para llevar un código de emparejamiento.
- **Historial.** El endpoint existe y devuelve 48 puntos; falta el gráfico.
- **Notificaciones push.** Una planta con sed a las tres de la tarde y nadie
  mirando la app es el caso que más justifica una notificación, y es lo único
  de esta lista que necesita un backend sí o sí.
