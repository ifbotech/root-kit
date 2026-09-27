/* banco.cpp — el firmware de banco del prototipo NodeMCU de Rocío.
 *
 * PARA QUÉ ES
 *
 * Para probar el hardware recién soldado sin la ceremonia del producto: no
 * hay QR, ni vínculo, ni nube, ni cofre. Arranca directo en la cara de Kip, y
 * esa cara reacciona a los sensores DE VERDAD con el mismo motor de ánimo que
 * el producto (core/mood.c):
 *
 *   - el capacitivo al aire da sed; en un vaso de agua, ahogo
 *   - tapar la LDR da oscuridad, y a los 8 segundos se duerme (de noche)
 *   - un dedo sobre el HTU21D sube la temperatura: calor
 *
 * Las lecturas van en las franjas que le sobran al panel de 2,2" arriba y
 * abajo de la cara, y cada segundo por el puerto serie, con los valores
 * crudos al lado para ver si un sensor está vivo.
 *
 * EL BOTÓN BOOT
 *
 *   corto   pasa de modo: AUTO (el ánimo de los sensores) y después los
 *           once ánimos de Kip uno por uno, para ver todas las animaciones
 *   largo   cambia la piel: común, rara, épica
 *
 * Por el monitor serie (115200) se maneja todo con una letra: "?" muestra la
 * ayuda.
 *
 * LO QUE NO ES
 *
 * No es el firmware del producto: esta placa no tiene toque, ni batería, ni
 * BH1750, y la luz es una LDR (nodo/sensores.h, rk_ldr_lux), que da lux
 * aproximados. La noche llega en 8 segundos y no en dos horas porque se mide
 * una vez por segundo y no cada quince minutos.
 */
#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <esp_heap_caps.h>
#include "../esp32/placa.h"
#include "../esp32/pantalla.h"
extern "C" {
#include "../art/face.h"
#include "../core/mood.h"
#include "../core/persona.h"
#include "../core/species.h"
#include "../gfx/font.h"
#include "../nodo/sensores.h"
#include "../nodo/soil.h"
#include "../ui/cara.h"
}

#define BANCO_VERSION   "banco-1"
#define MEDIR_MS        1000u      /* una lectura completa por segundo       */
#define CUADRO_MS       33u        /* ~30 cuadros por segundo, si le da      */
#define BOTON_LARGO_MS  700u
#define VCC_MV          3300u      /* los módulos van a la línea de 3,3 V    */
#define N_MUESTRAS      9          /* mediana de los analógicos              */
/* El ADC del ESP32 a 11 dB no baja de ~140 mV: con mucha luz la AO de la
 * LDR cae a unos 30 mV y el ADC se queda en el piso. Ahí no se sabe cuánta
 * luz hay, sólo que es mucha: se toma como sol. */
#define LUZ_PISO_RAW    16u
#define LUZ_SATURADA    30000u

/* Los colores de las franjas: las del banco, no las de la cara. */
#define COL_FRANJA   RK_RGB(16, 18, 20)
#define COL_ROTULO   RK_RGB(130, 136, 142)
#define COL_VALOR    RK_RGB(240, 180, 80)
#define COL_FALLA    RK_RGB(255, 90, 80)

/* Los umbrales con los que arranca: los de una planta de interior que
 * perdona, con la luz corrida hacia abajo, porque la LDR mide aproximado y
 * una mesa de trabajo tiene 200 a 500 lux. Así, en la mesa y con el
 * capacitivo en tierra húmeda, Kip está contento, y cada sensor lo saca de
 * ahí por su lado: el capacitivo al aire (sed) o en agua (ahogo), la LDR
 * tapada (oscuro, y a los 8 s dormido) o con una linterna encima (sol), un
 * dedo sobre el HTU21D (calor). Con "e" se pasa a las especies de verdad. */
static const rk_species_t ESPECIE_BANCO = {
    "banco", "Banco de pruebas", 20, 60, 150, 300, 30, 100, 20000, 0
};

/* ----------------------------------------------------------- el estado -- */
typedef enum { MODO_AUTO = -1 } modo_t;   /* 0..10: un ánimo de DEMO */

/* El recorrido del botón: primero los cinco de las láminas de Rocío, en el
 * orden en que llegaron, y después los que salen de su vocabulario. */
