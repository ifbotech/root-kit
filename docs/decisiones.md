# Registro de decisiones

Las decisiones cerradas y por qué, para no volver a discutirlas.
El análisis competitivo completo está en el dossier.

Las decisiones que se **revisaron** quedan acá abajo, en su propia sección, con
lo que las tumbó. Borrarlas sería perder la parte más útil del registro: saber
por qué algo que parecía correcto dejó de serlo.

## Arquitectura

**Un Prime y hasta cinco Minis.** Cada uno es una maceta con sensores y
pantalla. El Prime va enchufado, tiene la pantalla grande y sirve el Hub; los
Minis van a batería.

**Se descartó "solo Minis, sin Prime".** Es más barato y es lo que hace Senso,
pero deja al producto sin el lugar donde el simbionte es grande. Y se descartó
"todos los nodos con TFT de 2,2 pulgadas a batería", porque una pantalla de ese
tamaño encendida a batería dura días, no meses.

**El mismo simbionte se ve adulto en el Prime y brote en el Mini.** Ésta es la
decisión de producto, no una consecuencia técnica. Le da al Prime una razón que
no es "una pantalla más grande" —es donde tus criaturas están grandes— y a cada
Mini una razón que no es "un sensor más". El tope de cinco Minis sale de que
las fichas del selector del Prime no bajen de 38 px de ancho, que es el mínimo
cómodo para un toque en un panel de 36,5 mm.

## Hardware

**Prime: TFT 2,2" 240×320, ILI9341, SPI.** Área activa 36,5 × 47,5 mm, paso de
pixel 0,152 mm: el adulto a 2× mide 21,9 mm, que es tamaño de personaje y no de
icono. ARS 18.900. Cinco GPIO para la pantalla, que es lo que permite que
entren los sensores en un C3.

**Mini: TFT 1,44" 128×128 IPS, ST7735, SPI.** Área activa 25,9 × 25,9 mm, paso
0,202 mm: el brote a 2× mide 12,9 mm. ARS 9.160. Es IPS, tiene el pin de
retroiluminación accesible para PWM, y su controlador tiene librería madura.

**Descartada la shield de 2,4" para Arduino UNO (ARS 13.069).** Es paralela de
8 bits: trece GPIO sólo para la pantalla, contra cinco de la SPI. Un C3
SuperMini tiene diez pines útiles. Además es de 5 V —lleva conversores de nivel
en la placa— y el controlador es una lotería: estas shields MCUFRIEND vienen con
ILI9341, ILI9325, R61505 o S6D0154 según el día. Para un proyecto suelto es un
detalle; para cien unidades vendidas es descalificante. Y no gana nada a cambio:
su paso de pixel de 0,158 mm da una criatura de 22,8 mm, prácticamente la misma.

**Descartada la Guition JC3248W535 como Terminal.** Era la elección anterior
—ESP32-S3, 3,5" 320×480 IPS, táctil capacitivo— y quedó afuera cuando el
producto pasó a Prime + Minis. Cuesta ARS 69.103 contra los 49.100 de un Prime
completo, y su paso de pixel es casi idéntico al de la 2,2": el doble de
pixeles en un área apenas mayor da exactamente el mismo tamaño de criatura.

**Descartada la CYD de 2.8" (ESP32-2432S028R).** Táctil resistivo — por eso
viene con stylus. Rompe el mecanismo central del producto: no se puede
acariciar al simbionte, se lo aprieta. No es cuestión de precio.

**Descartada la Waveshare ESP32-S3-RLCD-4.2.** Panel reflectivo sin
retroiluminación, descrito por el fabricante como "experiencia tipo papel
electrónico": es la misma renuncia que hizo PlantSenso con su E-Ink, justo en el
terreno donde ROOTKIT gana.

**El Prime va enchufado, sin batería.** Con alimentación fija la pantalla puede
quedar siempre encendida — un simbionte que hay que despertar tocando es peor
mascota que uno que está siempre vivo. Ivy hace exactamente esto y tiene 200.000
clientes.

**Sensores iguales en los dos: capacitivo v2.0 + AHT21 + BH1750.** Un nodo con
menos sensores sería un nodo que no puede evaluarse solo, y eso rompe la regla
central del sistema.

**El AHT21 va en cada nodo.** Se había propuesto uno solo para todo el kit, pero
una maceta interior y una de balcón son microclimas distintos y cada nodo tiene
que poder distinguirlos.

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

**Protocolo binario, no JSON.** Una trama de telemetría son 26 bytes; el mismo
contenido en JSON son unos 190. Cada byte es tiempo de radio encendida, y la
radio es cerca de la mitad del presupuesto energético de un Mini.

**La configuración lleva los umbrales de la especie.** Son 32 bytes contra los
18 de antes, y es lo que permite que el Mini evalúe su propia maceta. No pesa en
la batería: viaja una sola vez al emparejar y cada vez que cambia la especie, en
sentido Prime → Mini, sobre un nodo que ya está despierto.

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
corrupta que pase por buena mueve al simbionte a un estado equivocado con total
convicción. Un test verifica que todos los flips de un bit se detecten.

