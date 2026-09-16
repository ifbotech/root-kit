# Registro de decisiones

Las decisiones cerradas y por qué, para no volver a discutirlas.
El análisis competitivo completo está en el dossier.

Las decisiones que se **revisaron** quedan acá abajo, en su propia sección, con
lo que las tumbó. Borrarlas sería perder la parte más útil del registro: saber
por qué algo que parecía correcto dejó de serlo.

## Arquitectura

**Un ROOTKIT es un aparato en una maceta.** Una placa, sus sensores y una
carcasa impresa en 3D que le da la cara. Se compra en caja ciega: cinco
modelos a la vista y un secreto, como un Smiski.

**La variedad es física; la cara es digital.** Lo único que se diseña en
pixeles es la cara. Lo que cambia entre un modelo y otro es la carcasa
impresa: su cresta, su pelo, su visera. Es mejor reparto de esfuerzo que
dibujar cuerpos — imprimir una carcasa cuesta filamento y unas horas de
modelado, dibujar y animar un cuerpo cuesta semanas. Y una cara sola en
128×128 tiene muchos más pixeles por rasgo que un cuerpo entero.

**El ánimo dice qué siente; la persona dice cómo lo muestra.** `core/mood.c`
decide el estado a partir de la planta y `core/persona.c` decide cómo se
dibuja ese estado. Seis modelos por once ánimos son sesenta y seis caras, y
salen todas del mismo código porque la cara es procedural.

**La especie y la carcasa son ejes independientes.** La especie decide los
umbrales; la carcasa decide la cara. Ninguna deriva de la otra, y esa
independencia es el producto: la misma planta con dos carcasas se ve distinta,
y la misma carcasa cuida cualquier planta.

**Las métricas viven en la app.** El aparato sólo dibuja una cara: ni números,
ni barras, ni nombre. Sacarle la interfaz es además lo que hace que la carcasa
mande — una pantalla llena de barras compite con el objeto, una cara lo
completa.

**La rareza es de la caja, no de la planta.** No hay compra aleatoria dentro
de un software: hay un juguete en una caja. Es la mecánica de los Smiski y los
Sonny Angel, y queda todavía más lejos del terreno regulado de las cajas de
botín que la versión anterior.

**Lo que se gana cuidando la planta es cómo se ve, no quién es.** Los días
sanos desbloquean capas cosméticas sobre la cara: brillos a los 30, aura a los
90, corona a los 180. El modelo te toca por azar; el aura no se compra.

**El secreto se imprime en filamento translúcido.** No cuesta un peso más que
cualquier otro color y hace algo que los otros cinco no hacen: deja de ocultar
el aparato y pasa a exhibirlo. Es una diferencia de categoría, no de color.

## Hardware

**Pantalla: TFT 1,44" 128×128 IPS, ST7735, SPI.** Área activa 25,9 × 25,9 mm.
ARS 9.160. Es IPS, tiene el pin de retroiluminación accesible para PWM, y su
controlador tiene librería madura. Cinco GPIO para la pantalla, que es lo que
permite que entren los sensores en un C3.

**Se descartó el Prime de 2,2 pulgadas.** Existía para mostrar al simbionte
grande y para tener el tablero. Cuando las métricas se mudaron a la app y el
bicho se redujo a una cara, esa pantalla se quedó sin trabajo: costaba el
doble para mostrar lo mismo más grande. Ver la sección de decisiones revisadas.

**Descartada la shield de 2,4" para Arduino UNO (ARS 13.069).** Es paralela de
8 bits: trece GPIO sólo para la pantalla, contra cinco de la SPI. Un C3
SuperMini tiene diez pines útiles. Además es de 5 V —lleva conversores de nivel
en la placa— y el controlador es una lotería: estas shields MCUFRIEND vienen con
ILI9341, ILI9325, R61505 o S6D0154 según el día. Para un proyecto suelto es un
detalle; para cien unidades vendidas es descalificante. Y no gana nada a cambio:
su paso de pixel de 0,158 mm da una criatura de 22,8 mm, prácticamente la misma.

