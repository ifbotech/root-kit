/* almacen.h — lo que sobrevive a un corte de luz.
 *
 *   NVS (Preferences, espacio "rootkit")
 *     secreto    16 bytes de fábrica. Si no existe, se genera: es el modo
 *                "placa de desarrollo". En producción lo graba la estación
 *                de fábrica junto con la persona (ver docs/fabrica.md).
 *     persona    id del Rooti (la figura), grabado en fábrica
 *     rareza     la piel que salió del cofre: 0 común, 1 rara, 2 épica
 *     enlace     época, wifi sí/no, vinculado, revelado
 *     wifi       ssid y clave
 *     nube       URL base del servidor (se puede cambiar desde el portal)
 *     app        URL base del QR, si es otra (un túnel HTTPS en desarrollo)
 *     especie    umbrales que mandó la nube
 *     cal        calibración del capacitivo
 *     vinculo    días sanos, racha
 *     prefs      brillo y modo de pantalla
 *
 *   LittleFS
 *     /historial.bin   lecturas pendientes de subir (nodo/historial.c)
 */
#ifndef ROOTKIT_ESP32_ALMACEN_H
#define ROOTKIT_ESP32_ALMACEN_H

#include <Arduino.h>
extern "C" {
#include "../core/codigo.h"
#include "../core/enlace.h"
#include "../core/vinculo.h"
#include "../net/nube.h"
#include "../nodo/historial.h"
#include "../nodo/soil.h"
}

typedef struct {
    uint8_t  secreto[RK_SECRETO_LEN];
    char     persona[16];
    uint8_t  rareza;
    rk_enlace_nvs_t enlace;
    char     ssid[33];
    char     clave[65];
    char     nube[96];
    char     app[96];       /* base del QR; vacío = la misma que la nube */
    bool     hay_especie;
    rk_especie_guardada_t especie;
    rk_soil_cal_t cal;
    rk_bond_t vinculo;
    char     nombre[24];
    uint8_t  brillo;
    bool     pantalla_siempre;
    uint32_t arranques;
} rk_almacen_t;

/* Carga todo. Devuelve false sólo si NVS no anda (placa rota). */
bool almacen_cargar(rk_almacen_t *a, const char *nube_por_defecto,
                   const char *app_por_defecto);

void almacen_guardar_enlace(const rk_almacen_t *a);
void almacen_guardar_wifi(const rk_almacen_t *a);
void almacen_borrar_wifi(rk_almacen_t *a);
void almacen_guardar_nube(const rk_almacen_t *a);
void almacen_guardar_config(const rk_almacen_t *a);    /* persona, rareza, especie, cal, nombre, prefs */
void almacen_guardar_vinculo(const rk_almacen_t *a);

bool almacen_historial_cargar(rk_historial_t *h);
void almacen_historial_guardar(const rk_historial_t *h);

#endif
