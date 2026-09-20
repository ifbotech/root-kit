/* El banco de pruebas (ESP32 DevKit de 30 pines).
 *
 * No tiene sustrato —se arma en protoboard— pero tiene las mismas trampas:
 * el ADC2 no funciona con el wifi prendido y el despertar por ext0 necesita
 * un GPIO del dominio RTC. Este archivo existe aparte porque placa.h sólo se
 * puede incluir una vez por unidad de compilación, y la suite del producto ya
 * la incluyó para el C3.
 */
#include <stdbool.h>
#include "rk_test.h"

#define RK_PLACA_DEVKIT
#define RK_PANEL_ST7735_128
#define VSPI_HOST 3
/* Lo mismo que le pasa platformio.ini al entorno devkit-144. */
#define RK_BANCO 1
#include "../esp32/placa.h"

void devkit_verificar(void);

void devkit_verificar(void)
{
    /* ADC1 del ESP32 clasico: GPIO32–GPIO39. El ADC2 lo toma el wifi. */
    CHECK_TRUE("el suelo del banco esta en el ADC1",
               RK_PIN_SUELO_ADC >= 32 && RK_PIN_SUELO_ADC <= 39);
    CHECK_TRUE("el riel del banco esta en el ADC1",
               RK_PIN_RIEL_ADC >= 32 && RK_PIN_RIEL_ADC <= 39);
    CHECK_TRUE("suelo y riel del banco no comparten pin",
               RK_PIN_SUELO_ADC != RK_PIN_RIEL_ADC);

    /* GPIO34–GPIO39 son sólo entrada: no pueden manejar un MOSFET. */
    CHECK_TRUE("la compuerta de sensores del banco es un pin de salida",
               RK_PIN_SENSORES_EN < 34);

    /* El toque despierta por ext0: tiene que ser un GPIO del dominio RTC. */
    {
        static const int rtc[] = { 0, 2, 4, 12, 13, 14, 15, 25, 26, 27,
                                   32, 33, 34, 35, 36, 37, 38, 39 };
        int ok = 0;
        for (unsigned i = 0; i < sizeof rtc / sizeof rtc[0]; i++) {
            if (rtc[i] == RK_PIN_TOQUE) {
                ok = 1;
            }
        }
        CHECK_TRUE("el toque del banco despierta por ext0", ok);
    }

    /* El reset del panel sí existe en el banco: sobran pines. */
    CHECK_TRUE("el banco cablea el reset del panel", RK_PIN_TFT_RST >= 0);
    CHECK_STR("la placa del banco", "esp32-devkit", RK_PLACA_NOMBRE);

    /* El banco habla con root-lab en la PC; el producto, sólo con la nube.
     * platformio.ini le pasa -DRK_BANCO=1 y placa.h lo traduce. */
    CHECK_TRUE("con -DRK_BANCO=1 la placa queda de banco", RK_ES_BANCO);
}