**Descartada la Guition JC3248W535 como Terminal.** Fue la elección de la
primera versión —ESP32-S3, 3,5" 320×480 IPS, táctil capacitivo— y quedó afuera
cuando el producto dejó de tener una terminal de escritorio. Cuesta ARS 69.103,
más que siete veces la pantalla actual.

**Descartada la CYD de 2.8" (ESP32-2432S028R).** Táctil resistivo — por eso
viene con stylus. Rompe el mecanismo central del producto: no se puede
acariciar al simbionte, se lo aprieta. No es cuestión de precio.

**Descartada la Waveshare ESP32-S3-RLCD-4.2.** Panel reflectivo sin
retroiluminación, descrito por el fabricante como "experiencia tipo papel
electrónico": es la misma renuncia que hizo PlantSenso con su E-Ink, justo en el
terreno donde ROOTKIT gana.

**Sensores: capacitivo v2.0 + AHT21 + BH1750, en cada aparato.** Uno con menos
sensores sería uno que no puede evaluarse solo, y eso rompe la regla central
del sistema. El AHT21 en particular no se comparte: una maceta interior y una
de balcón son microclimas distintos.

**La pantalla se enciende por rato, no siempre.** Ninguna pantalla a color
sobrevive siempre encendida a batería: una IPS da entre 0,9 y 3 días. Con la
pantalla apagada salvo cuando alguien mira, el aparato vive año y medio.

**Batería 18650, no LiPo.** A precios de MercadoLibre: 18650 de 2200 mAh con
portapilas sale unos ARS 5.000 comprando de a diez — ARS 2,27 por mAh — contra
ARS 17 a 23 por mAh de cualquier LiPo chica. Es más barata en términos absolutos
que la LiPo de 300 mAh y da siete veces más capacidad. Además, si la celda es
reemplazable por el usuario desaparecen el TP4056 y el conector USB, y con ellos
un agujero menos que sellar en un aparato que vive en tierra húmeda.

**Prohibido el FC-28 / YL-69.** Resistivo: se consume por electrólisis en 3 a 6
meses, y mide sales disueltas en vez de agua, así que fertilizar altera la
lectura. El propio aviso admite las dos cosas.

## Comunicación

*(Revisada: ver "El protocolo binario sobre ESP-NOW" abajo.)*

**Protocolo binario, no JSON.** Una trama de telemetría son 26 bytes; el mismo
contenido en JSON son unos 190. Cada byte es tiempo de radio encendida, y la
radio es cerca de la mitad del presupuesto energético del aparato.

**La configuración lleva los umbrales de la especie y el índice de la
carcasa.** Son 32 bytes contra los 18 de antes, y es lo que permite que el
aparato evalúe su propia maceta y sepa qué cara poner. No pesa en la batería:
viaja una sola vez al emparejar y cada vez que cambia la especie o la carcasa,
en sentido app → aparato, sobre un nodo que ya está despierto.

**Los dos límites de luz viajan comprimidos con el mismo códec que la lectura.**
Mantisa de 12 bits y exponente de 4. El error relativo arriba de 4.095 lux es
menor al 0,03%, muy por debajo de la tolerancia del propio BH1750, y reutiliza
un códec que ya estaba testeado en vez de agregar cuatro bytes crudos.

**El ánimo viaja resuelto, en un byte empaquetado.** Cuatro bits de ánimo y dos
de severidad. Empaquetar en vez de gastar dos bytes no es capricho: la trama
tiene que seguir entrando en un paquete ESP-NOW corto.

**La iluminancia viaja con mantisa y exponente.** Hay que cubrir de 1 a 100.000
lux. En 16 bits lineales habría que sacrificar la resolución baja, que es justo
donde vive el umbral de noche.

**CRC16-CCITT sobre toda la trama.** En 2,4 GHz con vecinos ruidosos, una trama
corrupta que pase por buena le pone al aparato una cara equivocada con total
convicción. Un test verifica que todos los flips de un bit se detecten.

**Medir y transmitir van desacoplados.** Medir cuesta 250 nAh y transmitir
28.000: 110 veces más. Se mide seguido y se emite sólo ante un cambio, un cruce
de umbral, un cambio de estado de batería o el latido de dos horas. Una semana
simulada da 68% menos de radio que un intervalo fijo de 15 minutos.

## Arte y colección

