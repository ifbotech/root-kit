#include "sensores_hw.h"
#include "placa.h"
#include <Wire.h>

/* ------------------------------------------------------------------ I2C -- */
static bool i2c_escribir(uint8_t dir, const uint8_t *b, size_t n)
{
    Wire.beginTransmission(dir);
    Wire.write(b, n);
    return Wire.endTransmission() == 0;
}

static bool i2c_leer(uint8_t dir, uint8_t *b, size_t n)
{
    if (Wire.requestFrom((int)dir, (int)n) != (int)n) {
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        b[i] = (uint8_t)Wire.read();
    }
    return true;
}

/* --------------------------------------------------------------- 1-Wire -- */
/* Un DS18B20 solo en la línea: no hace falta la librería OneWire, y así se
 * evita su dependencia de registros por arquitectura. Los tiempos son los
 * de la nota de aplicación 126 de Maxim, dentro de una sección crítica
 * para que el wifi no los estire. */
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

static void ow_bajo(void)  { digitalWrite(RK_PIN_UNOWIRE, LOW); }
static void ow_suelto(void) { digitalWrite(RK_PIN_UNOWIRE, HIGH); }

static bool ow_reset(void)
{
    bool presente;
    portENTER_CRITICAL(&g_mux);
    ow_bajo();
    delayMicroseconds(480);
    ow_suelto();
    delayMicroseconds(70);
    presente = digitalRead(RK_PIN_UNOWIRE) == LOW;
    portEXIT_CRITICAL(&g_mux);
    delayMicroseconds(410);
    return presente;
}

static void ow_bit_escribir(bool bit)
{
    portENTER_CRITICAL(&g_mux);
    ow_bajo();
    delayMicroseconds(bit ? 6 : 60);
    ow_suelto();
    portEXIT_CRITICAL(&g_mux);
    delayMicroseconds(bit ? 94 : 40);
}

static bool ow_bit_leer(void)
{
    bool bit;
    portENTER_CRITICAL(&g_mux);
    ow_bajo();
    delayMicroseconds(6);
    ow_suelto();
    delayMicroseconds(9);
    bit = digitalRead(RK_PIN_UNOWIRE) == HIGH;
    portEXIT_CRITICAL(&g_mux);
    delayMicroseconds(85);
    return bit;
}

static void ow_escribir(uint8_t v)
{
    for (int i = 0; i < 8; i++) {
        ow_bit_escribir((v >> i) & 1u);
    }
}

static uint8_t ow_leer(void)
{
    uint8_t v = 0;
    for (int i = 0; i < 8; i++) {
        if (ow_bit_leer()) {
            v |= (uint8_t)(1u << i);
        }
    }
    return v;
}

/* Deja la sonda a 10 bits (0,25 °C): sobra para tierra y mide en 188 ms. */
static bool ds18b20_leer(uint8_t sp[9])
{
    if (!ow_reset()) {
        return false;
    }
    ow_escribir(0xCC);                 /* SKIP ROM: hay una sola sonda  */
    ow_escribir(0x44);                 /* CONVERT T                     */
    delay(200);
    if (!ow_reset()) {
        return false;
    }
    ow_escribir(0xCC);
    ow_escribir(0xBE);                 /* READ SCRATCHPAD               */
    for (int i = 0; i < 9; i++) {
        sp[i] = ow_leer();
    }
    return true;
}

/* ------------------------------------------------------------- lectura -- */
void sensores_iniciar(void)
{
    pinMode(RK_PIN_SENSORES_EN, OUTPUT);
    digitalWrite(RK_PIN_SENSORES_EN, HIGH);        /* apagados */
    pinMode(RK_PIN_UNOWIRE, OUTPUT_OPEN_DRAIN);
    digitalWrite(RK_PIN_UNOWIRE, HIGH);
    analogReadResolution(12);
    analogSetPinAttenuation(RK_PIN_SUELO_ADC, ADC_11db);
    analogSetPinAttenuation(RK_PIN_RIEL_ADC, ADC_11db);
    Wire.begin(RK_PIN_SDA, RK_PIN_SCL, 100000);

    /* AHT20: si no reporta calibrado, se inicializa (0xBE 0x08 0x00). */
    uint8_t est = 0;
    if (i2c_leer(RK_I2C_AHT20, &est, 1) && !(est & 0x08)) {
        const uint8_t init[3] = { 0xBE, 0x08, 0x00 };
        i2c_escribir(RK_I2C_AHT20, init, 3);
        delay(10);
    }
    /* BH1750: encendido. */
    const uint8_t on = 0x01;
    i2c_escribir(RK_I2C_BH1750, &on, 1);
}

void sensores_leer(rk_crudos_t *c)
{
    memset(c, 0, sizeof *c);

    digitalWrite(RK_PIN_SENSORES_EN, LOW);          /* encender */

    /* Mientras el capacitivo se estabiliza (~100 ms), se disparan las
     * mediciones de aire y luz, que también tardan. */
    const uint8_t aht_medir[3] = { 0xAC, 0x33, 0x00 };
    bool aht_ok = i2c_escribir(RK_I2C_AHT20, aht_medir, 3);
    const uint8_t bh_una = 0x20;                    /* una lectura H-res */
    bool bh_ok = i2c_escribir(RK_I2C_BH1750, &bh_una, 1);
    delay(180);

    if (aht_ok) {
        c->aht_leido = i2c_leer(RK_I2C_AHT20, c->aht, 7);
    }
    if (bh_ok) {
        uint8_t b[2];
        if (i2c_leer(RK_I2C_BH1750, b, 2)) {
            c->bh1750 = (uint16_t)((b[0] << 8) | b[1]);
            c->bh_mtreg = 69;
            c->bh_leido = true;
        }
    }

    for (int i = 0; i < RK_SUELO_MUESTRAS; i++) {
        c->suelo[i] = (uint16_t)analogRead(RK_PIN_SUELO_ADC);
        delay(4);
    }
    c->n_suelo = RK_SUELO_MUESTRAS;

    c->ds_leido = ds18b20_leer(c->ds);

    digitalWrite(RK_PIN_SENSORES_EN, HIGH);         /* apagar */

    c->riel_adc_mv = (uint16_t)analogReadMilliVolts(RK_PIN_RIEL_ADC);
    c->r_arriba = RK_RIEL_R_ARRIBA;
    c->r_abajo = RK_RIEL_R_ABAJO;
}
