/* placa.h — los pines de cada placa y el panel de cada variante.
 *
 * Dos placas soportadas, elegidas en platformio.ini:
 *
 *   RK_PLACA_C3       ESP32-C3 SuperMini   (la del producto)
 *   RK_PLACA_DEVKIT   ESP32 DevKit 30 pines (la del banco de pruebas)
 *
 * y un panel:
 *
 *   RK_PANEL_ST7735_128     TFT 1,44" IPS 128x128, el de la ventana biselada
 *                           de las carcasas (área activa 25,9 x 25,9 mm)
 *   RK_PANEL_ILI9341_240    TFT 2,2" 240x320, el de la tira de 9 pines
 *
 * El de 2,2" fue el del primer prototipo, se retiró en la 0.6.0 y volvió en
 * la 0.6.2 como VARIANTE DE BANCO, no como producto: es el panel que entra
 * en la tira de nueve pines del sustrato v3.0 (docs/pcb.md), y es el que hay
 * sobre la mesa. El producto sigue siendo el de 1,44" mientras las carcasas
 * sean para él. El motor gráfico escala solo al lado corto del panel, así
 * que la cara se dibuja igual en los dos.
 *
 * Por qué cada pin está donde está, y el esquema de conexión completo, en
 * docs/hardware.md. Lo que NO conviene mover sin leer eso:
 *
 *   - En el C3, sólo GPIO0-GPIO5 despiertan del deep sleep: el toque va ahí.
 *   - En el C3, GPIO2, GPIO8 y GPIO9 son de arranque (strapping). Se usan
 *     para cosas que están en alto al encender: el MOSFET de los sensores
 *     (apagado = alto), la línea 1-Wire (pull-up) y el botón BOOT.
 *   - En el ESP32 clásico, el ADC2 no funciona con el wifi prendido: suelo
 *     y riel van al ADC1 (GPIO32-39).
 */
#ifndef ROOTKIT_PLACA_H
#define ROOTKIT_PLACA_H

/* Placa de BANCO (RK_BANCO=1, la del DevKit): la nube se puede cambiar desde
 * el portal y puede ser http://, para hablar con root-lab en la PC. El
 * producto no: habla sólo con RK_NUBE_URL y sólo por HTTPS, porque el portal
 * de configuración es una red abierta y cualquiera que esté cerca en ese
 * momento podría mandar al aparato —y a su token— a otro servidor. */
#if defined(RK_BANCO) && RK_BANCO
  #define RK_ES_BANCO true
#else
  #define RK_ES_BANCO false
#endif

#if defined(RK_PLACA_C3)
  #define RK_PLACA_NOMBRE      "c3-supermini"
  #define RK_PIN_SUELO_ADC     0    /* ADC1_CH0: salida del capacitivo        */
  #define RK_PIN_RIEL_ADC      1    /* ADC1_CH1: divisor 470k/470k del riel   */
  #define RK_PIN_SENSORES_EN   2    /* gate del P-MOSFET: LOW enciende        */
  #define RK_PIN_TOQUE         3    /* TTP223: alto al tocar, despierta       */
  #define RK_PIN_SDA           4
  #define RK_PIN_SCL           5
  #define RK_PIN_SCK           6
  #define RK_PIN_MOSI          7
  #define RK_PIN_UNOWIRE       8    /* DS18B20 con pull-up 4k7                */
  #define RK_PIN_BOTON         9    /* BOOT: apretado = LOW                   */
  #define RK_PIN_TFT_DC        10
  #define RK_PIN_TFT_CS        20
  #define RK_PIN_TFT_BL        21   /* a un MOSFET/transistor, no directo     */
  #define RK_PIN_TFT_RST       -1   /* reset por software                     */
  #define RK_SPI_HOST          SPI2_HOST
