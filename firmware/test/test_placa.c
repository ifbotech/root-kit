/* El sustrato y el firmware tienen que decir lo mismo.
 *
 * `esp32/placa.h` es la única verdad de los pines. El sustrato impreso
 * (hardware/pcb/nucleo.json) es la única verdad de las redes. Esta suite las
 * cruza: si alguien mueve un pin en un lado y se olvida del otro, el diff no
 * compila en verde y el error aparece acá y no con la placa armada.
 *
 * Lo que se verifica, y por qué cada cosa:
 *
 *   - cada GPIO de placa.h está en la red que le corresponde, y no hay dos
 *     funciones en el mismo pin;
 *   - los trece pines del C3 están todos declarados, incluido el que no va a
 *     ningún lado (GPIO9 es el botón BOOT de la propia SuperMini);
 *   - los dos analógicos caen en el ADC1, que es el único que anda con el
 *     wifi prendido;
 *   - el toque cae en GPIO0–GPIO5, que son los únicos que despiertan del
 *     sueño profundo en el C3;
 *   - los tres pines de arranque (2, 8 y 9) cuelgan de algo que los deja en
 *     alto al encender;
 *   - la línea 1-Wire y su pull-up salen del MISMO riel, que es lo que evita
 *     que la sonda se alimente por la pata de datos con el riel apagado;
 *   - el divisor del riel es el que el firmware cree que es.
 *
 * El banco (ESP32 DevKit) se verifica aparte, en test_placa_devkit.c: no
 * tiene sustrato, pero sí las mismas trampas de ADC y de despertar.
 */
#include <stdbool.h>
#include <string.h>
#include "rk_test.h"
#include "redes.h"

#define RK_PLACA_C3
#define RK_PANEL_ST7735_128
#define SPI2_HOST 1
#include "../esp32/placa.h"

void devkit_verificar(void);   /* test_placa_devkit.c */

static const rk_red_t *red_de(int gpio)
{
    for (int i = 0; i < RK_REDES_N; i++) {
        if (RK_REDES[i].gpio == gpio) {
            return &RK_REDES[i];
        }
    }
    return NULL;
}

/* El pin que placa.h usa para `nombre` tiene que estar en la red `red`. */
static void pin_en_red(const char *nombre, int gpio, const char *red)
{
    const rk_red_t *r = red_de(gpio);
    char etiqueta[96];

    snprintf(etiqueta, sizeof etiqueta, "%s (GPIO%d) esta en la netlist", nombre, gpio);
    CHECK_TRUE(etiqueta, r != NULL);
    if (r == NULL) {
        return;
    }
    snprintf(etiqueta, sizeof etiqueta, "%s (GPIO%d) va a la red %s", nombre, gpio, red);
    CHECK_STR(etiqueta, red, r->red);
}

