/* main.c — simulador de escritorio del ROOTKIT.
 *
 *   ./build/rootkit_sim               los seis modelos en vivo, lado a lado
 *   ./build/rootkit_sim --sheet F     6 modelos x 11 animos
 *   ./build/rootkit_sim --etapas F    las 5 etapas de crecimiento
 *   ./build/rootkit_sim --despertar F los ojos se abren, por modelo
 *   ./build/rootkit_sim --shot F [M] [T]  un cuadro suelto
 *   ./build/rootkit_sim --pantallas F QR, dormida, despertar y cara, en los dos paneles
 *   ./build/rootkit_sim --sprites DIR [LADO]  una imagen por modelo y animo, para la app
 *   ./build/rootkit_sim --bench       costo de renderizar
 *
 * Todos los modos de captura menos el interactivo funcionan sin SDL: sirven
 * para revisar el arte en cualquier lado y para dejar capturas en el repo.
 *
 * LA VENTANA MUESTRA LOS SEIS MODELOS A LA VEZ
 *
 * Es lo unico que permite juzgar lo que hay que juzgar: si los seis se leen
 * como el MISMO producto y como SEIS personajes distintos al mismo tiempo.
 * Con una cara por vez las dos cosas son imposibles de evaluar, porque el
 * parecido y la diferencia solo existen en comparacion.
 */
#define _POSIX_C_SOURCE 199309L   /* clock_gettime y struct timespec */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../gfx/fb.h"
#include "../gfx/font.h"
#include "../gfx/panel.h"
#include "../ui/cara.h"
#include "../ui/despertar.h"
#include "../ui/qr.h"
#include "../art/face.h"
#include "../core/mood.h"
#include "../core/node.h"
#include "../core/persona.h"

#define SIM_ESC 2     /* aumento de la ventana: 1 pixel de panel = 2 de PC */

/* ------------------------------------------------------------- BMP ------- */
static int save_bmp(const char *path, const rk_color_t *px, int w, int h)
{
    int      pad = (4 - (w * 3) % 4) % 4;
    int      size = 54 + (w * 3 + pad) * h;
    FILE    *f = fopen(path, "wb");
    unsigned char hdr[54];
    int x, y;

    if (f == NULL) {
        return -1;
    }
    memset(hdr, 0, sizeof hdr);
    hdr[0] = 'B'; hdr[1] = 'M';
    hdr[2] = (unsigned char)(size);        hdr[3] = (unsigned char)(size >> 8);
    hdr[4] = (unsigned char)(size >> 16);  hdr[5] = (unsigned char)(size >> 24);
    hdr[10] = 54;
    hdr[14] = 40;
    hdr[18] = (unsigned char)(w);          hdr[19] = (unsigned char)(w >> 8);
    hdr[20] = (unsigned char)(w >> 16);    hdr[21] = (unsigned char)(w >> 24);
    hdr[22] = (unsigned char)(h);          hdr[23] = (unsigned char)(h >> 8);
    hdr[24] = (unsigned char)(h >> 16);    hdr[25] = (unsigned char)(h >> 24);
    hdr[26] = 1;
    hdr[28] = 24;
    fwrite(hdr, 1, sizeof hdr, f);

    for (y = h - 1; y >= 0; y--) {
        for (x = 0; x < w; x++) {
            rk_color_t c = px[y * w + x];
            unsigned char bgr[3];
            bgr[0] = (unsigned char)(((c        & 0x1F) * 255 + 15) / 31);
            bgr[1] = (unsigned char)((((c >> 5) & 0x3F) * 255 + 31) / 63);
            bgr[2] = (unsigned char)((((c >> 11) & 0x1F) * 255 + 15) / 31);
            fwrite(bgr, 1, 3, f);
        }
        for (x = 0; x < pad; x++) {
            fputc(0, f);
        }
    }
    fclose(f);
    return 0;
}

static void pegar(rk_fb_t *dst, const rk_color_t *src, int sw, int sh,
                  int ox, int oy)
{
    int x, y;
    for (y = 0; y < sh; y++) {
        for (x = 0; x < sw; x++) {
            rk_px(dst, ox + x, oy + y, src[y * sw + x]);
        }
    }
}

