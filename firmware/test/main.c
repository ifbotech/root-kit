/* Corredor de pruebas de ROOTKIT.
 *
 *   make test          corre todo
 *   ./build/rk_test    lo mismo, directo
 *
 * Devuelve 0 si está todo en verde, 1 si algo falló, que es lo que mira CI.
 */
#include <stdio.h>
#include "rk_test.h"

int main(void)
{
    printf("\n  ROOTKIT — pruebas\n");
    printf("  =================\n");

    suite_mood();
    suite_proto();
    suite_spore();
    suite_gfx();
    suite_render();
    suite_companion();
    suite_eyes();

    return rk_t_report();
}
