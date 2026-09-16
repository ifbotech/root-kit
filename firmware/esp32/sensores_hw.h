/* sensores_hw.h — hablar con los sensores de verdad.
 *
 * Esta capa sólo mueve bytes: I2C, 1-Wire y ADC. Todo lo que convierte esos
 * bytes en unidades está en nodo/sensores.c, probado en el escritorio.
 *
 * El capacitivo de suelo y la sonda DS18B20 se alimentan a través de un
 * P-MOSFET que sólo se enciende durante la lectura: el capacitivo consume
 * ~5 mA permanentes, que es más que todo el resto del aparato dormido.
 */
#ifndef ROOTKIT_ESP32_SENSORES_H
#define ROOTKIT_ESP32_SENSORES_H

#include <Arduino.h>
extern "C" {
#include "../nodo/sensores.h"
}

void sensores_iniciar(void);

/* Una lectura completa. Tarda ~200 ms (el AHT20 necesita 80 ms para medir y
 * el DS18B20 hasta 750 ms a 12 bits: se lee a 10 bits, 188 ms). */
void sensores_leer(rk_crudos_t *out);

#endif
