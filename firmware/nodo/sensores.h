/* sensores.h — de los bytes que devuelve cada sensor a una lectura.
 *
 * LOS SENSORES DEL ROOTKIT
 *
 *   humedad de suelo   capacitivo v1.2/v2.0 (TLC555), analógico, con
 *                      alimentación cortada por MOSFET entre lecturas
 *   aire               AHT20 por I2C: temperatura ±0,3 °C, humedad ±2 %
 *   luz                BH1750 por I2C: 1 a 65.535 lux
 *   tierra             DS18B20 sumergible por 1-Wire (opcional)
 *   batería / USB      divisor sobre el riel, al ADC
 *
 * El detalle eléctrico, los modelos alternativos y por qué cada uno está en
 * docs/hardware.md.
 *
 * QUÉ HACE ESTE ARCHIVO Y QUÉ NO
 *
 * No habla I2C ni 1-Wire: eso es de la placa. Recibe los bytes crudos que la
 * placa leyó y devuelve números con unidades, verificando CRC y plausibilidad.
 * Así toda la aritmética de conversión —que es donde están los errores
 * silenciosos, un corrimiento de bits mal puesto da 23 °C igual de
 * convincentes que los verdaderos— se prueba en el escritorio con los
 * ejemplos de las hojas de datos.
 *
 * UN SENSOR QUE FALLA NO INVENTA DATOS
 *
 * Cada lectura fallida prende un bit en `fallas` y el ánimo ignora esa
 * magnitud. Una planta con el AHT20 desoldado no puede tener frío: puede no
 * saber qué temperatura hace, y eso se lo dice la app al usuario.
 */
#ifndef ROOTKIT_SENSORES_H
#define ROOTKIT_SENSORES_H

#include <stdbool.h>
#include <stdint.h>
#include "soil.h"
#include "../core/telemetry.h"

/* ---------------------------------------------------------------- AHT20 -- */
/* Lectura de 7 bytes tras el comando 0xAC 0x33 0x00 y 80 ms de espera:
 * estado, 20 bits de humedad, 20 bits de temperatura, CRC-8 (0x31, 0xFF). */
bool rk_aht20_convertir(const uint8_t b[7], int16_t *temp_dc, uint8_t *rh_pct);
uint8_t rk_crc8_aht(const uint8_t *b, int n);

/* --------------------------------------------------------------- BH1750 -- */
/* Modo alta resolución: lux = cuenta / 1,2 * (69 / MTreg). Con MTreg 69 (el
 * de fábrica) satura a 54.612 lux, que un sol directo de verano supera; la
 * placa puede bajar MTreg a 31 para sol pleno y pasarlo acá. */
uint32_t rk_bh1750_lux(uint16_t cuenta, uint8_t mtreg);

/* -------------------------------------------------------------- DS18B20 -- */
/* Scratchpad de 9 bytes con CRC Maxim en el último. Devuelve false si el
 * CRC falla, si la línea está suelta (todo 0xFF) o si es el 85,0 °C que el
 * sensor reporta cuando no llegó a convertir. */
bool rk_ds18b20_convertir(const uint8_t sp[9], int16_t *temp_dc);
uint8_t rk_crc8_maxim(const uint8_t *b, int n);

/* ---------------------------------------------------------- riel y USB -- */
/* Milivolts del riel a partir de lo que midió el ADC (ya calibrado a mV)
 * detrás de un divisor de `r_arriba` sobre `r_abajo`. */
uint16_t rk_riel_mv(uint16_t adc_mv, uint32_t r_arriba, uint32_t r_abajo);

/* Con el cargador en paralelo, el riel sigue a VBUS menos el Schottky (~4,6 V)
 * cuando hay USB, y a la celda (<= 4,2 V) cuando no. El umbral queda en el
 * medio y con margen para las dos tolerancias. */
#define RK_RIEL_USB_MV 4350u
bool rk_riel_usb(uint16_t riel_mv);

/* --------------------------------------------------------- la lectura ---- */
#define RK_SUELO_MUESTRAS 9

typedef struct {
    uint16_t suelo[RK_SUELO_MUESTRAS];
    uint8_t  n_suelo;          /* 0 = no se leyó                          */
    uint8_t  aht[7];
    bool     aht_leido;        /* el I2C contestó                         */
    uint16_t bh1750;
    uint8_t  bh_mtreg;
    bool     bh_leido;
    uint8_t  ds[9];
    bool     ds_leido;         /* false también si no hay sonda           */
    uint16_t riel_adc_mv;
    uint32_t r_arriba, r_abajo;
} rk_crudos_t;

/* Arma una telemetría completa y válida. Nunca falla: lo que no se pudo
 * leer queda marcado en `out->fallas` con un valor neutro al lado. */
void rk_sensores_telemetria(const rk_crudos_t *c, const rk_soil_cal_t *cal,
                            rk_telemetry_t *out);

#endif /* ROOTKIT_SENSORES_H */