**Seis familias de ojos, no seis juegos de sprites.** Cada modelo elige una
familia —redondos, rasgados, fieros, visor, único, pesados— y el ánimo elige
la forma dentro de esa familia. Dibujar sesenta y seis caras a mano sería un
mes de trabajo, y agregar un modelo costaría once caras más.

**Los rasgos se miden en centésimas del ancho del panel, no en pixeles.** Así
la misma tabla de modelos sirve para cualquier panel futuro sin tocar un
número, y quien ajusta el arte edita proporciones y no coordenadas.

**El modelo secreto no se lista hasta que sale.** Mostrarlo en gris ya le
contaría al usuario que existe, y ahí deja de ser un secreto para ser una
casilla vacía que además le informa cuántos le faltan.

**La colección no cuenta repetidos.** Registra qué modelos tenés, no cuántos
de cada uno. Contar duplicados la convertiría en un inventario, y un
inventario no da ganas de completar nada.

**El primer encendido no repite el gachapón.** La sorpresa ya ocurrió al abrir
la caja; hacerla de nuevo en pantalla sería contar dos veces el mismo chiste y
el segundo sería el falso. La escena cambia de sentido: no es "mirá lo que te
tocó" sino "ah, entonces soy este" —el aparato despierta a oscuras, se
descubre los rasgos y se presenta. Dura 3,6 s contra los 5,2 de la ceremonia
anterior, porque acompaña un momento en vez de intentar ser el momento.

**El vínculo crece con días sanos, no con días transcurridos.** Una planta
abandonada tiene un aparato que no evoluciona, y ahí está toda la mecánica.
Pero un mal día corta la racha sin borrar lo acumulado: castigar un descuido
con meses de progreso convierte un olvido en motivo para abandonar el
producto.

**Un aparato caído no acumula días sanos.** Si no sabemos cómo estuvo la
planta, no se premia. Es lo que impide que desenchufarlo haga crecer el
vínculo gratis.

**El catálogo de la app se genera desde el del firmware.**
`tools/sync_catalog.py` lo deriva de `species.c` y `persona.c`, y CI falla si
quedó desfasado. El índice del modelo es la clave con la que la carcasa viaja
por radio: si la tabla se reordena sin regenerar, cada maceta se pone la cara
de la vecina.

## Software

**El núcleo es C99 puro.** `firmware/core/` no depende de ESP-IDF y no usa punto
flotante. Se compila igual en el simulador, en los tests y en las dos placas. La
temperatura viaja en décimas de grado en un `int16_t`.

**Prioridad de estados de ánimo: agua, temperatura, luz, aire.** Se muestra la
necesidad insatisfecha más urgente, no un promedio. El agua va primero porque es
lo que mata más rápido.

**Histéresis en todos los umbrales.** Sin banda de salida, el simbionte titila
entre feliz y sediento cada vez que la lectura oscila un punto sobre el límite.

**De noche el simbionte duerme.** Tras ocho muestras consecutivas por debajo de
15 lux se suprimen las quejas por luz y por aire seco, pero no las de agua ni las
de temperatura.

**Resolución nativa, sin lienzo lógico.** 128×128 son 32 KB: entran holgados en
los 400 KB de SRAM de un ESP32-C3, con lugar de sobra para un segundo buffer y
mandar por DMA mientras se dibuja el siguiente.

**El aparato no muestra números.** Un "34%" en 1 mm de alto no se lee, y si se
leyera no significaría nada sin el rango de la especie al lado. El número
exacto está en la app, que es donde alguien lo va a ir a buscar.

**Renderer propio en vez de LVGL.** La cara es procedural: elipses, arcos y
trazos calculados. LVGL trae un motor de widgets que acá no se usaría nunca, y
un buffer RGB565 plano es exactamente lo que espera
`esp_lcd_panel_draw_bitmap()`.

**El presupuesto energético vive en código testeado, no en una planilla.** Las
cifras de autonomía salen de correr `make test`, así que si alguien cambia un
parámetro el número se rompe en vez de quedar viejo en silencio. Los costos por
evento van en nanoamperios-hora: en microamperios-hora enteros, una medición de
0,25 µAh se redondea a cero y desaparece del modelo.