/* --------------------------------------------------- mundo simulado ------ */
typedef struct {
    int soil_x10;
    int temp_dc;
    int rh;
    int dry_rate;
    int lux_peak;
} env_t;

static rk_roster_t g_kit;
static env_t       g_env[RK_MAX_NODES];
static rk_mood_t   g_force = RK_MOOD_COUNT;   /* COUNT = no forzar */

/* Un nodo por modelo: la ventana muestra los seis a la vez, cada uno con su
 * planta y su vinculo de distinta edad. */
static void world_init(void)
{
    static const struct {
        const char *nom; const char *sp;
        int soil, t, rh, dry, lux, dias;
    } SEED[6] = {
        { "MONSTERA", "monstera",     42, 235, 58, 4,  6200,  95 },
        { "POTUS",    "pothos",       33, 228, 46, 6,  3100,  34 },
        { "CACTUS",   "cactus",       12, 262, 24, 2, 26000,   9 },
        { "BONSAI",   "bonsai",       38, 222, 52, 5,  9000, 200 },
        { "HELECHO",  "helecho",      56, 219, 68, 7,  1800,  62 },
        { "ORQUIDEA", "orquidea",     44, 240, 62, 4,  4200, 140 },
    };
    int i, d;

    rk_roster_init(&g_kit);
    g_kit.wifi = true;

    for (i = 0; i < 6 && i < rk_persona_count; i++) {
        int k = rk_roster_add(&g_kit, SEED[i].nom,
                              rk_species_find(SEED[i].sp),
                              rk_persona_at(i));
        if (k < 0) {
            continue;
        }
        for (d = 0; d < SEED[i].dias; d++) {
            rk_bond_dia(&g_kit.nodes[k].bond, true);
        }
        g_env[k].soil_x10 = SEED[i].soil * 10;
        g_env[k].temp_dc  = SEED[i].t;
        g_env[k].rh       = SEED[i].rh;
        g_env[k].dry_rate = SEED[i].dry;
        g_env[k].lux_peak = SEED[i].lux;
    }
}

#define DAY_MS 48000u

static void world_tick(uint32_t t_ms)
{
    int i;
    int phase = (int)((t_ms % DAY_MS) * 256 / DAY_MS);
    int sun   = rk_sin8((uint8_t)phase);

    for (i = 0; i < g_kit.count; i++) {
        rk_node_t *n = &g_kit.nodes[i];
        env_t     *e = &g_env[i];

        e->soil_x10 -= e->dry_rate;
        if (e->soil_x10 < 0) {
            e->soil_x10 = 0;
        }
        n->tel.valid    = true;
        n->tel.age_s    = 60;
        n->tel.soil_pct = (uint8_t)(e->soil_x10 / 10);
        n->tel.temp_dc  = (int16_t)(e->temp_dc + sun * 35 / 127);
        n->tel.rh_pct   = (uint8_t)(e->rh - sun * 8 / 127);
        n->tel.lux      = sun > 0 ? (uint32_t)(sun * e->lux_peak / 127) : 0u;
        n->tel.batt_mv  = (uint16_t)(3560 + i * 110 +
                                     rk_sin8((uint8_t)(t_ms / 900)) * 240 / 127);
    }

    rk_roster_eval(&g_kit);

    if (g_force != RK_MOOD_COUNT) {
        for (i = 0; i < g_kit.count; i++) {
            rk_node_t *n = &g_kit.nodes[i];
            n->verdict.mood     = g_force;
            n->verdict.severity = RK_SEV_WATCH;
            n->verdict.reason   = rk_mood_reason(g_force);
        }
    }
}

/* ------------------------------------------------- modos de captura ------ */
static void asentar(uint32_t t_ms)
{
    int i;
    world_init();
    for (i = 0; i < 40; i++) {
        world_tick(t_ms);
    }
}

