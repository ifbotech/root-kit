/* telemetry.h — lo que un Spore le manda a la Terminal.
 *
 * Sin punto flotante a propósito: la temperatura viaja en décimas de grado
 * para que la misma struct sirva en el ESP32-C3 del Spore, en el ESP32-S3
 * de la Terminal y en el simulador de escritorio sin sorpresas de FPU.
 */
#ifndef ROOTKIT_TELEMETRY_H
#define ROOTKIT_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool     valid;     /* false si nunca llegó una lectura de este Spore  */
    uint32_t age_s;     /* segundos transcurridos desde la última lectura  */
    uint8_t  soil_pct;  /* 0-100, humedad volumétrica aproximada           */
    int16_t  temp_dc;   /* décimas de grado Celsius: 234 == 23,4 °C        */
    uint8_t  rh_pct;    /* 0-100, humedad relativa del aire                */
    uint32_t lux;       /* iluminancia medida por el BH1750                */
    uint16_t batt_mv;   /* milivolts de la celda, para avisar antes de morir */
} rk_telemetry_t;

#endif /* ROOTKIT_TELEMETRY_H */