**Regresión visual por hash, una fila por modelo y ánimo.**
`firmware/test/golden.h` guarda un FNV-1a del framebuffer de las sesenta y seis
caras. Se versiona en vez de ignorarse porque el diff del archivo generado *es*
la revisión del cambio visual: once filas seguidas dicen "se movió un modelo",
una columna dice "se movió un ánimo en todos".

**La app es una PWA, no una app nativa.** La foto obliga a tener el celular,
pero no una App Store: una página web abre la cámara con `<input capture>`. Sin
tienda no hay cuota anual ni nadie revisando las mecánicas de colección contra
las políticas de cajas de botín. Se sirve por HTTP en la red local, porque una
página HTTPS tiene prohibido pedirle datos a una IP privada.

## Producto

**Sin pH.** Ni PlantSenso, ni Ivy, ni Mi Flora miden pH real de suelo: es
genuinamente difícil y todos los aparatos de consumo que muestran el número lo
están inventando. No mostrarlo es honesto y además es un argumento de venta para
el público al que apunta ROOTKIT.

**El azar está en la caja, no en el software.** Es la misma mecánica que los
Smiski: comprás una caja ciega y te toca un juguete. El cofre de la app revela
la persona grabada en fábrica; sólo tira dados en prototipos sin persona, con
probabilidades públicas y nada que comprar, así que el riesgo regulatorio de
las cajas de botín —que Bélgica y Países Bajos ya restringen— no aplica.

**Nada de la palabra Tamagotchi en el marketing.** Es marca registrada de Bandai.
El vocabulario propio — simbionte, criatura digital — es más distintivo igual.

## La nube, el QR y el cofre

**La pantalla muestra dos cosas: el QR y los ojos.** Ni batería, ni wifi, ni
pictogramas, ni texto. Todo lo que no es una cara es presentación, y la
presentación vive en la app. La batería crítica se nota igual: con la planta
bien, la cara se duerme; el aviso con palabras llega como notificación.

**El primer encendido es un QR.** Lleva a la app en `/v/<código>`. Debajo va el
código en letras grandes, partido en dos grupos de cuatro, porque es el plan B
cuando la cámara no enfoca y el plan A en iPhone cuando hay que tipearlo.

**Base32 de Crockford para el código.** Sin I, L, O ni U: se dicta y se tipea
sin confundir 0 con O. Y todos sus caracteres entran en el modo alfanumérico
del QR, que en el panel de 128 es la diferencia entre módulos de 3 y de 2
pixeles.

**El código cambia con cada desvinculación.** Sale de una época que sube al
desvincular. Un QR fotografiado por el dueño anterior deja de valer en ese
instante.

**Portal cautivo para pasar el wifi.** Anda en cualquier teléfono sin instalar
nada. Web Bluetooth no existe en iPhone, y una página HTTPS no puede hablarle a
`192.168.4.1`.

**En la app, instalar antes que avisos.** En iPhone las notificaciones web
existen sólo para la app instalada. Pedir el permiso antes sería pedir algo
imposible, y el usuario lo rechazaría para siempre.

**El cofre se abre en la app y la maceta despierta.** La sorpresa de quién
te tocó pasa en la pantalla grande, con luz y confeti. La maceta muestra la
consecuencia: abre los ojos. El pedido al servidor sale recién al tercer toque
para que las dos cosas pasen juntas.

**Antes del cofre, ojos dormidos y grises.** Mostrar la piel del personaje en
la maceta arruinaría la sorpresa.

**La persona se graba en fábrica; si no hay, el cofre tira.** Con carcasa
impresa, el personaje ya existe y el cofre lo revela. En prototipos sin
persona grabada, el cofre elige con probabilidades públicas (70 / 25 / 5) y
nada que comprar para cambiarlas.

**La especie sale de una foto, y los umbrales del catálogo.** La IA identifica;
si la planta está en el catálogo curado se usan sus números y no los del
modelo. Si no está, los rangos del modelo pasan por la misma validación que el
firmware (acotados y coherentes).

**Las caras de la app las dibuja el firmware.** El núcleo compilado a
WebAssembly pesa 50 KB. No hay una versión CSS de las caras que se pueda
desincronizar de la maceta.

