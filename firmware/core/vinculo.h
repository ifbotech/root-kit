/* vinculo.h — el vínculo con la planta, y cómo crece.
 *
 * Esto sobrevivió entero al cambio de producto, y es el único mecanismo de
 * mérito que queda. Antes convivía con la rareza —la especie difícil daba el
 * simbionte legendario—; ahora que la rareza se mudó a la caja física, el
 * vínculo es lo único que NO se compra.
 *
 * DOS REGLAS QUE NO CONVIENE TOCAR
 *
 * 1. LAS ETAPAS AVANZAN CON DÍAS SANOS, NO CON DÍAS TRANSCURRIDOS.
 *    Una planta abandonada tiene un aparato que no evoluciona. Si avanzara
 *    con el reloj, cuidar la planta no cambiaría nada y el producto sería un
 *    adorno con termómetro.
 *
 * 2. UN MAL DÍA CORTA LA RACHA PERO NO BORRA LO ACUMULADO.
 *    Castigar un olvido con meses de progreso convierte un descuido en
 *    motivo para abandonar el producto. La racha es el premio al cuidado
 *    sostenido; los días sanos son el piso que ya te ganaste.
 *
 * Lo que las etapas desbloquean son capas cosméticas sobre la cara —brillos,
 * aura, corona— y no personajes. Ver art/face.h.
 */
#ifndef ROOTKIT_VINCULO_H
#define ROOTKIT_VINCULO_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    RK_ETAPA_ESPORA = 0,   /*   0 días sanos */
    RK_ETAPA_RETONO,       /*   7: "retoño" (no "brote": Brote es un Rooti) */
    RK_ETAPA_JOVEN,        /*  30 */
    RK_ETAPA_MADURO,       /*  90 */
    RK_ETAPA_ANCESTRAL,    /* 180 */
    RK_ETAPA_COUNT
} rk_stage_t;

/* Vive en NVS, uno por maceta. */
typedef struct {
    uint16_t dias_vividos;     /* desde el alta                            */
    uint16_t dias_sanos;       /* los que terminaron bien: mueven la etapa */
    uint16_t racha;            /* días sanos consecutivos                  */
    uint16_t mejor_racha;
} rk_bond_t;

void       rk_bond_init(rk_bond_t *b);
/* Cierra un día. `sano` es true si la planta lo terminó en buen estado. */
void       rk_bond_dia(rk_bond_t *b, bool sano);
rk_stage_t rk_stage_from_bond(const rk_bond_t *b);
const char *rk_stage_name(rk_stage_t e);
/* Días sanos que faltan para la próxima etapa; 0 si ya está en la última. */
uint16_t   rk_stage_faltan(const rk_bond_t *b);
/* Progreso hacia la próxima etapa, de 0 a 100. */
uint8_t    rk_stage_progreso(const rk_bond_t *b);

#endif /* ROOTKIT_VINCULO_H */
