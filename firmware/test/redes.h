/* redes.h — GENERADO por tools/pcb.py desde hardware/pcb/nucleo.json.
 *
 * No se edita a mano: se edita el JSON y se corre `make pcb`. Es la
 * netlist del sustrato vista desde el firmware, y test_placa.c la
 * compara contra placa.h para que los pines tengan una sola verdad.
 */
#ifndef ROOTKIT_REDES_H
#define ROOTKIT_REDES_H

typedef struct {
    int         gpio;      /* GPIO del ESP32-C3 SuperMini            */
    const char *red;       /* como se llama la red en el sustrato    */
    const char *riel;      /* de que cuelga lo que hay del otro lado */
    const char *arranque;  /* alto / bajo / libre al encender        */
    int         adc1;      /* 1 si la red va a una entrada del ADC1  */
    int         despierta; /* 1 si tiene que despertar del sueño     */
} rk_red_t;

static const rk_red_t RK_REDES[] = {
    {  0, "SUELO", "3V3S", "libre", 1, 0 },
    {  1, "RIEL", "VINT", "libre", 1, 0 },
    {  2, "SENS_EN", "3V3", "alto", 0, 0 },
    {  3, "TOQUE", "3V3", "libre", 0, 1 },
    {  4, "SDA", "3V3", "libre", 0, 0 },
    {  5, "SCL", "3V3", "libre", 0, 0 },
    {  6, "SCK", "3V3", "libre", 0, 0 },
    {  7, "MOSI", "3V3", "libre", 0, 0 },
    {  8, "OW", "3V3", "alto", 0, 0 },
    {  9, "NC_BOOT", "—", "alto", 0, 0 },
    { 10, "TFT_DC", "3V3", "libre", 0, 0 },
    { 20, "TFT_CS", "3V3", "libre", 0, 0 },
    { 21, "BL_G", "3V3", "bajo", 0, 0 },
};
#define RK_REDES_N ((int)(sizeof RK_REDES / sizeof RK_REDES[0]))

/* Lo que el sustrato le promete al firmware, en numeros. */
#define RK_SUSTRATO_VERSION        "v1.0"
#define RK_SUELO_DIVISOR_NUM       1
#define RK_SUELO_DIVISOR_DEN       1
#define RK_RIEL_R_ARRIBA_OHM       470000u
#define RK_RIEL_R_ABAJO_OHM        470000u
#define RK_TFT_BL_ALTO_ENCIENDE    1
#define RK_REPOSO_OBJETIVO_UA      80

#endif /* ROOTKIT_REDES_H */