**Medir y transmitir van desacoplados.** Medir cuesta 250 nAh y transmitir
28.000: 110 veces más. Se mide seguido y se emite sólo ante un cambio, un cruce
de umbral, un cambio de estado de batería o el latido de dos horas. Una semana
simulada da 68% menos de radio que un intervalo fijo de 15 minutos.

## Arte y colección

**Una silueta, doce paletas, doce copetes.** El adulto comparte geometría entre
los doce simbiontes; se distinguen por su familia cromática. Los brotes además
tienen copete propio —hojas, espinas, flor, esporas, rizo— porque a 32×32 el
copete es casi un tercio de la silueta y es ahí donde rinde la diferenciación.

Es un compromiso explícito, no un final: el adulto comparte silueta **hasta que
cada uno tenga su cuerpo propio**. La cañería ya lo soporta —agregar un cuerpo
es agregar una función y una fila en la tabla— y hay tests que verifican que las
doce paletas y los doce brotes sean distinguibles entre sí.

**El brote invierte las proporciones del adulto, no lo reduce.** Donde el adulto
es caparazón con una cabeza asomando, el brote es cabeza con un caparazón
asomando. Cabeza grande, ojos grandes y bajos, cuerpo chico: el esquema
infantil. Un adulto reducido al 33% se lee como "el mismo dibujo, más chico"; la
inversión se lee como "la cría del mismo bicho".

**El crecimiento se muestra alrededor del brote, no en su tamaño.** Doce
simbiontes por cinco etapas serían sesenta sprites, y encima a 32×32 un cuerpo
15% más grande no se nota. En cambio le van saliendo hojas —una por etapa, hasta
cuatro— y en las dos últimas se enciende un aura de su color de acento. Eso sí
se ve desde el otro lado de la habitación, que es el único lugar desde el que
alguien mira una maceta. La etapa ESPORA además lo muestra dentro de un
cascarón con sólo los ojos asomando: da algo que esperar los primeros siete
días.

**Qué cara pone cada ánimo vive una sola vez, en `art/look.c`.** El adulto y el
brote tienen arte distinto pero comparten la tabla de carácter. Si viviera
duplicada, el día que alguien tocara una el Mini y el Prime empezarían a decir
cosas distintas sobre la misma planta.

**La rareza es mérito, no suerte.** Sale de la dificultad hortícola de la
especie: un potus da un común porque perdona todo, un bonsái da un legendario
porque mantenerlo vivo es trabajo real. Así la rareza significa algo, y de paso
el producto queda afuera del terreno de las cajas de botín, que Bélgica y Países
Bajos ya restringen.

**La ceremonia de apertura se conserva entera, y pasa siempre en el Prime.**
Cápsula que cae, tiembla, se raja y estalla con destellos graduados por rareza.
Registrar la planta de un Mini también abre la cápsula en el Prime: es el altar
del kit. Lo único que no hay es azar en el resultado.

**El simbionte crece con días sanos, no con días transcurridos.** Una planta
abandonada tiene un simbionte que no evoluciona, y ahí está toda la mecánica de
vínculo. Pero un mal día corta la racha sin borrar lo acumulado: castigar un
descuido con meses de progreso convierte un olvido en motivo para abandonar el
producto.

**Un nodo caído no acumula días sanos.** Si no sabemos cómo estuvo la planta, no
se premia. Es lo que impide que desenchufar un Mini haga crecer al simbionte
gratis.

**El catálogo del Hub se genera desde el del firmware.** `tools/sync_catalog.py`
lo deriva de `species.c` y `companion.c`, y CI falla si quedó desfasado. Dos
copias a mano se desincronizan, y acá una desincronización significa ofrecerle
al usuario una especie que el kit no sabe evaluar.

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

**Resolución nativa, sin lienzo lógico.** 240×320 son 150 KB y 128×128 son 32
KB: los dos entran en los 400 KB de SRAM de un ESP32-C3 sin tocar PSRAM. Se
dibuja directo y desaparece una capa entera de conversión de coordenadas. El
arte se sigue escalando por enteros, que es lo que importaba.

**El texto del Prime arranca en escala 2.** Al dibujar nativo, un glifo de 5×7 a
escala 1 mide 0,76 × 1,06 mm: ilegible. Es la consecuencia no obvia de haber
sacado el lienzo lógico, y está codificada en `gfx/panel.h` para que ninguna
pantalla la olvide.

**El Mini no muestra números.** Un "34%" en 1 mm de alto no se lee, y si se
leyera no significaría nada sin el rango de la especie al lado. Tres barras con
la zona cómoda marcada dicen lo mismo en un tercio del espacio. El número exacto
está en el Prime y en el Hub, que es donde alguien lo va a ir a buscar.

**Renderer propio en vez de LVGL.** El pixel art necesita escalado por enteros,
no el suavizado que LVGL asume; un buffer RGB565 plano es exactamente lo que
espera `esp_lcd_panel_draw_bitmap()`; y hay dos paneles de tamaños muy distintos
donde LVGL pesaría lo mismo.