static const rk_mood_t DEMO[] = {
    RK_MOOD_HAPPY, RK_MOOD_DROWNING, RK_MOOD_HOT, RK_MOOD_DARK, RK_MOOD_PARCHED_AIR,
    RK_MOOD_THIRSTY, RK_MOOD_COLD, RK_MOOD_SCORCHED, RK_MOOD_SLEEPING,
    RK_MOOD_UNKNOWN, RK_MOOD_OFFLINE,
};
#define N_DEMO ((int)(sizeof DEMO / sizeof DEMO[0]))

static const rk_persona_t *g_kip;
static const rk_species_t *g_especie;
static int                 g_especie_idx;
static uint8_t             g_rareza;
static int                 g_modo = MODO_AUTO;
static rk_cara_anim_t      g_anim;
static rk_mood_state_t     g_mst;
static rk_verdict_t        g_veredicto;
static rk_soil_cal_t       g_cal;
static Preferences         g_nvs;

/* Las lecturas: los analógicos se muestrean de a uno por cuadro (así la
 * animación no se frena), y la mediana de los últimos nueve es la lectura. */
static uint16_t g_suelo[N_MUESTRAS], g_luz_mv[N_MUESTRAS], g_luz_raw[N_MUESTRAS];
static bool     g_luz_saturada;
static int      g_i_muestra;
static bool     g_luz_do;
static bool     g_htu_ok, g_htu_presente;
static uint8_t  g_sht[6];
static rk_telemetry_t g_tel;
static uint16_t g_suelo_raw, g_luz_ao_mv;

/* El HTU21D mide temperatura y humedad en dos conversiones. Se esperan los
 * tiempos del SHT21, el más lento de la familia —85 ms a 14 bits y 29 a 12—,
 * así el mismo código sirve para los tres. Es una máquina de estados, para
 * no frenar los cuadros de la cara mientras el sensor convierte. */
typedef enum { HTU_QUIETO, HTU_TEMPERATURA, HTU_HUMEDAD } htu_paso_t;
static htu_paso_t g_htu = HTU_QUIETO;
static uint32_t   g_htu_t0;
static bool       g_htu_t_ok;

static rk_color_t *g_franja;      /* una franja de texto, ancho x alto */
static int         g_franja_alto;

/* Medidas de rendimiento, para el serie. */
static uint32_t g_cuadros, g_render_us, g_t_medida_fps;

/* ------------------------------------------------------------- nombres -- */
static const char *animo_corto(rk_mood_t m)
{
    switch (m) {
    case RK_MOOD_HAPPY:       return "CONTENTO";
    case RK_MOOD_THIRSTY:     return "SED";
    case RK_MOOD_DROWNING:    return "SE AHOGA";
    case RK_MOOD_COLD:        return "FRIO";
    case RK_MOOD_HOT:         return "CALOR";
    case RK_MOOD_SCORCHED:    return "SOL DIRECTO";
    case RK_MOOD_DARK:        return "OSCURO";
    case RK_MOOD_PARCHED_AIR: return "AIRE SECO";
    case RK_MOOD_SLEEPING:    return "DORMIDO";
    case RK_MOOD_OFFLINE:     return "DESCONECTADO";
    default:                  return "SIN DATOS";
    }
}

static rk_mood_t animo_en_pantalla(void)
{
    return g_modo == MODO_AUTO ? g_veredicto.mood : DEMO[g_modo];
}

/* ---------------------------------------------------------------- I2C --- */
static bool i2c_escribir(uint8_t dir, uint8_t b)
{
    Wire.beginTransmission(dir);
    Wire.write(b);
    return Wire.endTransmission() == 0;
}

static bool i2c_leer(uint8_t dir, uint8_t *b, size_t n)
{
    if (Wire.requestFrom((int)dir, (int)n) != (int)n) {
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        b[i] = (uint8_t)Wire.read();
    }
    return true;
}

static void i2c_escanear(void)
{
    int encontrados = 0;

    Serial.print("  I2C (SDA ");
    Serial.print(RK_PIN_SDA);
    Serial.print(", SCL ");
    Serial.print(RK_PIN_SCL);
    Serial.print("):");
    for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            Serial.printf(" 0x%02X", a);
            if (a == RK_I2C_SHT21) {
                Serial.print(" (HTU21D)");
            }
            encontrados++;
        }
    }
    if (encontrados == 0) {
        Serial.print(" nada. Mirar SDA/SCL y los 3,3 V del HTU21D");
    }
    Serial.println();
    Wire.beginTransmission(RK_I2C_SHT21);
    g_htu_presente = Wire.endTransmission() == 0;
}