/* Hoja de contacto generica. */
typedef void (*celda_fn)(rk_color_t *px, int i, uint32_t t_ms);

static int hoja(const char *path, int n, int cols, int w, int h,
                celda_fn dibujar, const char *(*etiqueta)(int),
                rk_color_t (*borde)(int))
{
    enum { PAD = 8, LAB = 11 };
    int cw = w + PAD, ch = h + PAD + LAB;
    int filas = (n + cols - 1) / cols;
    int W = cols * cw + PAD, H = filas * ch + PAD;
    rk_color_t *sheet = calloc((size_t)W * H, sizeof(rk_color_t));
    rk_color_t *cell  = calloc((size_t)w * h, sizeof(rk_color_t));
    rk_fb_t sfb;
    int i;

    if (sheet == NULL || cell == NULL) {
        free(sheet);
        free(cell);
        return 1;
    }
    rk_fb_init(&sfb, sheet, W, H);
    rk_fb_clear(&sfb, RK_RGB(12, 14, 13));

    for (i = 0; i < n; i++) {
        int ox = PAD + (i % cols) * cw;
        int oy = PAD + (i / cols) * ch + LAB;

        memset(cell, 0, (size_t)w * h * sizeof(rk_color_t));
        dibujar(cell, i, 1200u + (uint32_t)i * 137u);
        pegar(&sfb, cell, w, h, ox, oy);
        rk_rect(&sfb, ox - 1, oy - 1, w + 2, h + 2,
                borde ? borde(i) : RK_RGB(60, 70, 62));
        if (etiqueta != NULL) {
            rk_text(&sfb, ox, oy - LAB, etiqueta(i), RK_RGB(227, 165, 74), 1);
        }
    }

    if (save_bmp(path, sheet, W, H) != 0) {
        fprintf(stderr, "no pude escribir %s\n", path);
        free(sheet);
        free(cell);
        return 1;
    }
    printf("%dx%d -> %s\n", W, H, path);
    free(sheet);
    free(cell);
    return 0;
}

/* --- 6 modelos x 11 animos: la lamina que decide si el rig funciona ------ */
static void cel_sheet(rk_color_t *px, int i, uint32_t t_ms)
{
    rk_fb_t fb;
    int persona = i / RK_MOOD_COUNT;
    int mood    = i % RK_MOOD_COUNT;
    rk_severity_t sev = (mood == RK_MOOD_THIRSTY) ? RK_SEV_URGENT
                      : (mood == RK_MOOD_HAPPY || mood == RK_MOOD_SLEEPING)
                        ? RK_SEV_OK : RK_SEV_WATCH;

    rk_fb_init(&fb, px, RK_MINI_W, RK_MINI_H);
    rk_face_draw(&fb, rk_persona_at(persona), (rk_mood_t)mood, sev, 0u, t_ms);
}

static const char *et_sheet(int i)
{
    return (i % RK_MOOD_COUNT == 0)
           ? rk_persona_at(i / RK_MOOD_COUNT)->nombre
           : rk_mood_name((rk_mood_t)(i % RK_MOOD_COUNT));
}

/* --- las cinco etapas, por modelo ---------------------------------------- */
static void cel_etapas(rk_color_t *px, int i, uint32_t t_ms)
{
    rk_fb_t fb;
    int persona = i / RK_ETAPA_COUNT;
    int etapa   = i % RK_ETAPA_COUNT;

    rk_fb_init(&fb, px, RK_MINI_W, RK_MINI_H);
    rk_face_draw(&fb, rk_persona_at(persona), RK_MOOD_HAPPY, RK_SEV_OK,
                 rk_face_adornos_etapa(etapa), t_ms);
}

static const char *et_etapas(int i)
{
    return (i % RK_ETAPA_COUNT == 0)
           ? rk_persona_at(i / RK_ETAPA_COUNT)->nombre
           : rk_stage_name((rk_stage_t)(i % RK_ETAPA_COUNT));
}

/* --- el despertar --------------------------------------------------------- */
static const uint32_t MOM[6] = { 250u, 800u, 1150u, 1400u, 1700u, 2400u };