**El aparato decide su cara; la nube, los días sanos.** La cara no puede
depender de la red. Los días sanos sí se cuentan en la nube, que ve el día
entero aunque el aparato duerma, y se los devuelve para los adornos.

**Un pedido por sincronización.** Cada conexión TLS cuesta casi un segundo de
radio: juntar estado, lecturas y configuración en un ida y vuelta es la mayor
optimización del protocolo.

**Sin reloj de pared.** Las lecturas llevan "hace N segundos" desde un reloj
monótono que suma el deep sleep. El servidor les pone fecha.

**Una respuesta sin `"ok": true` no se aplica.** Un portal de hotel que
contesta HTML no puede desvincular una maceta.

**C3 SuperMini y no ESP32 de 30 pines.** El regulador del DevKit consume más
durmiendo que el C3 midiendo. Detalle en [hardware.md](hardware.md).

**Un sensor caído no inventa problemas.** Cada lectura marca qué sensores
fallaron y el ánimo ignora esas magnitudes: sin AHT20 no hay frío, hay "no sé".

**Avisar por estado, no por lectura.** Una sed de seis horas es un aviso, no
veinticuatro. Se repite a las 8 h, o a las 3 h si es urgente. De 23 a 8, sólo
lo urgente.

---

## Decisiones revisadas

### El protocolo binario sobre ESP-NOW

**Antes:** tramas binarias de 26 y 32 bytes con CRC16, un Prime que recibía por
ESP-NOW y servía el tablero en la red local.

**Qué lo tumbó:** las notificaciones. Tienen que llegar con la app cerrada y
el teléfono fuera de casa, y eso lo manda un servidor. Con la maceta hablando
HTTPS con la nube, el costo de radio lo pone el apretón de manos TLS y no el
tamaño del cuerpo: JSON pasó a costar lo mismo y se lee en un log. Lo que
sobrevivió del protocolo viejo: medir y transmitir desacoplados, y el ánimo
evaluado en el aparato.

### El pixel art

**Antes:** caras de trazo duro, sin suavizado, escaladas por enteros.

**Qué lo tumbó:** la dirección de arte. Las caras tienen que ser ilustración
plana tipo Duolingo, y ese estilo vive en el borde de las curvas: sin
suavizado se ve escalonado. Se escribió `gfx/aa.c`, antialiasing en punto
fijo, y el fondo de la cara pasó a ser liso para que los párpados sean piel.

### El primer encendido como ceremonia en la pantalla

**Antes:** la maceta se "descubría la cara" sola, con nombre y rareza en
letras.

**Qué lo tumbó:** el cofre en la app. La sorpresa pasa en el teléfono; la
maceta sólo abre los ojos, sin texto. La pantalla del aparato ya no escribe
nada salvo el código del QR.

### El aviso de batería y el pictograma en la pantalla

**Antes:** un ícono de pila parpadeante y un pictograma (gota, sol, copo) en la
esquina.

**Qué lo tumbó:** la regla de "QR y ojos". Lo que pide la planta lo dice la
cara y lo detalla la notificación; la batería crítica duerme la cara.

### Declarar la carcasa en la app

**Antes:** al dar de alta la maceta, el usuario elegía qué carcasa le había
tocado.

**Qué lo tumbó:** la persona grabada en fábrica y el cofre. El usuario no
declara nada: lo descubre.

### El gachapón se volvió físico

**Antes:** doce simbiontes dibujados, cada uno con su cuerpo pixel art, y el
que te tocaba salía de la especie de tu planta. La rareza venía de la
dificultad hortícola: el bonsái daba un legendario porque mantenerlo vivo es
trabajo real.

**Qué la tumbó:** dos cosas a la vez. La primera es que **no funcionaba
visualmente**. Los doce compartían silueta —una tortuga repintada doce veces—
y eso estaba anotado acá como compromiso explícito, pero "documentado" no es
lo mismo que "bueno". La segunda es que el esfuerzo estaba mal repartido:
dibujar y animar un cuerpo nuevo cuesta semanas, imprimir una carcasa cuesta
filamento y unas horas de modelado.

**Lo que la reemplaza:** la variedad se mudó al plano físico. Seis carcasas
impresas en caja ciega, y en pantalla sólo la cara, que le hace juego a la
carcasa que te tocó. Seis modelos por once ánimos son sesenta y seis caras que
salen de un rig procedural en vez de sesenta y seis dibujos.