/* Un paso de la máquina del HTU21D; se llama en cada cuadro. */
static void htu_paso(void)
{
    uint32_t ahora = millis();

    switch (g_htu) {
    case HTU_QUIETO:
        break;
    case HTU_TEMPERATURA:
        if (ahora - g_htu_t0 >= 90u) {
            g_htu_t_ok = i2c_leer(RK_I2C_SHT21, g_sht, 3) && i2c_escribir(RK_I2C_SHT21, 0xF5);
            g_htu_t0 = ahora;
            g_htu = g_htu_t_ok ? HTU_HUMEDAD : HTU_QUIETO;
            if (!g_htu_t_ok) {
                g_htu_ok = false;
            }
        }
        break;
    case HTU_HUMEDAD:
        if (ahora - g_htu_t0 >= 35u) {
            g_htu_ok = i2c_leer(RK_I2C_SHT21, &g_sht[3], 3);
            g_htu = HTU_QUIETO;
        }
        break;
    }
}

static void htu_disparar(void)
{
    /* "No hold": el bus queda libre mientras el sensor convierte. */
    if (g_htu == HTU_QUIETO) {
        if (i2c_escribir(RK_I2C_SHT21, 0xF3)) {
            g_htu = HTU_TEMPERATURA;
            g_htu_t0 = millis();
        } else {
            g_htu_ok = false;
        }
    }
}

/* ---------------------------------------------------------- analógicos -- */
static void muestrear(void)
{
    g_suelo[g_i_muestra] = (uint16_t)analogRead(RK_PIN_SUELO_ADC);
    g_luz_mv[g_i_muestra] = (uint16_t)analogReadMilliVolts(RK_PIN_LUZ_ADC);
    g_luz_raw[g_i_muestra] = (uint16_t)analogRead(RK_PIN_LUZ_ADC);
    g_i_muestra = (g_i_muestra + 1) % N_MUESTRAS;
    g_luz_do = digitalRead(RK_PIN_LUZ_DIG) == HIGH;
}

/* ------------------------------------------------------------- medir ----- */
/* Arma la telemetría con la misma conversión que el producto
 * (rk_sensores_telemetria) y le suma la luz de la LDR; después, el ánimo. */
static void medir(void)
{
    rk_crudos_t c;

    memset(&c, 0, sizeof c);
    memcpy(c.suelo, g_suelo, sizeof c.suelo);
    c.n_suelo = N_MUESTRAS;
    if (g_htu_ok) {
        memcpy(c.sht, g_sht, sizeof c.sht);
        c.sht_leido = true;
    }
    rk_sensores_telemetria(&c, &g_cal, &g_tel);

    g_suelo_raw = g_tel.suelo_raw;
    g_luz_ao_mv = rk_soil_median(g_luz_mv, N_MUESTRAS);
    g_luz_saturada = rk_soil_median(g_luz_raw, N_MUESTRAS) < LUZ_PISO_RAW;
    g_tel.lux = g_luz_saturada ? LUZ_SATURADA : rk_ldr_lux(g_luz_ao_mv, VCC_MV);
    g_tel.fallas &= (uint8_t)~RK_FALLA_LUZ;
    g_tel.usb = true;                  /* el banco va enchufado            */
    g_tel.batt_mv = 0u;

    g_veredicto = rk_mood_eval(&g_mst, g_especie, &g_tel);
}

/* "23.4" o "-1.5": las décimas de grado, con el signo bien puesto. */
static void grados(char *s, size_t n, int16_t dc)
{
    int v = dc < 0 ? -(int)dc : (int)dc;
    snprintf(s, n, "%s%d.%d", dc < 0 ? "-" : "", v / 10, v % 10);
}

/* ------------------------------------------------------------- franjas --- */
static void franja_linea(rk_fb_t *fb, int y, const char *rotulo, const char *valor,
                         rk_color_t col_valor)
{
    int x = 6;
    x += rk_text(fb, x, y, rotulo, COL_ROTULO, 2);
    rk_text(fb, x + 10, y, valor, col_valor, 2);
}