static void cel_despertar(rk_color_t *px, int i, uint32_t t_ms)
{
    rk_fb_t fb;
    (void)t_ms;
    rk_fb_init(&fb, px, RK_MINI_W, RK_MINI_H);
    rk_despertar_draw(&fb, rk_persona_at(i / 6), MOM[i % 6]);
}

static const char *et_despertar(int i)
{
    return (i % 6 == 0) ? rk_persona_at(i / 6)->nombre : "";
}

static rk_color_t bd_despertar(int i)
{
    return rk_rarity_color(rk_persona_at(i / 6)->rareza);
}

/* --- un cuadro suelto ----------------------------------------------------- */
static int do_shot(const char *path, const char *mood_name, uint32_t t_ms)
{
    rk_color_t *px = calloc(RK_MINI_PX, sizeof(rk_color_t));
    rk_fb_t fb;
    rk_mood_t m = RK_MOOD_HAPPY;
    int i;

    if (px == NULL) {
        return 1;
    }
    if (mood_name != NULL) {
        for (i = 0; i < RK_MOOD_COUNT; i++) {
            if (strcmp(rk_mood_name((rk_mood_t)i), mood_name) == 0) {
                m = (rk_mood_t)i;
                break;
            }
        }
    }
    asentar(t_ms);
    g_kit.nodes[0].verdict.mood = m;
    rk_fb_init(&fb, px, RK_MINI_W, RK_MINI_H);
    rk_cara_draw(&fb, &g_kit.nodes[0], t_ms);
    if (save_bmp(path, px, RK_MINI_W, RK_MINI_H) != 0) {
        free(px);
        return 1;
    }
    printf("cuadro %dx%d -> %s\n", RK_MINI_W, RK_MINI_H, path);
    free(px);
    return 0;
}

/* --- las pantallas del vínculo: QR, dormida, despertar y cara --------------
 * Arriba el panel de 1,44"; abajo el de 2,2". Es la lámina que muestra todo lo
 * que la pantalla del ROOTKIT puede llegar a mostrar en su vida. */
static int do_pantallas(const char *path)
{
    enum { PAD = 10 };
    static rk_color_t chico[RK_MINI_PX];
    static rk_color_t grande[RK_PRIME_PX];
    static rk_qr_t q;
    int W = PAD + 4 * (RK_PRIME_W + PAD);
    int H = PAD + RK_MINI_H + PAD + 12 + RK_PRIME_H + PAD + 12;
    rk_color_t *lienzo = calloc((size_t)W * (size_t)H, sizeof(rk_color_t));
    rk_fb_t big, fb;
    rk_node_t *n;
    int i;
    static const char *ET[4] = { "QR: PORTAL", "QR: CONECTANDO", "QR: EN LINEA", "COFRE CERRADO" };

    if (lienzo == NULL) {
        return 1;
    }
    rk_fb_init(&big, lienzo, W, H);
    rk_fb_clear(&big, RK_RGB(12, 14, 13));
    rk_qr_preparar(&q, "HTTP://192.168.0.20:8080/V/K7Q2M9XA", "K7Q2M9XA");
    asentar(1200u);
    n = &g_kit.nodes[1];

    for (i = 0; i < 4; i++) {
        int ox = PAD + i * (RK_PRIME_W + PAD) + (RK_PRIME_W - RK_MINI_W) / 2;
        rk_fb_init(&fb, chico, RK_MINI_W, RK_MINI_H);
        if (i < 3) {
            rk_qr_draw(&fb, &q, (rk_qr_estado_t)i, 300u);
        } else {
            rk_cara_dormida(&fb, 1200u);
        }
        pegar(&big, chico, RK_MINI_W, RK_MINI_H, ox, PAD + 12);
        rk_text(&big, ox, PAD, ET[i], RK_RGB(227, 165, 74), 1);
    }
    for (i = 0; i < 4; i++) {
        int ox = PAD + i * (RK_PRIME_W + PAD);
        int oy = PAD + 12 + RK_MINI_H + PAD + 12;
        rk_fb_init(&fb, grande, RK_PRIME_W, RK_PRIME_H);
        switch (i) {
        case 0:  rk_qr_draw(&fb, &q, RK_QR_EN_LINEA, 300u); break;
        case 1:  rk_cara_dormida(&fb, 1200u); break;
        case 2:  rk_despertar_draw(&fb, n->persona, 1150u); break;
        default: rk_cara_draw(&fb, n, 1200u); break;
        }
        pegar(&big, grande, RK_PRIME_W, RK_PRIME_H, ox, oy);
        rk_text(&big, ox, oy - 12,
                i == 0 ? "2,2 QR" : i == 1 ? "2,2 DORMIDA" : i == 2 ? "2,2 DESPERTANDO" : "2,2 CARA",
                RK_RGB(227, 165, 74), 1);
    }
    if (save_bmp(path, lienzo, W, H) != 0) {
        free(lienzo);
        return 1;
    }
    printf("%dx%d -> %s\n", W, H, path);
    free(lienzo);
    return 0;
}

