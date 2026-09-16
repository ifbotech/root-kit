/* energia.h — cuándo prender la pantalla y cuándo dormir.
 *
 * LA REGLA
 *
 *   Enchufado             pantalla siempre prendida, nunca duerme.
 *   A batería, en el QR,  pantalla prendida: hay alguien configurando.
 *   dormida o despertando
 *   A batería, con cara   se prende al tocarla y se apaga a los 20 s. Entre
 *                         medición y medición, deep sleep. Desde la app se
 *                         puede pedir "siempre prendida", sabiendo lo que
 *                         cuesta (docs/hardware.md tiene los números).
 *
 * EL RELOJ MONÓTONO
 *
 * millis() vuelve a cero en cada deep sleep. El historial necesita un reloj
 * que no retroceda, así que se guarda en memoria RTC (que sobrevive al deep
 * sleep) cuánto iba a dormir el aparato y se suma al despertar.
 */
#ifndef ROOTKIT_ESP32_ENERGIA_H
#define ROOTKIT_ESP32_ENERGIA_H

#include <Arduino.h>

typedef enum { RK_DESPERTAR_FRIO = 0, RK_DESPERTAR_TIMER, RK_DESPERTAR_TOQUE } rk_despertar_causa_t;

void                 energia_iniciar(void);
rk_despertar_causa_t energia_causa(void);

uint32_t energia_reloj_s(void);
/* Tras un corte de luz, el reloj arranca de cero pero el historial trae
 * lecturas con relojes mayores: se adelanta para que "hace" no mienta. */
void     energia_reloj_minimo(uint32_t s);

/* Duerme hasta `segundos` o hasta que toquen la maceta. No vuelve. */
void     energia_dormir(uint32_t segundos);

#endif