**Lo que se ganó además, sin buscarlo:** la posición legal quedó más firme. La
rareza determinista existía para no ser una caja de botín; ahora directamente
no hay ninguna tirada de dados dentro del software. Hay un juguete en una
caja, que es lo que venden los Smiski desde hace años.

**Lo que se perdió:** la rareza ya no premia el trabajo hortícola. Un potus y
un bonsái tienen la misma chance de venir con el secreto. Se compensó moviendo
el mérito a lo cosmético —los días sanos desbloquean brillos, aura y corona—
pero es un premio más chico y conviene saberlo.

### El Prime y los Minis

**Antes:** un Prime enchufado con pantalla de 2,2 pulgadas y hasta cinco Minis
a batería. El Prime mostraba el tablero, abría las cápsulas y era donde el
simbionte se veía grande.

**Qué la tumbó:** mover las métricas a la app. Con el tablero afuera y el
bicho reducido a una cara, la pantalla grande se quedó sin trabajo: costaba el
doble que la de 1,44" para mostrar lo mismo más grande. Y la jerarquía entre
"adulto" y "brote", que era la razón de producto del Prime, dejó de tener
sentido cuando el personaje pasó a ser la carcasa.

**Lo que se conservó:** `gfx/panel.h` sigue describiendo los dos tamaños y el
rig dibuja en cualquiera, porque los rasgos se miden en centésimas del ancho.
Una variante grande es un cambio de configuración, no un rediseño.

### El nodo dejó de ser tonto

**Antes:** el Spore medía, empaquetaba y dormía. No conocía especies ni
umbrales, y toda la interpretación vivía en la Terminal.

**Qué la tumbó:** ponerle pantalla a cada nodo. Un aparato que necesita la red
para saber qué cara poner se queda mudo justo cuando más importa.

**Cómo se respetan las razones originales.** La batería: evaluar el ánimo son
unas pocas comparaciones enteras sobre datos que ya están en RAM, y no agrega
un solo despertar. Las actualizaciones: los umbrales no están compilados,
viajan en `CONFIG`. La implementación única: `core/mood.c` sigue existiendo una
sola vez.

**Y la regla nueva que hizo falta:** quien recibe **no recalcula**. Hay un test
que manda una telemetría cuyos números gritarían `THIRSTY` y cuyo ánimo dice
`HAPPY`, y verifica que se respeta el ánimo.

### La OLED monocroma

**Antes:** OLED 0,91" 128×32 mostrando sólo los ojos. Compartía el bus I2C de
los sensores —cero pines nuevos— y consumía la sexta parte.

**Qué la tumbó:** el precio de la TFT de 1,44" bajó a ARS 9.160, apenas 1.660
más, y a esa diferencia el argumento económico desaparece. Y una vez que se
decide mostrar una cara entera y no dos ojos, el color deja de ser un lujo:
seis modelos monocromos serían seis siluetas parecidas.

**Qué se perdió:** la autonomía baja de unos 750 días estimados a unos 595, y
la SPI se lleva cinco GPIO contra cero de la OLED. Ese consumo de pines es
exactamente el motivo por el que hoy la carcasa no se detecta sola.

### El lienzo lógico de 160×240

**Antes:** se rasterizaba a 160×240 y se escalaba 2× al presentar, porque el
panel de la Terminal era de 320×480 y sus 300 KB no entraban en la SRAM.

**Qué la tumbó:** el cambio de panel. 128×128 son 32 KB y entran de sobra, así
que la capa de conversión de coordenadas dejó de pagar lo que costaba.

### La cañería de sprites

**Antes:** `tools/gen_art.py` generaba los cuerpos, los ojos y las bocas como
datos C indexados por paleta, y `gfx/fb.c` tenía cuatro variantes de blit para
dibujarlos.

**Qué la tumbó:** la cara procedural no usa un solo sprite. Quedaron 21 KB de
arte generado y unas 200 líneas de blit sin un solo llamador, así que se
borraron. Está anotado porque la tentación de dejar código muerto "por si
vuelve" es exactamente cómo un repo se pudre.