/* Las dos franjas: arriba el suelo y la luz, abajo el aire y el ánimo. */
static void dibujar_franjas(void)
{
    rk_fb_t fb;
    char a[40], b[40];
    const int alto = g_franja_alto;
    const int l1 = (alto - 30) / 2 < 2 ? 2 : (alto - 30) / 2, l2 = l1 + 16;

    if (g_franja == NULL || alto < 30) {
        return;
    }
    rk_fb_init(&fb, g_franja, RK_TFT_W, alto);

    /* Arriba: suelo y luz. */
    rk_fb_clear(&fb, COL_FRANJA);
    if (g_tel.fallas & RK_FALLA_SUELO) {
        snprintf(a, sizeof a, "?? (%u)", g_suelo_raw);
        franja_linea(&fb, l1, "SUELO", a, COL_FALLA);
    } else {
        snprintf(a, sizeof a, "%u%% (%u)", g_tel.soil_pct, g_suelo_raw);
        franja_linea(&fb, l1, "SUELO", a, COL_VALOR);
    }
    if (g_luz_saturada) {
        snprintf(b, sizeof b, "MUCHA DO:%d", g_luz_do ? 1 : 0);
    } else {
        snprintf(b, sizeof b, "%lu LX DO:%d", (unsigned long)g_tel.lux, g_luz_do ? 1 : 0);
    }
    franja_linea(&fb, l2, "LUZ", b, COL_VALOR);
    pantalla_rect(0, 0, RK_TFT_W, alto, g_franja);

    /* Abajo: el aire y lo que la cara está mostrando. */
    rk_fb_clear(&fb, COL_FRANJA);
    if (g_tel.fallas & RK_FALLA_AIRE) {
        franja_linea(&fb, l1, "AIRE", g_htu_presente ? "NO MIDE" : "SIN HTU21D", COL_FALLA);
    } else {
        char t[12];
        grados(t, sizeof t, g_tel.temp_dc);
        snprintf(a, sizeof a, "%s~C %u%%", t, g_tel.rh_pct);
        franja_linea(&fb, l1, "AIRE", a, COL_VALOR);
    }
    if (g_modo == MODO_AUTO) {
        franja_linea(&fb, l2, "AUTO", animo_corto(g_veredicto.mood), COL_VALOR);
    } else {
        snprintf(a, sizeof a, "%d/%d", g_modo + 1, N_DEMO);
        snprintf(b, sizeof b, "%s", animo_corto(DEMO[g_modo]));
        franja_linea(&fb, l2, a, b, rk_mix(COL_VALOR, RK_RGB(255, 255, 255), 90));
    }
    pantalla_rect(0, RK_TFT_H - alto, RK_TFT_W, alto, g_franja);
}

/* ------------------------------------------------------------ el serie --- */
static void ayuda(void)
{
    Serial.println();
    Serial.println("  BOOT corto: siguiente modo (AUTO y los 11 animos). BOOT largo: piel.");
    Serial.println("  a  AUTO: la cara sigue a los sensores");
    Serial.println("  n  siguiente animo;  1-9, 0 y x  uno en particular:");
    Serial.println("     1 contento  2 agua  3 calor  4 oscuro  5 aire seco  6 sed");
    Serial.println("     7 frio  8 sol  9 dormido  0 sin datos  x desconectado");
    Serial.println("  p  siguiente piel (comun, rara, epica)");
    Serial.println("  e  siguiente especie (cambia los umbrales del animo)");
    Serial.println("  s  calibrar suelo SECO: el sensor al aire, limpio");
    Serial.println("  m  calibrar suelo MOJADO: el sensor en un vaso de agua, hasta la raya");
    Serial.println("  r  volver a la calibracion de fabrica");
    Serial.println("  i  escanear el I2C;  ?  esta ayuda");
    Serial.println();
}

static void informar(void)
{
    Serial.printf("suelo %4u (%s%3u%%)  luz AO %4u mV ~%5lu lx%s DO %d  ",
                  g_suelo_raw, (g_tel.fallas & RK_FALLA_SUELO) ? "??" : "",
                  g_tel.soil_pct, g_luz_ao_mv, (unsigned long)g_tel.lux,
                  g_luz_saturada ? " (saturada)" : "", g_luz_do ? 1 : 0);
    if (g_tel.fallas & RK_FALLA_AIRE) {
        Serial.print("aire: sin lectura  ");
    } else {
        char t[12];
        grados(t, sizeof t, g_tel.temp_dc);
        Serial.printf("aire %s C %u%%  ", t, g_tel.rh_pct);
    }
    Serial.printf("| %s: %s", g_especie->nombre, rk_mood_name(g_veredicto.mood));
    Serial.printf(" (%s)", g_veredicto.reason);
    if (g_modo != MODO_AUTO) {
        Serial.printf("  | pantalla: %s", rk_mood_name(DEMO[g_modo]));
    }
    Serial.println();
}

