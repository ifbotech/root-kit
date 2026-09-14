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
doble, así que los pixeles quedan nítidos sin interpolación.

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