**El presupuesto energético vive en código testeado, no en una planilla.** Las
cifras de autonomía salen de correr `make test`, así que si alguien cambia un
parámetro el número se rompe en vez de quedar viejo en silencio. Los costos por
evento van en nanoamperios-hora: en microamperios-hora enteros, una medición de
0,25 µAh se redondea a cero y desaparece del modelo.

**Regresión visual por hash, una tabla por panel.** `firmware/test/golden.h`
guarda un FNV-1a del framebuffer de cada pantalla para cada estado de ánimo. Se
versiona en vez de ignorarse porque el diff del archivo generado *es* la revisión
del cambio visual, y están separados por panel para que el diff diga además
*dónde* cambió.

**El Hub es una vista, no una aplicación.** La foto obliga a tener el celular,
pero no una app nativa: una página web abre la cámara con `<input capture>`. Sin
App Store no hay cuota anual ni nadie revisando las mecánicas de colección
contra las políticas de cajas de botín. El Prime sirve la PWA por HTTP en la red
local, porque una página HTTPS tiene prohibido pedirle datos a una IP privada.

## Producto

**Sin pH.** Ni PlantSenso, ni Ivy, ni Mi Flora miden pH real de suelo: es
genuinamente difícil y todos los aparatos de consumo que muestran el número lo
están inventando. No mostrarlo es honesto y además es un argumento de venta para
el público al que apunta ROOTKIT.

**Desbloqueos deterministas, no aleatorios.** Registrás una especie nueva y
aparece su simbionte; mantenés una planta viva 90 días y evoluciona. La sensación
de colección se conserva y desaparece el riesgo regulatorio de las cajas de
botín.

**Nada de la palabra Tamagotchi en el marketing.** Es marca registrada de Bandai.
El vocabulario propio — simbionte, criatura digital — es más distintivo igual.

---

## Decisiones revisadas

### El nodo dejó de ser tonto

**Antes:** el Spore medía, empaquetaba y dormía. No conocía especies ni
umbrales, y toda la interpretación vivía en la Terminal. Tres razones lo
sostenían: batería, poder corregir los rangos de una especie en un solo lugar, y
que la máquina de estados existiera una sola vez.

**Qué la tumbó:** ponerle pantalla a cada nodo. Un nodo que necesita la red para
saber qué cara poner se queda mudo justo cuando más importa —cuando el Prime
está apagado, reiniciándose o fuera de alcance.

**Cómo se respetan las tres razones originales.** La batería: evaluar el ánimo
son unas pocas comparaciones enteras sobre datos que el nodo ya tiene en RAM, y
no agrega un solo despertar. Las actualizaciones: los umbrales no están
compilados en el Mini, viajan en `CONFIG`. La implementación única: `core/mood.c`
sigue existiendo una sola vez, compilado dos veces.

**Y la regla nueva que hizo falta agregar:** el Prime **no recalcula** lo que
recibe. Si lo hiciera, su histéresis acumulada sería distinta de la del Mini y
las dos pantallas discreparían. Hay un test que manda una telemetría cuyos
números gritarían `THIRSTY` y cuyo ánimo dice `HAPPY`, y verifica que el Prime
respeta el ánimo.

### La OLED monocroma dejó paso a una TFT a color

**Antes:** OLED 0,91" 128×32 SSD1306 en cada Spore, mostrando sólo los ojos del
simbionte. Compartía el bus I2C de los sensores —cero pines nuevos—, usaba 512
bytes de framebuffer contra 115.200, y consumía la sexta parte. Había además un
argumento estético: monocromo a 128×32 *es* la estética Tamagotchi original.

**Qué la tumbó:** dos cosas a la vez. La primera, el precio: la TFT de 1,44"
apareció a ARS 9.160, apenas 1.660 más que la OLED, y a esa diferencia el
argumento económico desaparece. La segunda, y más importante: **una vez que se
decide mostrar el cuerpo del bicho y no sólo sus ojos, el color deja de ser un
lujo.** Doce simbiontes monocromos de 32×32 son doce siluetas parecidas; con
paleta propia son doce criaturas.

**Qué se perdió, y qué se hizo al respecto.** La autonomía: el Mini pasa de unos
750 días estimados a unos 595. Sigue siendo año y medio. Y se perdió el
argumento de los cero pines nuevos: la SPI se lleva cinco GPIO, que es
exactamente el motivo por el que se descartó la shield paralela de 2,4".

**Lo que sobrevivió intacto** es la jerarquía: sigue habiendo una diferencia
clara entre la presencia del bicho en la maceta y su mundo en el Prime. Sólo que
ahora esa diferencia es de tamaño y de detalle —13 mm contra 22 mm, brote contra
adulto— en vez de ser de color.

### El lienzo lógico de 160×240

**Antes:** se rasterizaba a 160×240 y se escalaba 2× al presentar, porque el
panel de la Terminal era de 320×480 y sus 300 KB de framebuffer no entraban en
la SRAM interna del S3.

**Qué la tumbó:** el cambio de panel. 240×320 son 150 KB y 128×128 son 32 KB.
Los dos entran, así que la capa de conversión de coordenadas dejó de pagar lo
que costaba.