static void guardar_cal(void)
{
    g_nvs.putUShort("seco", g_cal.dry_raw);
    g_nvs.putUShort("mojado", g_cal.wet_raw);
}

static void cambiar_modo(int modo)
{
    g_modo = modo;
    Serial.printf("  modo: %s\n", modo == MODO_AUTO ? "AUTO (los sensores)"
                                                    : rk_mood_name(DEMO[modo]));
    dibujar_franjas();
}

static void cambiar_piel(void)
{
    g_rareza = (uint8_t)((g_rareza + 1u) % RK_RAREZA_COUNT);
    Serial.printf("  piel: %s (%s)\n", rk_persona_piel(g_kip, g_rareza)->nombre,
                  rk_rareza_nombre((rk_rareza_t)g_rareza));
}

static void comando(int c)
{
    switch (c) {
    case 'a': case 'A':
        cambiar_modo(MODO_AUTO);
        break;
    case 'n': case 'N':
        cambiar_modo(g_modo + 1 >= N_DEMO ? MODO_AUTO : g_modo + 1);
        break;
    case 'x': case 'X':
        cambiar_modo(10);
        break;
    case 'p': case 'P':
        cambiar_piel();
        break;
    case 'e': case 'E':
        /* -1 es la del banco; después, la tabla de core/species.c. */
        g_especie_idx = g_especie_idx + 1 >= rk_species_count ? -1 : g_especie_idx + 1;
        g_especie = g_especie_idx < 0 ? &ESPECIE_BANCO : &rk_species_table[g_especie_idx];
        rk_mood_state_init(&g_mst);
        Serial.printf("  especie: %s (suelo %u-%u%%, %d-%d C, HR >= %u%%, luz %lu-%lu lx)\n",
                      g_especie->nombre, g_especie->soil_min, g_especie->soil_max,
                      g_especie->temp_min_dc / 10, g_especie->temp_max_dc / 10,
                      g_especie->rh_min, (unsigned long)g_especie->lux_min,
                      (unsigned long)g_especie->lux_max);
        break;
    case 's': case 'S':
        g_cal.dry_raw = g_suelo_raw;
        Serial.printf("  suelo SECO = %u%s\n", g_cal.dry_raw,
                      rk_soil_cal_valid(&g_cal) ? "" : "  (falta MOJADO, o quedaron muy cerca)");
        guardar_cal();
        break;
    case 'm': case 'M':
        g_cal.wet_raw = g_suelo_raw;
        Serial.printf("  suelo MOJADO = %u%s\n", g_cal.wet_raw,
                      rk_soil_cal_valid(&g_cal) ? "" : "  (falta SECO, o quedaron muy cerca)");
        guardar_cal();
        break;
    case 'r': case 'R':
        g_cal = RK_SOIL_CAL_DEFAULT;
        guardar_cal();
        Serial.printf("  suelo: calibracion de fabrica (%u seco, %u mojado)\n",
                      g_cal.dry_raw, g_cal.wet_raw);
        break;
    case 'i': case 'I':
        i2c_escanear();
        break;
    case '?': case 'h': case 'H':
        ayuda();
        break;
    default:
        if (c >= '1' && c <= '9') {
            cambiar_modo(c - '1');
        } else if (c == '0') {
            cambiar_modo(9);
        }
        break;
    }
}

/* ------------------------------------------------------------- el botón -- */
static void boton(void)
{
    static bool antes = false;
    static uint32_t t_apretado;
    static bool largo_hecho;
    bool ahora = digitalRead(RK_PIN_BOTON) == LOW;
    uint32_t t = millis();

    if (ahora && !antes) {
        t_apretado = t;
        largo_hecho = false;
    } else if (ahora && !largo_hecho && t - t_apretado >= BOTON_LARGO_MS) {
        largo_hecho = true;
        cambiar_piel();
    } else if (!ahora && antes && !largo_hecho && t - t_apretado >= 30u) {
        cambiar_modo(g_modo + 1 >= N_DEMO ? MODO_AUTO : g_modo + 1);
    }
    antes = ahora;
}

