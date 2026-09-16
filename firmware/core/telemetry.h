/* telemetry.h — lo que un nodo mide en su maceta.
 *
 * Sin punto flotante a propósito: la temperatura viaja en décimas de grado
 * para que la misma struct sirva en el ESP32-C3, en el ESP32 clásico y en el
 * simulador de escritorio sin sorpresas de FPU.
 */
#ifndef ROOTKIT_TELEMETRY_H
#define ROOTKIT_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

/* Valor de temperatura que significa "no hay sensor". */
#define RK_TEMP_NO_HAY  ((int16_t)-32768)

/* Qué sensores no contestaron en esta lectura. El ánimo ignora la magnitud
 * que falla en vez de reaccionar a un cero inventado. */
#define RK_FALLA_SUELO  0x01u
#define RK_FALLA_AIRE   0x02u   /* AHT20: temperatura y humedad del aire */
#define RK_FALLA_LUZ    0x04u
#define RK_FALLA_SONDA  0x08u   /* DS18B20: es opcional, no afecta el ánimo */

typedef struct {
    bool     valid;     /* false si nunca llegó una lectura de este nodo   */
    uint32_t age_s;     /* segundos transcurridos desde la última lectura  */
    uint8_t  soil_pct;  /* 0-100, humedad volumétrica aproximada           */
    int16_t  temp_dc;   /* décimas de grado Celsius: 234 == 23,4 °C        */
    uint8_t  rh_pct;    /* 0-100, humedad relativa del aire                */
    uint32_t lux;       /* iluminancia medida por el BH1750                */
    uint16_t batt_mv;   /* milivolts de la celda, para avisar antes de morir */
    int16_t  suelo_dc;  /* temperatura de la tierra (DS18B20), décimas; o
                         * RK_TEMP_NO_HAY si la sonda no está               */
    uint16_t suelo_raw; /* ADC crudo del capacitivo: recalibrar sin ir      */
    bool     usb;       /* enchufado: la celda carga y el rail mide VBUS    */
    uint8_t  fallas;    /* RK_FALLA_*                                       */
} rk_telemetry_t;

#endif /* ROOTKIT_TELEMETRY_H */
