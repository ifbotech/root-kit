/* historial.h — las lecturas que todavía no llegaron a la nube.
 *
 * El aparato mide aunque no haya wifi: se cortó internet, el router está
 * lejos, o está en su primer día y todavía nadie lo configuró. Esas
 * lecturas no se pierden: se acumulan acá y viajan en tanda en la próxima
 * consulta que salga bien. La app muestra la curva completa, sin el hueco
 * de las horas sin red.
 *
 * SIN RELOJ DE PARED
 *
 * Cada registro guarda el RELOJ MONÓTONO del aparato (segundos desde que se
 * encendió, sumando los que pasó dormido), no una fecha. Al mandarlo se
 * convierte en "hace N segundos" y el servidor le pone la fecha restando a
 * su propia hora. Así no hace falta NTP, ni pila de reloj, ni lidiar con un
 * aparato que arranca en 1970.
 *
 * FORMATO EN FLASH
 *
 * Se guarda como bytes explícitos (little endian, campo por campo), no como
 * un volcado del struct: el mismo archivo se lee igual en el C3 (RISC-V),
 * en el ESP32 clásico (Xtensa) y en el escritorio, y sobrevive a un cambio
 * de compilador que reordene el relleno. Lleva versión y CRC32: un archivo
 * cortado a mitad de escritura por un corte de luz se descarta entero en
 * vez de devolver lecturas corruptas.
 */
#ifndef ROOTKIT_HISTORIAL_H
#define ROOTKIT_HISTORIAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../core/telemetry.h"

/* Tres días a una lectura cada quince minutos, que es lo que aguanta un
 * corte de internet largo de fin de semana. 288 x 20 bytes = 5,8 KB. */
#define RK_HIST_CAP      288u
#define RK_HIST_REG_LEN   20u
#define RK_HIST_CAB_LEN   16u
#define RK_HIST_BYTES_MAX (RK_HIST_CAB_LEN + RK_HIST_CAP * RK_HIST_REG_LEN + 4u)

#define RK_HIST_SIN_DATO_U8   0xFFu
#define RK_HIST_SIN_LUX       0xFFFFFFFFu

typedef struct {
    uint32_t reloj_s;
    uint32_t lux;           /* RK_HIST_SIN_LUX si falló                  */
    int16_t  temp_dc;       /* RK_TEMP_NO_HAY si falló                   */
    int16_t  suelo_dc;      /* RK_TEMP_NO_HAY si no hay sonda            */
    uint16_t bat_mv;        /* 0 = enchufado o desconocido               */
    uint16_t suelo_raw;
    uint8_t  suelo_pct;     /* RK_HIST_SIN_DATO_U8 si falló              */
    uint8_t  hr_pct;        /* RK_HIST_SIN_DATO_U8 si falló              */
    uint8_t  animo;         /* rk_mood_t evaluado en el aparato          */
    uint8_t  banderas;      /* bit 0: USB; bits 1-2: severidad;          *
                             * bit 3: el riego se escurrió (RK_FALLA_    *
                             * ESCURRE); bits 4-7: fallas de sensor << 4 */
} rk_registro_t;

typedef struct {
    rk_registro_t reg[RK_HIST_CAP];
    uint16_t inicio;        /* el más viejo                              */
    uint16_t cuenta;
    uint32_t perdidos;      /* los que se pisaron por falta de lugar     */
} rk_historial_t;

void rk_historial_iniciar(rk_historial_t *h);

/* Un registro a partir de una telemetría y el veredicto del aparato. La
 * severidad viaja con la lectura para que la app y las notificaciones
 * usen exactamente la misma que puso la cara, sin recalcularla. */
rk_registro_t rk_registro_desde(const rk_telemetry_t *t, uint32_t reloj_s,
                                uint8_t animo, uint8_t severidad);
uint8_t rk_registro_severidad(const rk_registro_t *r);
/* Las RK_FALLA_* del registro, escurrimiento incluido. */
uint8_t rk_registro_fallas(const rk_registro_t *r);

/* Agrega al final. Si está lleno pisa el más viejo y cuenta la pérdida: lo
 * reciente importa más que lo de hace tres días. */
void rk_historial_agregar(rk_historial_t *h, const rk_registro_t *r);

/* El i-ésimo más viejo (0 = el más viejo). NULL fuera de rango. */
const rk_registro_t *rk_historial_ver(const rk_historial_t *h, uint16_t i);

/* Descarta los `n` más viejos: los que la nube confirmó. */
void rk_historial_descartar(rk_historial_t *h, uint16_t n);

/* Serializa a `buf`. Devuelve los bytes escritos, 0 si no entra. */
size_t rk_historial_serializar(const rk_historial_t *h, uint8_t *buf, size_t cap);

/* Carga desde bytes. Devuelve false (y deja el historial vacío) si el
 * formato, la versión o el CRC no coinciden. */
bool rk_historial_cargar(rk_historial_t *h, const uint8_t *buf, size_t len);

uint32_t rk_crc32(const uint8_t *b, size_t n);

#endif /* ROOTKIT_HISTORIAL_H */