/* --- caras sueltas para la app ---------------------------------------------
 * Una imagen por modelo y ánimo, más la cara dormida. La app las usa donde
 * no corre el renderer en WebAssembly: íconos de notificación, la tarjeta
 * para compartir y la primera pintada antes de que cargue el módulo. */
static int do_sprites(const char *dir, int lado)
{
    rk_color_t *px;
    rk_fb_t fb;
    char path[512];
    int p, m;

    if (lado < 32 || lado > 512) {
        fprintf(stderr, "lado fuera de rango: %d\n", lado);
        return 1;
    }
    px = calloc((size_t)lado * (size_t)lado, sizeof(rk_color_t));
    if (px == NULL) {
        return 1;
    }
    rk_fb_init(&fb, px, lado, lado);
    for (p = 0; p < rk_persona_count; p++) {
        for (m = 0; m < RK_MOOD_COUNT; m++) {
            /* Un instante sin parpadeo ni gesto de alegría: la foto carnet. */
            rk_face_draw(&fb, rk_persona_at(p), (rk_mood_t)m, RK_SEV_OK,
                         rk_face_adornos_etapa(RK_ETAPA_JOVEN), 1200u);
            snprintf(path, sizeof path, "%s/%s-%s.bmp", dir, rk_persona_at(p)->id,
                     rk_mood_name((rk_mood_t)m));
            if (save_bmp(path, px, lado, lado) != 0) {
                fprintf(stderr, "no pude escribir %s\n", path);
                free(px);
                return 1;
            }
        }
    }
    rk_cara_dormida(&fb, 1200u);
    snprintf(path, sizeof path, "%s/incognito.bmp", dir);
    save_bmp(path, px, lado, lado);
    printf("%d caras de %dx%d -> %s\n", rk_persona_count * RK_MOOD_COUNT + 1, lado, lado, dir);
    free(px);
    return 0;
}

/* ------------------------------------------------------------- bench ----- */
static double ahora_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

#define BENCH_N 2000

