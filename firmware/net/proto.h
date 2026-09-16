/* proto.h — protocolo binario entre el ROOTKIT y la app.
 *
 * Cada byte que viaja es tiempo de radio encendida, y la radio es lo único
 * que consume de verdad en el aparato: una trama de 26 bytes contra un JSON de
 * 190 es, a grandes rasgos, un 15% menos de tiempo de transmisión por ciclo.
 * Por eso el formato es binario, fijo y sin campos opcionales.
 *
 * Decisiones que vale la pena no olvidar:
 *
 *  - Todo va en little endian, que es el orden nativo del ESP32 y del x86 del
 *    simulador. Igual se codifica byte a byte, así que no depende del
 *    alineamiento ni del endianness del compilador.
 *  - La iluminancia va comprimida en 16 bits con mantisa y exponente: hace
 *    falta cubrir de 1 a 100.000 lux, y con lineal a 16 bits habría que
 *    sacrificar la resolución baja, justo donde vive el umbral de noche.
 *  - CRC16-CCITT sobre todo menos el propio CRC. Con paquetes por radio en
 *    2,4 GHz y vecinos ruidosos, una trama corrupta que pase por buena
 *    le pone al aparato una cara equivocada.
 *  - El número de secuencia permite descartar duplicados y detectar pérdidas
 *    sin reloj compartido.
 *
 * QUÉ CAMBIÓ EN LA VERSIÓN 2, Y POR QUÉ
 *
 * La v1 asumía un nodo sin pantalla: mandaba números crudos y el ánimo lo
 * decidía otro. Desde que cada aparato tiene su propia pantalla eso no
 * alcanza, porque tiene que saber qué cara poner aunque no haya red. Así
 * que:
 *
 *  - CONFIG creció de 18 a 32 bytes y ahora lleva los UMBRALES DE LA
 *    ESPECIE, además de qué carcasa lleva puesta y su etapa. Con eso el
 *    aparato corre core/mood.c y se dibuja solo. CONFIG viaja en sentido
 *    app -> aparato, una sola vez al emparejar y cada vez que cambia la
 *    especie o la carcasa, así que su tamaño no pesa en la batería.
 *  - TELEMETRY creció de 24 a 26 bytes y ahora lleva el ÁNIMO YA RESUELTO.
 *    La app no lo recalcula: lo muestra. Así no hay forma de que la cara de
 *    la maceta y la ficha del teléfono digan cosas distintas.
 *  - HELLO lleva la VARIANTE DE PLACA en un byte que antes era relleno.
 */
#ifndef ROOTKIT_PROTO_H
#define ROOTKIT_PROTO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define RK_PROTO_MAGIC    0x52u   /* 'R' */
#define RK_PROTO_VERSION  2u

#define RK_PKT_TELEMETRY  1u
#define RK_PKT_HELLO      2u
#define RK_PKT_CONFIG     3u      /* app -> aparato */

#define RK_TELEMETRY_LEN  26
#define RK_HELLO_LEN      18
#define RK_CONFIG_LEN     32
#define RK_PKT_MAX        32

/* Mapa de bytes, por si hace falta leerlo desde otro lenguaje:
 *
 *   TELEMETRY (26)  0 magic | 1 ver | 2 tipo | 3 flags | 4..9 id
 *                  10 seq   | 12 soil% | 13 temp_dc | 15 rh%
 *                  16 lux   | 18 batt_mv | 20 soil_raw | 22 estado
 *                  23 etapa | 24 crc
 *   HELLO     (18)  0..9 cabecera | 10 hw | 11 fw_maj | 12 fw_min
 *                  13 boot_count | 15 hw_variant | 16 crc
 *   CONFIG    (32)  0..9 cabecera | 10 interval_s | 12 dry_raw | 14 wet_raw
 *                  16 soil_min | 17 soil_max | 18 temp_min_dc
 *                  20 temp_max_dc | 22 rh_min | 23 persona_idx | 24 etapa
 *                  25 lux_min | 27 lux_max | 29 cfg_flags | 30 crc
 *
 * El byte `estado` empaqueta ánimo y severidad: los cuatro bits bajos son el
 * rk_mood_t (once valores, entran de sobra) y los bits 4-5 la severidad.
 * Empaquetar en vez de gastar dos bytes no es microoptimización caprichosa:
 * la trama tiene que seguir entrando en un paquete ESP-NOW corto.
 */

/* Banderas de la trama de telemetría. */
#define RK_FLAG_LOW_BATT   0x01u
#define RK_FLAG_FIRST_BOOT 0x02u
#define RK_FLAG_CALIBRATED 0x04u
#define RK_FLAG_SOIL_FAULT 0x08u  /* lectura fuera de rango físico  */