void suite_placa(void)
{
    RK_SUITE("sustrato y placa.h");

    /* ---- el mapa de pines del producto, uno por uno ---- */
    pin_en_red("capacitivo de suelo", RK_PIN_SUELO_ADC, "SUELO");
    pin_en_red("divisor del riel", RK_PIN_RIEL_ADC, "RIEL");
    pin_en_red("compuerta del riel de sensores", RK_PIN_SENSORES_EN, "SENS_EN");
    pin_en_red("TTP223", RK_PIN_TOQUE, "TOQUE");
    pin_en_red("SDA", RK_PIN_SDA, "SDA");
    pin_en_red("SCL", RK_PIN_SCL, "SCL");
    pin_en_red("SCK de la pantalla", RK_PIN_SCK, "SCK");
    pin_en_red("MOSI de la pantalla", RK_PIN_MOSI, "MOSI");
    pin_en_red("DS18B20", RK_PIN_UNOWIRE, "OW");
    pin_en_red("boton BOOT", RK_PIN_BOTON, "NC_BOOT");
    pin_en_red("DC de la pantalla", RK_PIN_TFT_DC, "TFT_DC");
    pin_en_red("CS de la pantalla", RK_PIN_TFT_CS, "TFT_CS");
    pin_en_red("luz de fondo", RK_PIN_TFT_BL, "BL_G");

    /* El reset del panel es por software: no gasta un GPIO. En el sustrato
     * existe igual, atado a 3V3 con 10 k. */
    CHECK_INT("RES de la pantalla no usa GPIO", -1, RK_PIN_TFT_RST);

    /* ---- la netlist describe los trece pines, y sin repetirse ---- */
    CHECK_INT("el C3 expone trece GPIO y estan todos", 13, RK_REDES_N);
    for (int i = 0; i < RK_REDES_N; i++) {
        for (int j = i + 1; j < RK_REDES_N; j++) {
            char b[64];
            snprintf(b, sizeof b, "GPIO%d aparece una sola vez", RK_REDES[i].gpio);
            CHECK_TRUE(b, RK_REDES[i].gpio != RK_REDES[j].gpio);
            snprintf(b, sizeof b, "la red %s aparece una sola vez", RK_REDES[i].red);
            CHECK_TRUE(b, strcmp(RK_REDES[i].red, RK_REDES[j].red) != 0);
        }
    }

    /* ---- ADC1: el ADC2 no funciona con el wifi prendido ---- */
    for (int i = 0; i < RK_REDES_N; i++) {
        const rk_red_t *r = &RK_REDES[i];
        if (!r->adc1) {
            continue;
        }
        char b[64];
        snprintf(b, sizeof b, "%s entra por un canal del ADC1", r->red);
        /* En el C3 el ADC1 son GPIO0–GPIO4. */
        CHECK_TRUE(b, r->gpio >= 0 && r->gpio <= 4);
    }
    CHECK_TRUE("el suelo es analogico", red_de(RK_PIN_SUELO_ADC)->adc1 == 1);
    CHECK_TRUE("el riel es analogico", red_de(RK_PIN_RIEL_ADC)->adc1 == 1);

    /* ---- despertar: en el C3 solo GPIO0–GPIO5 ---- */
    for (int i = 0; i < RK_REDES_N; i++) {
        const rk_red_t *r = &RK_REDES[i];
        if (!r->despierta) {
            continue;
        }
        char b[64];
        snprintf(b, sizeof b, "%s puede despertar del sueño profundo", r->red);
        CHECK_TRUE(b, r->gpio >= 0 && r->gpio <= 5);
    }
    CHECK_TRUE("el toque despierta", red_de(RK_PIN_TOQUE)->despierta == 1);

    /* ---- pines de arranque: GPIO2, GPIO8 y GPIO9, altos al encender ---- */
    {
        static const int arranque[3] = { 2, 8, 9 };
        for (int i = 0; i < 3; i++) {
            const rk_red_t *r = red_de(arranque[i]);
            char b[80];
            snprintf(b, sizeof b, "GPIO%d es de arranque y esta declarado",
                     arranque[i]);
            CHECK_TRUE(b, r != NULL);
            if (r == NULL) {
                continue;
            }
            snprintf(b, sizeof b, "GPIO%d queda en alto al encender", arranque[i]);
            CHECK_STR(b, "alto", r->arranque);
        }
    }

    /* ---- el 1-Wire y su pull-up, del mismo riel ---- */
    {
        const rk_red_t *ow = red_de(RK_PIN_UNOWIRE);
        CHECK_STR("el 1-Wire cuelga del riel fijo", "3V3", ow->riel);
        /* Si algun dia la sonda vuelve al riel conmutado, su pull-up tiene
         * que irse con ella: el test lo dice antes que el multimetro. */
        CHECK_TRUE("el 1-Wire no cuelga del riel conmutado",
                   strcmp(ow->riel, "3V3S") != 0);
    }

    /* ---- el capacitivo si cuelga del riel conmutado: come 5 mA ---- */
    CHECK_STR("el capacitivo cuelga del riel conmutado", "3V3S",
              red_de(RK_PIN_SUELO_ADC)->riel);

    /* ---- la luz de fondo enciende con el GPIO en alto ---- */
    CHECK_INT("la etapa de la luz no invierte", 1, RK_TFT_BL_ALTO_ENCIENDE);
#ifdef RK_TFT_BL_INVERTIDO
    CHECK_INT("platformio y el sustrato coinciden en la polaridad de la luz",
              RK_TFT_BL_ALTO_ENCIENDE, RK_TFT_BL_INVERTIDO ? 0 : 1);
#endif

    /* ---- el divisor del riel, igual en los dos lados ---- */
    CHECK_INT("rama de arriba del divisor", (long)RK_RIEL_R_ARRIBA_OHM,
              (long)RK_RIEL_R_ARRIBA);
    CHECK_INT("rama de abajo del divisor", (long)RK_RIEL_R_ABAJO_OHM,
              (long)RK_RIEL_R_ABAJO);

    /* ---- el capacitivo va directo al ADC, sin divisor ---- */
    CHECK_INT("la salida del capacitivo no se divide",
              RK_SUELO_DIVISOR_DEN, RK_SUELO_DIVISOR_NUM);

    /* ---- las direcciones de I2C ---- */
    CHECK_HEX("AHT20", 0x38, RK_I2C_AHT20);
    CHECK_HEX("BH1750", 0x23, RK_I2C_BH1750);

    /* ---- el panel del producto ---- */
    CHECK_INT("el panel es cuadrado de 128", 128, RK_TFT_W);
    CHECK_INT("el panel es cuadrado de 128", 128, RK_TFT_H);
    CHECK_STR("la placa del producto", "c3-supermini", RK_PLACA_NOMBRE);

    devkit_verificar();

    RK_SUITE_END();
}
