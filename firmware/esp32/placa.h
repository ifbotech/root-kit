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
 *
 * El TFT de 2,2" (ILI9341, 240x320) fue el del primer prototipo y se retiró:
 * las carcasas son para el de 1,44", y mantener dos paneles duplicaba
 * compilaciones y pruebas sin producto detrás. Volver a sumar un panel es
 * agregar su bloque acá y su clase en pantalla.cpp (docs/decisiones.md).
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
#else
  #error "Definir RK_PANEL_ST7735_128 (ver platformio.ini)"
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

/* Divisor del riel: dos resistencias iguales de 470k. Altas para que el
 * divisor gaste 4 uA y no 40; con 100 nF en el nodo el ADC lee estable. */
#define RK_RIEL_R_ARRIBA  470000u
#define RK_RIEL_R_ABAJO   470000u

#define RK_I2C_AHT20   0x38
#define RK_I2C_BH1750  0x23

#endif /* ROOTKIT_PLACA_H */