#elif defined(RK_PLACA_DEVKIT)
  #define RK_PLACA_NOMBRE      "esp32-devkit"
  #define RK_PIN_SUELO_ADC     34
  #define RK_PIN_RIEL_ADC      35
  #define RK_PIN_SENSORES_EN   26
  #define RK_PIN_TOQUE         33   /* RTC GPIO: despierta por ext0           */
  #define RK_PIN_SDA           21
  #define RK_PIN_SCL           22
  #define RK_PIN_SCK           18
  #define RK_PIN_MOSI          23
  #define RK_PIN_UNOWIRE       25
  #define RK_PIN_BOTON         0
  #define RK_PIN_TFT_DC        16
  #define RK_PIN_TFT_CS        5
  #define RK_PIN_TFT_BL        4
  #define RK_PIN_TFT_RST       17
  #define RK_SPI_HOST          VSPI_HOST
#else
  #error "Definir RK_PLACA_C3 o RK_PLACA_DEVKIT (ver platformio.ini)"
#endif

#if defined(RK_PANEL_ST7735_128)
  #define RK_PANTALLA_NOMBRE   "st7735-128"
  #define RK_TFT_W             128
  #define RK_TFT_H             128
  /* El 1,44" usa la memoria de 132x162 del controlador. */
  #define RK_TFT_MEM_W         132
  #define RK_TFT_MEM_H         162
#elif defined(RK_PANEL_ILI9341_240)
  #define RK_PANTALLA_NOMBRE   "ili9341-240"
  #define RK_TFT_W             240
  #define RK_TFT_H             320
  #define RK_TFT_MEM_W         240
  #define RK_TFT_MEM_H         320
#else
  #error "Definir RK_PANEL_ST7735_128 o RK_PANEL_ILI9341_240 (ver platformio.ini)"
#endif

/* Ajustes finos del panel que cambian entre lotes del mismo modelo. Si la
 * imagen sale corrida, con los colores invertidos o con rojo y azul
 * cambiados, se corrigen desde platformio.ini sin tocar código. */
#ifndef RK_TFT_OFS_X
  #define RK_TFT_OFS_X 0
#endif
#ifndef RK_TFT_OFS_Y
  #define RK_TFT_OFS_Y 0
#endif
#ifndef RK_TFT_INVERT
  #define RK_TFT_INVERT 0
#endif
#ifndef RK_TFT_BGR
  #define RK_TFT_BGR 0
#endif

/* Polaridad de la luz de fondo. Por defecto 0: el GPIO en alto enciende, que
 * es lo que hace la etapa del sustrato (N-MOSFET que tira de la compuerta de
 * un P-MOSFET del lado alto, docs/pcb.md). Si el modulo que llegue resulta
 * tener el pin BL activo en bajo, se pone 1 desde platformio.ini y no se toca
 * una linea de codigo. */
#ifndef RK_TFT_BL_INVERTIDO
  #define RK_TFT_BL_INVERTIDO 0
#endif

/* Velocidad de escritura del bus SPI de la pantalla. 40 MHz andan con las
 * pistas cortas del sustrato; si la imagen sale con basura, se baja desde
 * platformio.ini antes de tocar el hardware (docs/pcb.md, "El SPI"). */
#ifndef RK_TFT_SPI_HZ
  #define RK_TFT_SPI_HZ 40000000
#endif

/* Divisor del riel: dos resistencias iguales de 470k. Altas para que el
 * divisor gaste 4 uA y no 40; con 100 nF en el nodo el ADC lee estable. */
#define RK_RIEL_R_ARRIBA  470000u
#define RK_RIEL_R_ABAJO   470000u

#define RK_I2C_AHT20   0x38
#define RK_I2C_SHT21   0x40
#define RK_I2C_BH1750  0x23

/* Cuál de los dos sensores de aire hay en la placa. Los dos entran en la
 * misma tira de cuatro pines del sustrato (VIN GND SCL SDA), así que el
 * hardware no elige: elige esto. 0 = AHT20 (0x38), 1 = SHT21/HTU21/Si7021
 * (0x40). Se pone desde platformio.ini. */
#ifndef RK_AIRE_SHT21
  #define RK_AIRE_SHT21 0
#endif

#endif /* ROOTKIT_PLACA_H */