/* Empaquetado del byte de estado. */
#define RK_ESTADO_MOOD(b)  ((uint8_t)((b) & 0x0Fu))
#define RK_ESTADO_SEV(b)   ((uint8_t)(((b) >> 4) & 0x03u))
#define RK_ESTADO(m, s)    ((uint8_t)(((m) & 0x0Fu) | (((s) & 0x03u) << 4)))

typedef struct {
    uint8_t  id[6];      /* derivado de la MAC, identifica al nodo      */
    uint16_t seq;
    uint8_t  flags;
    uint8_t  soil_pct;
    int16_t  temp_dc;
    uint8_t  rh_pct;
    uint32_t lux;
    uint16_t batt_mv;
    uint16_t soil_raw;   /* ADC crudo: permite recalibrar sin ir a la maceta */
    uint8_t  mood;       /* rk_mood_t, evaluado en el propio aparato     */
    uint8_t  severity;   /* rk_severity_t                                */
    uint8_t  etapa;      /* rk_stage_t: cada maceta lleva su vínculo     */
} rk_telemetry_pkt_t;

typedef struct {
    uint8_t  id[6];
    uint8_t  hw_rev;
    uint8_t  fw_major;
    uint8_t  fw_minor;
    uint16_t boot_count;
    uint8_t  hw_variant; /* variante de placa, 0 = la de referencia      */
} rk_hello_pkt_t;

typedef struct {
    uint8_t  id[6];
    uint16_t interval_s;    /* período base de muestreo                 */
    uint16_t soil_dry_raw;  /* calibración: lectura en aire              */
    uint16_t soil_wet_raw;  /* calibración: lectura sumergido            */
    uint8_t  flags;

    /* Umbrales de la especie: con esto el aparato evalúa su propio ánimo. */
    uint8_t  soil_min;
    uint8_t  soil_max;
    int16_t  temp_min_dc;
    int16_t  temp_max_dc;
    uint8_t  rh_min;
    uint32_t lux_min;
    uint32_t lux_max;

    /* Qué carcasa lleva puesta. Sale de la caja ciega y la carga el usuario
     * en la app; el aparato no tiene forma de saberlo solo. Ver la nota de
     * docs/carcasas.md sobre por qué no se detecta por hardware todavía. */
    uint8_t  persona_idx;   /* índice en rk_persona_table, 0xFF = ninguna  */
    uint8_t  etapa;
} rk_config_pkt_t;

typedef enum {
    RK_PROTO_OK            =  0,
    RK_PROTO_E_TRUNCATED   = -1,
    RK_PROTO_E_MAGIC       = -2,
    RK_PROTO_E_VERSION     = -3,
    RK_PROTO_E_CRC         = -4,
    RK_PROTO_E_TYPE        = -5,
    RK_PROTO_E_LEN         = -6,
    RK_PROTO_E_ARG         = -7
} rk_proto_err_t;

/* Codificación. Devuelven bytes escritos, o un rk_proto_err_t negativo. */
int rk_proto_encode_telemetry(uint8_t *buf, size_t cap,
                              const rk_telemetry_pkt_t *p);
int rk_proto_encode_hello(uint8_t *buf, size_t cap, const rk_hello_pkt_t *p);
int rk_proto_encode_config(uint8_t *buf, size_t cap, const rk_config_pkt_t *p);

/* Inspección sin decodificar del todo: devuelve el tipo o un error. */
int rk_proto_peek_type(const uint8_t *buf, size_t len);

/* Decodificación. Devuelven RK_PROTO_OK o un error negativo. */
int rk_proto_decode_telemetry(const uint8_t *buf, size_t len,
                              rk_telemetry_pkt_t *out);
int rk_proto_decode_hello(const uint8_t *buf, size_t len, rk_hello_pkt_t *out);
int rk_proto_decode_config(const uint8_t *buf, size_t len,
                           rk_config_pkt_t *out);

/* Utilidades expuestas porque se testean por separado. */
uint16_t rk_crc16(const uint8_t *data, size_t len);
uint16_t rk_lux_encode(uint32_t lux);
uint32_t rk_lux_decode(uint16_t code);

/* ¿La secuencia `seq` es posterior a `last`? Maneja el envolvimiento de los
 * 16 bits: una diferencia de más de media vuelta se interpreta como pasado,
 * que es lo que evita que un duplicado viejo reviva tras el wrap. */
bool rk_seq_is_new(uint16_t last, uint16_t seq);

const char *rk_proto_strerror(int err);

#endif /* ROOTKIT_PROTO_H */
