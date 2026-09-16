#include "node.h"
#include <stddef.h>
#include <string.h>

void rk_roster_init(rk_roster_t *r)
{
    if (r == NULL) {
        return;
    }
    memset(r, 0, sizeof *r);
    r->selected = 0;
}

int rk_roster_add(rk_roster_t *r, const char *nombre,
                  const rk_species_t *sp, const rk_persona_t *persona)
{
    rk_node_t *n;
    int        i;

    if (r == NULL || nombre == NULL || nombre[0] == '\0' ||
        r->count >= RK_MAX_NODES) {
        return -1;
    }

    i = r->count;
    n = &r->nodes[i];
    memset(n, 0, sizeof *n);

    strncpy(n->nombre, nombre, sizeof n->nombre - 1);
    n->nombre[sizeof n->nombre - 1] = '\0';
    /* Los dos ejes del producto, y son independientes a propósito: la
     * especie decide con qué umbrales se juzga la planta, la carcasa decide
     * qué cara pone el aparato. Ninguna deriva de la otra. */
    n->sp      = sp;
    n->persona = persona;

    /* Identificador provisional derivado del índice. Al emparejar de verdad
     * lo reemplaza la MAC; tenerlo desde el alta permite que el simulador y
     * los tests ejerciten el camino de búsqueda por id. */
    n->id[0] = 0x52u;                     /* 'R' */
    n->id[5] = (uint8_t)i;

    rk_mood_state_init(&n->mst);
    rk_bond_init(&n->bond);
    n->verdict.mood     = RK_MOOD_UNKNOWN;
    n->verdict.severity = RK_SEV_OK;
    n->verdict.reason   = rk_mood_reason(RK_MOOD_UNKNOWN);

    r->count = i + 1;
    return i;
}

rk_node_t *rk_roster_find(rk_roster_t *r, const uint8_t id[6])
{
    int i;

    if (r == NULL || id == NULL) {
        return NULL;
    }
    for (i = 0; i < r->count && i < RK_MAX_NODES; i++) {
        if (memcmp(r->nodes[i].id, id, 6) == 0) {
            return &r->nodes[i];
        }
    }
    return NULL;
}

void rk_roster_eval(rk_roster_t *r)
{
    int i;

    if (r == NULL) {
        return;
    }
    for (i = 0; i < r->count && i < RK_MAX_NODES; i++) {
        rk_node_t *n = &r->nodes[i];
        n->verdict = rk_mood_eval(&n->mst, n->sp, &n->tel);
    }
}

rk_link_t rk_node_link(const rk_node_t *n)
{
    if (n == NULL || !n->tel.valid) {
        return RK_LINK_NUNCA;
    }
    if (n->tel.age_s >= RK_LINK_CAIDO_S) {
        return RK_LINK_CAIDO;
    }
    if (n->tel.age_s >= RK_LINK_TIBIO_S) {
        return RK_LINK_TIBIO;
    }
    return RK_LINK_VIVO;
}

const char *rk_link_name(rk_link_t l)
{
    switch (l) {
    case RK_LINK_NUNCA: return "sin enlazar";
    case RK_LINK_VIVO:  return "en linea";
    case RK_LINK_TIBIO: return "demorado";
    case RK_LINK_CAIDO: return "sin senal";
    default:            return "?";
    }
}

int rk_roster_peor(const rk_roster_t *r)
{
    int i, mejor = -1;
    int mejor_sev = -1;
    uint32_t mejor_edad = 0;

    if (r == NULL || r->count <= 0) {
        return -1;
    }
    for (i = 0; i < r->count && i < RK_MAX_NODES; i++) {
        const rk_node_t *n = &r->nodes[i];
        int sev = (int)n->verdict.severity;

        /* A igualdad de severidad gana el que lleva más tiempo callado: si
         * dos plantas piden agua, la que hace rato que nadie mira es la que
         * conviene mostrar. */
        if (sev > mejor_sev ||
            (sev == mejor_sev && n->tel.age_s > mejor_edad)) {
            mejor_sev  = sev;
            mejor_edad = n->tel.age_s;
            mejor      = i;
        }
    }
    return mejor;
}

void rk_roster_cerrar_dia(rk_roster_t *r)
{
    int i;

    if (r == NULL) {
        return;
    }
    for (i = 0; i < r->count && i < RK_MAX_NODES; i++) {
        rk_node_t *n = &r->nodes[i];
        /* Un día sano es un día en el que la planta no llegó a urgente y el
         * aparato estuvo reportando. Un nodo caído no cuenta como día sano:
         * si no sabemos cómo estuvo, no se premia. Es lo que impide que
         * desenchufarlo haga crecer el vínculo gratis. */
        bool sano = n->verdict.severity != RK_SEV_URGENT &&
                    rk_node_link(n) == RK_LINK_VIVO;
        rk_bond_dia(&n->bond, sano);
    }
}