/* ------------------------------------------------------ setup y loop ---- */
void setup(void)
{
    Serial.begin(115200);
    delay(300);
    Serial.println();
    Serial.println("ROOTKIT / firmware de banco " BANCO_VERSION " / " RK_PLACA_NOMBRE
                   " + " RK_PANTALLA_NOMBRE);

    pinMode(RK_PIN_BOTON, INPUT_PULLUP);
    pinMode(RK_PIN_LUZ_DIG, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(RK_PIN_SUELO_ADC, ADC_11db);
    analogSetPinAttenuation(RK_PIN_LUZ_ADC, ADC_11db);
    Wire.begin(RK_PIN_SDA, RK_PIN_SCL, 100000);
    /* El HTU21D: reset por software, y a los 15 ms está listo. */
    i2c_escribir(RK_I2C_SHT21, 0xFE);
    delay(20);
    i2c_escanear();

    g_nvs.begin("banco", false);
    g_cal.dry_raw = g_nvs.getUShort("seco", RK_SOIL_CAL_DEFAULT.dry_raw);
    g_cal.wet_raw = g_nvs.getUShort("mojado", RK_SOIL_CAL_DEFAULT.wet_raw);
    Serial.printf("  suelo calibrado: %u seco, %u mojado%s\n", g_cal.dry_raw, g_cal.wet_raw,
                  rk_soil_cal_valid(&g_cal) ? "" : " (invalida: se usa la de fabrica)");
    if (!rk_soil_cal_valid(&g_cal)) {
        g_cal = RK_SOIL_CAL_DEFAULT;
    }

    g_kip = rk_persona_find("kip");
    g_especie_idx = -1;
    g_especie = &ESPECIE_BANCO;
    rk_mood_state_init(&g_mst);

    Serial.printf("  memoria: %u libres, bloque mas grande %u\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    if (!pantalla_iniciar(100)) {
        Serial.println("  PANTALLA: no hubo memoria para el cuadro. Nada que dibujar.");
    } else {
        Serial.printf("  pantalla: cara de %dx%d en (%d, %d)\n", pantalla_lado(),
                      pantalla_lado(), pantalla_x0(), pantalla_y0());
        g_franja_alto = pantalla_y0();
        if (g_franja_alto >= 30) {
            g_franja = (rk_color_t *)malloc((size_t)RK_TFT_W * (size_t)g_franja_alto
                                            * sizeof(rk_color_t));
            if (g_franja != NULL) {
                pantalla_reservar_franjas(true);
            }
        }
    }

    for (int i = 0; i < N_MUESTRAS; i++) {
        muestrear();
    }
    htu_disparar();
    delay(100);
    htu_paso();
    delay(50);
    htu_paso();
    medir();
    rk_cara_anim_iniciar(&g_anim, animo_en_pantalla(), millis());
    ayuda();
    informar();
    dibujar_franjas();
    g_t_medida_fps = millis();
}

void loop(void)
{
    static uint32_t t_medir, t_cuadro;
    uint32_t ahora = millis();

    while (Serial.available() > 0) {
        comando(Serial.read());
    }
    boton();
    muestrear();
    htu_paso();

    if (ahora - t_medir >= MEDIR_MS) {
        t_medir = ahora;
        medir();
        htu_disparar();                  /* la próxima lectura del aire */
        informar();
        dibujar_franjas();
    }

    if (ahora - t_cuadro >= CUADRO_MS) {
        rk_fb_t *fb = pantalla_fb();
        uint32_t t0 = micros();
        t_cuadro = ahora;
        if (fb->px != NULL) {
            rk_cara_anim_animo(&g_anim, animo_en_pantalla(), ahora);
            rk_cara_anim_draw(fb, g_kip, g_rareza, &g_anim, g_veredicto.severity, 0u, 0u,
                              ahora);
            g_render_us += micros() - t0;
            pantalla_presentar(fb->px[0]);
            g_cuadros++;
        }
    }

    if (ahora - g_t_medida_fps >= 10000u && g_cuadros > 0u) {
        Serial.printf("  %lu cuadros/s, %lu ms por cara, %u de memoria libre\n",
                      (unsigned long)(g_cuadros * 1000u / (ahora - g_t_medida_fps)),
                      (unsigned long)(g_render_us / 1000u / g_cuadros),
                      (unsigned)ESP.getFreeHeap());
        g_cuadros = 0;
        g_render_us = 0;
        g_t_medida_fps = ahora;
    }
}
