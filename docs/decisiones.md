# Registro de decisiones

Las decisiones cerradas y por qué, para no volver a discutirlas.
El análisis competitivo completo está en el dossier.

## Hardware

**Terminal: Guition JC3248W535.** ESP32-S3 con 16 MB de flash y 8 MB de PSRAM,
3,5" 320×480 IPS, AXS15231B por QSPI, táctil capacitivo. USD 11–18 importada,
que en Argentina sale lo mismo que la CYD comprada localmente. Se eligió por
precio, por potencia gráfica y sobre todo por soporte oficial en ESPHome y
comunidad activa, contra las dos placas Waveshare cuyas wikis están vacías.

**Descartada la CYD de 2.8" (ESP32-2432S028R).** Táctil resistivo — por eso
viene con stylus. Rompe el mecanismo central del producto: no se puede
acariciar al simbionte, se lo aprieta. No es cuestión de precio.

**Descartada la Waveshare ESP32-S3-RLCD-4.2.** Panel reflectivo sin
retroiluminación, descrito por el fabricante como "experiencia tipo papel
electrónico": es la misma renuncia que hizo PlantSenso con su E-Ink,
justo en el terreno donde ROOTKIT gana.

**La Terminal va enchufada, sin batería.** Con alimentación fija la pantalla
puede quedar siempre encendida — un simbionte que hay que despertar tocando es
peor mascota que uno que está siempre vivo — y el refresco total obligatorio
del AXS15231B deja de importar. Ivy hace exactamente esto y tiene 200.000
clientes.

**Spore: ESP32-C3 + capacitivo v2.0 + AHT21 + BH1750 + 18650.**

**El AHT21 se queda en el Spore.** Se había propuesto moverlo a la Terminal
para ahorrar, pero una maceta interior y una de balcón son microclimas
distintos y el Spore tiene que poder distinguirlos. Con clima por maceta,
el AHT21 de la Terminal pasa a ser opcional.

**Batería 18650, no LiPo.** A precios de MercadoLibre: 18650 de 2200 mAh con
portapilas sale unos ARS 5.000 comprando de a diez — ARS 2,27 por mAh — contra
ARS 17 a 23 por mAh de cualquier LiPo chica. Es más barata en términos
absolutos que la LiPo de 300 mAh y da siete veces más capacidad. Además, si la
celda es reemplazable por el usuario desaparecen el TP4056 y el conector USB, y
con ellos un agujero menos que sellar en un aparato que vive en tierra húmeda.

**Prohibido el FC-28 / YL-69.** Resistivo: se consume por electrólisis en 3 a 6
meses, y mide sales disueltas en vez de agua, así que fertilizar altera la
lectura. El propio aviso admite las dos cosas.

## Comunicación

**Protocolo binario, no JSON.** Una trama de telemetría son 24 bytes; el mismo
contenido en JSON son unos 180. Cada byte es tiempo de radio encendida, y la
radio es cerca de la mitad del presupuesto energético del Spore.

**La iluminancia viaja con mantisa y exponente.** Hay que cubrir de 1 a 100.000
lux. En 16 bits lineales habría que sacrificar la resolución baja, que es justo
donde vive el umbral de noche.

**CRC16-CCITT sobre toda la trama.** En 2,4 GHz con vecinos ruidosos, una trama
corrupta que pase por buena mueve al simbionte a un estado equivocado con total
convicción. Un test verifica que los 192 flips de un bit se detecten.

**El Spore es deliberadamente tonto.** Mide, empaqueta y duerme; no conoce
especies ni umbrales. Por batería, porque corregir los rangos de una especie
para todos es tocar la Terminal y no seis nodos enterrados, y porque la lógica
de ánimo tiene que existir en un solo lugar.

**Medir y transmitir van desacoplados.** Medir cuesta 250 nAh y transmitir
28.000: 110 veces más. Se mide seguido y se emite sólo ante un cambio, un cruce
de umbral, un cambio de estado de batería o el latido de dos horas. Una semana
simulada da 68% menos de radio que un intervalo fijo de 15 minutos.

## Software

**El núcleo es C99 puro.** `firmware/core/` no depende de LVGL ni de ESP-IDF y
no usa punto flotante. Se compila igual en el simulador, en los tests y en la
Terminal. La temperatura viaja en décimas de grado en un `int16_t`.

**Prioridad de estados de ánimo: agua, temperatura, luz, aire.** Se muestra la
necesidad insatisfecha más urgente, no un promedio. El agua va primero porque
es lo que mata más rápido.

**Histéresis en todos los umbrales.** Sin banda de salida, el simbionte titila
entre feliz y sediento cada vez que la lectura oscila un punto sobre el límite.

**De noche el simbionte duerme.** Tras ocho muestras consecutivas por debajo de
15 lux se suprimen las quejas por luz y por aire seco, pero no las de agua ni
las de temperatura.

**Arte a 160 × 240, escalado 2× con enteros.** La pantalla es exactamente el
doble, así que los pixeles quedan nítidos sin interpolación. Y hay una razón de
performance además de la estética: el framebuffer lógico son 75 KB y entran en
la SRAM interna del S3. Rasterizar a 320×480 serían 300 KB y habría que ir a
PSRAM, que es un orden de magnitud más lenta.

**Renderer propio en vez de LVGL.** El AXS15231B no soporta refresco parcial,
así que la optimización principal de LVGL no aplica; un buffer RGB565 plano es
exactamente lo que espera `esp_lcd_panel_draw_bitmap()`; y el pixel art
necesita escalado por enteros, no el suavizado que LVGL asume.

**El presupuesto energético vive en código testeado, no en una planilla.** Las
cifras de autonomía salen de correr `make firmware`, así que si alguien cambia
un parámetro del firmware el número se rompe en vez de quedar viejo en silencio.
Los costos por evento van en nanoamperios-hora: en microamperios-hora enteros,
una medición de 0,25 µAh se redondea a cero y desaparece del modelo.

**Regresión visual por hash.** `firmware/test/golden.h` guarda un FNV-1a del
framebuffer por cada estado de ánimo. Se versiona en vez de ignorarse porque el
diff del archivo generado *es* la revisión del cambio visual.

**El Hub es una vista, no una aplicación.** La foto obliga a tener el celular,
pero no una app nativa: una página web abre la cámara con `<input capture>`.
Sin App Store no hay cuota anual ni nadie revisando las mecánicas de colección
contra las políticas de cajas de botín. La Terminal sirve la PWA por HTTP en la
red local, porque una página HTTPS tiene prohibido pedirle datos a una IP
privada.

## Producto

**Sin pH.** Ni PlantSenso, ni Ivy, ni Mi Flora miden pH real de suelo: es
genuinamente difícil y todos los aparatos de consumo que muestran el número lo
están inventando. No mostrarlo es honesto y además es un argumento de venta
para el público al que apunta ROOTKIT.

**Desbloqueos deterministas, no aleatorios.** Registrás una especie nueva y
aparece su simbionte; mantenés una planta viva 90 días y evoluciona. La
sensación de colección se conserva y desaparece el riesgo regulatorio de las
cajas de botín, que Bélgica y Países Bajos ya restringen.

**Nada de la palabra Tamagotchi en el marketing.** Es marca registrada de
Bandai. El vocabulario propio — simbionte, criatura digital — es más
distintivo igual.
