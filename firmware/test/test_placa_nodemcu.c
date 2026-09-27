/* El prototipo soldado de Rocío (NodeMCU ESP32 de 38 pines).
 *
 * Lo que se verifica acá es que placa.h diga exactamente lo que está
 * soldado: si un número no coincide con el cableado, el firmware lee el
 * sensor equivocado o la pantalla queda en blanco, y eso en la mesa parece
 * un sensor roto. El mapeo es el que mandó con la placa. Aparte, por la misma
 * razón que test_placa_devkit.c: placa.h se incluye una vez por archivo.
 */
#include <stdbool.h>
#include "rk_test.h"

#define RK_PLACA_NODEMCU38
#define RK_PANEL_ILI9341_240
#define VSPI_HOST 3
#include "../esp32/placa.h"

void nodemcu_verificar(void);

void nodemcu_verificar(void)
{
    /* La pantalla, como está soldada. */
    CHECK_INT("TFT CS en GPIO5", 5, RK_PIN_TFT_CS);
    CHECK_INT("TFT RESET en GPIO4", 4, RK_PIN_TFT_RST);
    CHECK_INT("TFT DC en GPIO2", 2, RK_PIN_TFT_DC);
    CHECK_INT("TFT MOSI en GPIO23", 23, RK_PIN_MOSI);
    CHECK_INT("TFT SCK en GPIO18", 18, RK_PIN_SCK);
    CHECK_INT("TFT MISO en GPIO19", 19, RK_PIN_MISO);
    CHECK_INT("la luz del panel va a 3,3 V, no a un GPIO", -1, RK_PIN_TFT_BL);
    CHECK_INT("es el panel de 240 de ancho", 240, RK_TFT_W);
    CHECK_INT("y 320 de alto", 320, RK_TFT_H);

    /* Los sensores. */
    CHECK_INT("suelo en GPIO34", 34, RK_PIN_SUELO_ADC);
    CHECK_INT("LDR analogica en GPIO35", 35, RK_PIN_LUZ_ADC);
    CHECK_INT("LDR digital en GPIO32", 32, RK_PIN_LUZ_DIG);
    CHECK_INT("HTU21D SDA en GPIO21", 21, RK_PIN_SDA);
    CHECK_INT("HTU21D SCL en GPIO22", 22, RK_PIN_SCL);
    CHECK_INT("el HTU21D es de la familia SHT21, en 0x40", 0x40, RK_I2C_SHT21);

    /* Las trampas del ESP32 clásico. */
    CHECK_TRUE("los dos analogicos estan en el ADC1, que convive con el wifi",
               RK_PIN_SUELO_ADC >= 32 && RK_PIN_SUELO_ADC <= 39 &&
               RK_PIN_LUZ_ADC >= 32 && RK_PIN_LUZ_ADC <= 39);
    CHECK_TRUE("suelo y luz no comparten pin", RK_PIN_SUELO_ADC != RK_PIN_LUZ_ADC);
    CHECK_TRUE("el SPI de la pantalla va por los pines nativos del VSPI",
               RK_PIN_SCK == 18 && RK_PIN_MOSI == 23 && RK_PIN_TFT_CS == 5);
    {
        static const int usados[] = { RK_PIN_TFT_CS, RK_PIN_TFT_RST, RK_PIN_TFT_DC,
                                      RK_PIN_MOSI, RK_PIN_SCK, RK_PIN_MISO,
                                      RK_PIN_SUELO_ADC, RK_PIN_LUZ_ADC, RK_PIN_LUZ_DIG,
                                      RK_PIN_SDA, RK_PIN_SCL, RK_PIN_BOTON };
        const int n = (int)(sizeof usados / sizeof usados[0]);
        int i, j, repetidos = 0;
        for (i = 0; i < n; i++) {
            for (j = i + 1; j < n; j++) {
                if (usados[i] == usados[j]) {
                    repetidos++;
                }
            }
        }
        CHECK_INT("ningun GPIO hace dos cosas", 0, repetidos);
    }
    CHECK_INT("el boton es el BOOT de la placa", 0, RK_PIN_BOTON);
    CHECK_TRUE("lo que no esta soldado queda en -1",
               RK_PIN_TOQUE < 0 && RK_PIN_SENSORES_EN < 0 && RK_PIN_RIEL_ADC < 0 &&
               RK_PIN_UNOWIRE < 0);
    CHECK_STR("la placa del prototipo", "nodemcu-38", RK_PLACA_NOMBRE);
}