static int do_bench(void)
{
    static rk_color_t px[RK_MINI_PX];
    rk_fb_t fb;
    double  t0, ms, base = 0.0;
    int     i, k;

    rk_fb_init(&fb, px, RK_MINI_W, RK_MINI_H);
    asentar(1200u);

    printf("PANEL %dx%d = %d px, %d KB por cuadro\n\n",
           RK_MINI_W, RK_MINI_H, RK_MINI_PX,
           RK_MINI_PX * (int)sizeof(rk_color_t) / 1024);

    printf("%-10s %11s %9s %8s\n", "MODELO", "MS/CUADRO", "FPS", "REL");
    for (k = 0; k < rk_persona_count; k++) {
        const rk_persona_t *p = rk_persona_at(k);

        for (i = 0; i < 100; i++) {
            rk_face_draw(&fb, p, RK_MOOD_HAPPY, RK_SEV_OK, 0u,
                         (uint32_t)i * 37u);
        }
        t0 = ahora_ms();
        for (i = 0; i < BENCH_N; i++) {
            rk_face_draw(&fb, p, RK_MOOD_HAPPY, RK_SEV_OK, 0u,
                         (uint32_t)i * 37u);
        }
        ms = (ahora_ms() - t0) / BENCH_N;
        if (k == 0) {
            base = ms;
        }
        printf("%-10s %11.4f %9.0f %7.2fx\n",
               p->nombre, ms, 1000.0 / ms, base > 0.0 ? ms / base : 1.0);
    }

    printf("\n%-26s %11s\n", "ETAPA", "MS");
    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_fb_clear(&fb, 0x1234);
    }
    printf("%-26s %11.4f\n", "clear", (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_vgradient(&fb, 0, 0, RK_MINI_W, RK_MINI_H, 0x1234, 0x4321);
    }
    printf("%-26s %11.4f\n", "gradiente del fondo",
           (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_elipse(&fb, 64, 56, 20, 16, 0x07E0);
    }
    printf("%-26s %11.4f\n", "una elipse de ojo",
           (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_despertar_draw(&fb, rk_persona_at(5), (uint32_t)(i * 3u) % RK_DESP_FIN_MS);
    }
    printf("%-26s %11.4f\n", "un cuadro del despertar",
           (ahora_ms() - t0) / BENCH_N);

    printf("\n  COTA DEL BUS\n");
    printf("  %-22s %8d B  a %4.0f MB/s = %5.2f ms (%3.0f fps techo)\n",
           "SPI 40 MHz", RK_MINI_PX * 2, 5.0,
           (RK_MINI_PX * 2.0) / (5.0 * 1024.0 * 1024.0) * 1000.0,
           1000.0 / ((RK_MINI_PX * 2.0) / (5.0 * 1024.0 * 1024.0) * 1000.0));
    printf("\n  El framebuffer son %d KB: entra en la SRAM de un ESP32-C3\n",
           RK_MINI_PX * 2 / 1024);
    printf("  (400 KB) sin tocar PSRAM, con lugar de sobra para un segundo\n");
    printf("  buffer y mandar por DMA mientras se dibuja el siguiente.\n\n");
    return 0;
}

/* ------------------------------------------------------- interactivo ----- */
#ifdef RK_WITH_SDL
#include <SDL2/SDL.h>

#define COLS   3
#define GAP    10
#define LW     (COLS * RK_MINI_W + (COLS + 1) * GAP)
#define LH     (2 * RK_MINI_H + 3 * GAP + 24)

static int run_window(void)
{
    static rk_color_t lienzo[LW * LH];
    static rk_color_t celda[RK_MINI_PX];
    rk_fb_t big, fb;
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    uint32_t     t0, t_ms = 0, last_tick = 0, rev_t0 = 0;
    int          speed = 1, running = 1, rev = 0, sel = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    win = SDL_CreateWindow("ROOTKIT / los seis modelos",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           LW * SIM_ESC, LH * SIM_ESC, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565,
                            SDL_TEXTUREACCESS_STREAMING, LW, LH);
    if (win == NULL || ren == NULL || tex == NULL) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }

    rk_fb_init(&big, lienzo, LW, LH);
    rk_fb_init(&fb, celda, RK_MINI_W, RK_MINI_H);
    world_init();
    t0 = SDL_GetTicks();

    puts("teclas: 1-6 seleccionar | W regar | M ciclar animo | R soltar animo");
    puts("        G primer encendido del seleccionado | +/- velocidad");
    puts("        S captura | ESC salir");

    while (running) {
        SDL_Event ev;
        int i;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE || k == SDLK_q) {
                    running = 0;
                } else if (k >= SDLK_1 && k <= SDLK_6) {
                    int nn = k - SDLK_1;
                    if (nn < g_kit.count) { sel = nn; }
                } else if (k == SDLK_w) {
                    g_env[sel].soil_x10 += 320;
                    if (g_env[sel].soil_x10 > 1000) {
                        g_env[sel].soil_x10 = 1000;
                    }
                } else if (k == SDLK_m) {
                    g_force = (rk_mood_t)((g_force + 1) % (RK_MOOD_COUNT + 1));
                } else if (k == SDLK_r) {
                    g_force = RK_MOOD_COUNT;
                } else if (k == SDLK_g) {
                    rev = 1;
                    rev_t0 = t_ms;
                } else if (k == SDLK_EQUALS || k == SDLK_PLUS) {
                    if (speed < 64) { speed *= 2; }
                } else if (k == SDLK_MINUS) {
                    if (speed > 1) { speed /= 2; }
                } else if (k == SDLK_s) {
                    save_bmp("captura.bmp", lienzo, LW, LH);
                    puts("captura.bmp escrito");
                }
            }
        }

        t_ms = (SDL_GetTicks() - t0) * (uint32_t)speed;
        if (t_ms - last_tick > 250u) {
            world_tick(t_ms);
            last_tick = t_ms;
        }

        rk_fb_clear(&big, RK_RGB(8, 10, 9));

        for (i = 0; i < g_kit.count && i < 6; i++) {
            int ox = GAP + (i % COLS) * (RK_MINI_W + GAP);
            int oy = GAP + (i / COLS) * (RK_MINI_H + GAP + 12);

            if (rev && i == sel && !rk_despertar_termino(t_ms - rev_t0)) {
                rk_despertar_draw(&fb, g_kit.nodes[i].persona, t_ms - rev_t0);
            } else {
                if (i == sel) { rev = 0; }
                rk_cara_draw(&fb, &g_kit.nodes[i], t_ms);
            }
            pegar(&big, celda, RK_MINI_W, RK_MINI_H, ox, oy);
            rk_rect(&big, ox - 1, oy - 1, RK_MINI_W + 2, RK_MINI_H + 2,
                    i == sel ? RK_RGB(72, 214, 190) : RK_RGB(50, 58, 52));
            rk_text(&big, ox, oy + RK_MINI_H + 2,
                    g_kit.nodes[i].persona->nombre,
                    i == sel ? RK_RGB(200, 230, 210) : RK_RGB(110, 124, 114),
                    1);
        }

        SDL_UpdateTexture(tex, NULL, lienzo, LW * (int)sizeof(rk_color_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
#else
static int run_window(void)
{
    fprintf(stderr, "compilado sin SDL: usa --sheet, --etapas o --despertar\n");
    return 1;
}
#endif

int main(int argc, char **argv)
{
    if (argc >= 3 && strcmp(argv[1], "--sheet") == 0) {
        return hoja(argv[2], rk_persona_count * RK_MOOD_COUNT, RK_MOOD_COUNT,
                    RK_MINI_W, RK_MINI_H, cel_sheet, et_sheet, NULL);
    }
    if (argc >= 3 && strcmp(argv[1], "--etapas") == 0) {
        return hoja(argv[2], rk_persona_count * RK_ETAPA_COUNT,
                    RK_ETAPA_COUNT, RK_MINI_W, RK_MINI_H,
                    cel_etapas, et_etapas, NULL);
    }
    if (argc >= 3 && strcmp(argv[1], "--despertar") == 0) {
        return hoja(argv[2], rk_persona_count * 6, 6, RK_MINI_W, RK_MINI_H,
                    cel_despertar, et_despertar, bd_despertar);
    }
    if (argc >= 2 && strcmp(argv[1], "--bench") == 0) {
        return do_bench();
    }
    if (argc >= 3 && strcmp(argv[1], "--pantallas") == 0) {
        return do_pantallas(argv[2]);
    }
    if (argc >= 3 && strcmp(argv[1], "--sprites") == 0) {
        return do_sprites(argv[2], argc >= 4 ? atoi(argv[3]) : 256);
    }
    if (argc >= 3 && strcmp(argv[1], "--shot") == 0) {
        return do_shot(argv[2],
                       argc >= 4 ? argv[3] : NULL,
                       argc >= 5 ? (uint32_t)strtoul(argv[4], NULL, 10) : 1200u);
    }
    return run_window();
}
