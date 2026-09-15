/* main.c — simulador de escritorio del ROOTKIT.
 *
 *   ./build/rootkit_sim              el kit entero: Prime + Minis, en vivo
 *   ./build/rootkit_sim --sheet F    hoja de contacto del Prime, todos los animos
 *   ./build/rootkit_sim --minis F    hoja de contacto del Mini, todos los animos
 *   ./build/rootkit_sim --brotes F   los doce brotes en sus cinco etapas
 *   ./build/rootkit_sim --gacha F    la ceremonia, una fila por rareza
 *   ./build/rootkit_sim --shot F [M] [T]   un cuadro suelto del Prime
 *   ./build/rootkit_sim --bench      costo de renderizar
 *
 * Todos los modos de captura menos el interactivo funcionan sin SDL: sirven
 * para revisar el arte en cualquier lado y para dejar capturas en el repo.
 *
 * LA VENTANA MUESTRA EL KIT, NO UNA PANTALLA
 *
 * El Prime a la izquierda y los Minis a la derecha, todos animandose con el
 * mismo reloj sobre el mismo mundo simulado. Es la unica forma de juzgar lo
 * que de verdad hay que juzgar: si la jerarquia entre el adulto de 22 mm y
 * el brote de 13 mm se lee, y si las dos pantallas cuentan la misma historia
 * sobre la misma planta.
 */
#define _POSIX_C_SOURCE 199309L   /* clock_gettime y struct timespec */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../gfx/fb.h"
#include "../gfx/font.h"
#include "../gfx/panel.h"
#include "../ui/prime.h"
#include "../ui/mini.h"
#include "../ui/gacha.h"
#include "../art/adulto.h"
#include "../art/brote.h"
#include "../core/mood.h"
#include "../core/node.h"
#include "../core/companion.h"

#define SIM_ESC 2     /* aumento de la ventana: 1 pixel de panel = 2 de PC */

/* ------------------------------------------------------------- BMP ------- */
/* Escritor propio de 24 bits: asi los modos de captura no dependen de SDL. */
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

    for (y = h - 1; y >= 0; y--) {          /* BMP va de abajo hacia arriba */
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

/* Copia un framebuffer chico dentro de uno grande. */
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
/* Cada maceta tiene un ambiente que deriva solo: la tierra se seca, el sol
 * sale y se pone. Sin esto el simulador muestra estados congelados y no se
 * puede juzgar si las transiciones se sienten bien. */
typedef struct {
    int soil_x10;     /* humedad en decimas, para que la deriva sea suave */
    int temp_dc;
    int rh;
    int dry_rate;     /* decimas de punto por ciclo                       */
    int lux_peak;
} env_t;

static rk_roster_t g_kit;
static env_t       g_env[RK_MAX_NODES];
static rk_mood_t   g_force = RK_MOOD_COUNT;   /* COUNT = no forzar */

static void world_init(void)
{
    static const struct {
        const char *nom; const char *sp; rk_role_t rol;
        int soil, t, rh, dry, lux, dias;
    } SEED[] = {
        { "MONSTERA", "monstera",    RK_ROLE_PRIME, 42, 235, 58, 4,  6200,  95 },
        { "POTUS",    "pothos",      RK_ROLE_MINI,  33, 228, 46, 6,  3100,  34 },
        { "CACTUS",   "cactus",      RK_ROLE_MINI,  12, 262, 24, 2, 26000,   9 },
        { "BONSAI",   "bonsai",      RK_ROLE_MINI,  38, 222, 52, 5,  9000, 200 },
    };
    int i, d;

    rk_roster_init(&g_kit);
    g_kit.wifi = true;

    for (i = 0; i < (int)(sizeof SEED / sizeof SEED[0]); i++) {
        int k = rk_roster_add(&g_kit, SEED[i].nom, SEED[i].rol,
                              rk_species_find(SEED[i].sp));
        if (k < 0) {
            continue;
        }
        /* Vinculos de distinta edad: asi la ventana muestra de una las
         * cinco etapas de crecimiento y no hay que esperar seis meses. */
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

/* Un "dia" simulado dura 48 s de reloj real: suficiente para ver el ciclo
 * completo sin esperar, y lento como para que no maree. */
#define DAY_MS 48000u

static void world_tick(uint32_t t_ms)
{
    int i;
    int phase = (int)((t_ms % DAY_MS) * 256 / DAY_MS);
    int sun   = rk_sin8((uint8_t)phase);          /* -127..127 */

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
        /* El Prime va enchufado; los Minis muestran celda de verdad. */
        n->tel.batt_mv  = (n->role == RK_ROLE_PRIME) ? 4200
                          : (uint16_t)(3600 + i * 120 +
                                       rk_sin8((uint8_t)(t_ms / 900)) * 220 / 127);
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

static void forzar(int nodo, rk_mood_t mood)
{
    rk_node_t *n;
    if (mood == RK_MOOD_COUNT || nodo >= g_kit.count) {
        return;
    }
    n = &g_kit.nodes[nodo];
    n->verdict.mood     = mood;
    n->verdict.severity = (mood == RK_MOOD_THIRSTY) ? RK_SEV_URGENT
                                                    : RK_SEV_WATCH;
    n->verdict.reason   = rk_mood_reason(mood);
}

static void render_prime(rk_color_t *px, rk_mood_t mood, uint32_t t_ms)
{
    rk_fb_t fb;
    rk_fb_init(&fb, px, RK_PRIME_W, RK_PRIME_H);
    asentar(t_ms);
    forzar(0, mood);
    rk_prime_draw(&fb, &g_kit, t_ms);
}

static void render_mini(rk_color_t *px, rk_mood_t mood, uint32_t t_ms)
{
    rk_fb_t fb;
    rk_fb_init(&fb, px, RK_MINI_W, RK_MINI_H);
    asentar(t_ms);
    forzar(1, mood);
    rk_mini_draw(&fb, &g_kit.nodes[1], t_ms);
}

/* Hoja de contacto generica: N celdas de w x h con su etiqueta arriba. */
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

static const char *et_mood(int i) { return rk_mood_name((rk_mood_t)i); }

static void cel_prime(rk_color_t *px, int i, uint32_t t_ms)
{
    render_prime(px, (rk_mood_t)i, t_ms);
}

static void cel_mini(rk_color_t *px, int i, uint32_t t_ms)
{
    render_mini(px, (rk_mood_t)i, t_ms);
}

/* Los doce simbiontes por sus cinco etapas: 60 brotes en una sola lamina.
 * Es la captura que hay que mirar para decidir si el crecimiento se nota. */
static void cel_brote(rk_color_t *px, int i, uint32_t t_ms)
{
    rk_fb_t fb;
    int comp = i / RK_ETAPA_COUNT;
    int et   = i % RK_ETAPA_COUNT;

    /* La celda es mas ancha que el cuerpo porque las hojas de crecimiento
     * salen hasta 22 px de arte a cada lado: con 72 se cortaban justo lo que
     * hay que mirar. */
    rk_fb_init(&fb, px, 96, 80);
    rk_fb_clear(&fb, RK_RGB(20, 26, 22));
    rk_brote_draw(&fb, 48, 40, &rk_companion_table[comp], RK_MOOD_HAPPY,
                  RK_SEV_OK, (rk_stage_t)et, RK_MINI_ART, t_ms);
}

static const char *et_brote(int i)
{
    return (i % RK_ETAPA_COUNT == 0)
           ? rk_companion_table[i / RK_ETAPA_COUNT].nombre
           : rk_stage_name((rk_stage_t)(i % RK_ETAPA_COUNT));
}

/* Ceremonia: una fila por rareza, una columna por momento. */
static const uint32_t MOM[5] = { 350u, 1200u, 2450u, 3200u, 4600u };
static const char *EJEMPLO[RK_RAR_COUNT] = { "myco", "tuga", "orqui", "bonz" };

static void cel_gacha(rk_color_t *px, int i, uint32_t t_ms)
{
    rk_fb_t fb;
    const rk_companion_t *c = rk_companion_find(EJEMPLO[i / 5]);
    (void)t_ms;
    rk_fb_init(&fb, px, RK_PRIME_W, RK_PRIME_H);
    rk_gacha_draw(&fb, c, MOM[i % 5]);
}

static const char *et_gacha(int i)
{
    return (i % 5 == 0)
           ? rk_rarity_name(rk_rarity_of(rk_companion_find(EJEMPLO[i / 5])))
           : "";
}

static rk_color_t bd_gacha(int i)
{
    return rk_rarity_color(rk_rarity_of(rk_companion_find(EJEMPLO[i / 5])));
}

static int do_shot(const char *path, const char *mood_name, uint32_t t_ms)
{
    rk_color_t *px = calloc(RK_PRIME_PX, sizeof(rk_color_t));
    rk_mood_t   m  = RK_MOOD_COUNT;
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
    render_prime(px, m, t_ms);
    if (save_bmp(path, px, RK_PRIME_W, RK_PRIME_H) != 0) {
        free(px);
        return 1;
    }
    printf("cuadro %dx%d -> %s\n", RK_PRIME_W, RK_PRIME_H, path);
    free(px);
    return 0;
}

/* ------------------------------------------------------------- bench ----- */
/* Mide el costo de renderizar. El numero que importa no es el absoluto de
 * esta PC sino la proporcion entre etapas: dice donde conviene optimizar
 * antes de tener las placas en la mano, y queda como linea de base para
 * comparar cuando lleguen. Un ESP32-C3 a 160 MHz es groseramente un orden de
 * magnitud mas lento que un x86 de escritorio. */
static double ahora_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

#define BENCH_N 2000

static void bus(const char *nombre, int px, double mbps)
{
    double bytes = (double)px * 2.0;
    printf("  %-22s %8.0f B  a %4.0f MB/s = %5.2f ms  (%3.0f fps techo)\n",
           nombre, bytes, mbps,
           bytes / (mbps * 1024.0 * 1024.0) * 1000.0,
           1000.0 / (bytes / (mbps * 1024.0 * 1024.0) * 1000.0));
}

static int do_bench(void)
{
    static rk_color_t pp[RK_PRIME_PX];
    static rk_color_t pm[RK_MINI_PX];
    rk_fb_t fbp, fbm;
    double  t0, ms, base = 0.0;
    int     i, m;

    rk_fb_init(&fbp, pp, RK_PRIME_W, RK_PRIME_H);
    rk_fb_init(&fbm, pm, RK_MINI_W, RK_MINI_H);
    asentar(1200u);

    printf("PRIME %dx%d = %d px, %d KB por cuadro\n",
           RK_PRIME_W, RK_PRIME_H, RK_PRIME_PX,
           RK_PRIME_PX * (int)sizeof(rk_color_t) / 1024);
    printf("MINI  %dx%d = %d px, %d KB por cuadro\n\n",
           RK_MINI_W, RK_MINI_H, RK_MINI_PX,
           RK_MINI_PX * (int)sizeof(rk_color_t) / 1024);

    printf("%-14s %11s %9s %11s %8s\n",
           "ANIMO", "PRIME ms", "fps", "MINI ms", "REL");
    for (m = 0; m < RK_MOOD_COUNT; m++) {
        double msm;
        forzar(0, (rk_mood_t)m);
        forzar(1, (rk_mood_t)m);

        for (i = 0; i < 100; i++) {          /* calentar cache */
            rk_prime_draw(&fbp, &g_kit, (uint32_t)i * 37u);
        }
        t0 = ahora_ms();
        for (i = 0; i < BENCH_N; i++) {
            rk_prime_draw(&fbp, &g_kit, (uint32_t)i * 37u);
        }
        ms = (ahora_ms() - t0) / BENCH_N;

        t0 = ahora_ms();
        for (i = 0; i < BENCH_N; i++) {
            rk_mini_draw(&fbm, &g_kit.nodes[1], (uint32_t)i * 37u);
        }
        msm = (ahora_ms() - t0) / BENCH_N;

        if (m == 0) {
            base = ms;
        }
        printf("%-14s %11.4f %9.0f %11.4f %7.2fx\n",
               rk_mood_name((rk_mood_t)m), ms, 1000.0 / ms, msm,
               base > 0.0 ? ms / base : 1.0);
    }

    printf("\n%-26s %11s\n", "ETAPA", "MS");

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_fb_clear(&fbp, 0x1234);
    }
    printf("%-26s %11.4f\n", "clear del Prime",
           (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_vgradient(&fbp, 0, 34, RK_PRIME_W, 176, 0x1234, 0x4321);
    }
    printf("%-26s %11.4f\n", "gradiente de la escena",
           (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_adulto_draw(&fbp, 120, 120, NULL, RK_MOOD_HAPPY, RK_SEV_OK,
                       RK_PRIME_ART, (uint32_t)i * 37u);
    }
    printf("%-26s %11.4f\n", "el adulto a 2x",
           (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_brote_draw(&fbm, 64, 56, NULL, RK_MOOD_HAPPY, RK_SEV_OK,
                      RK_ETAPA_JOVEN, RK_MINI_ART, (uint32_t)i * 37u);
    }
    printf("%-26s %11.4f\n", "el brote a 2x",
           (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_text(&fbp, 4, 4, "ABCDEFGHIJ 0123456789", 0xFFFF, 2);
    }
    printf("%-26s %11.4f\n", "21 glifos a escala 2",
           (ahora_ms() - t0) / BENCH_N);

    /* Lo que de verdad limita en la placa no es rasterizar sino empujar. */
    printf("\n  COTA DEL BUS (lo que manda de verdad)\n");
    bus("Prime  SPI 40 MHz", RK_PRIME_PX, 5.0);
    bus("Prime  SPI 80 MHz", RK_PRIME_PX, 10.0);
    bus("Mini   SPI 40 MHz", RK_MINI_PX, 5.0);
    printf("\n  El framebuffer del Prime son %d KB y el del Mini %d KB: los dos\n",
           RK_PRIME_PX * 2 / 1024, RK_MINI_PX * 2 / 1024);
    printf("  entran en la SRAM de un ESP32-C3 (400 KB) sin tocar PSRAM.\n\n");

    return 0;
}

/* ------------------------------------------------------- interactivo ----- */
#ifdef RK_WITH_SDL
#include <SDL2/SDL.h>

/* La ventana: el Prime a la izquierda, los Minis apilados a la derecha. */
#define GAP      12
#define WIN_W   ((RK_PRIME_W + GAP + RK_MINI_W + GAP * 2) * SIM_ESC)
#define WIN_H   ((RK_PRIME_H + GAP * 2) * SIM_ESC)

static int run_window(void)
{
    static rk_color_t lienzo[(RK_PRIME_W + GAP + RK_MINI_W + GAP * 2) *
                             (RK_PRIME_H + GAP * 2)];
    static rk_color_t pp[RK_PRIME_PX];
    static rk_color_t pm[RK_MINI_PX];
    int LW = RK_PRIME_W + GAP + RK_MINI_W + GAP * 2;
    int LH = RK_PRIME_H + GAP * 2;
    rk_fb_t big, fbp, fbm;
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    uint32_t     t0, t_ms = 0, last_tick = 0;
    int          speed = 1, running = 1, gacha = -1;
    uint32_t     gacha_t0 = 0;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");   /* vecino mas cercano */
    win = SDL_CreateWindow("ROOTKIT / Prime + Minis",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565,
                            SDL_TEXTUREACCESS_STREAMING, LW, LH);
    if (win == NULL || ren == NULL || tex == NULL) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }

    rk_fb_init(&big, lienzo, LW, LH);
    rk_fb_init(&fbp, pp, RK_PRIME_W, RK_PRIME_H);
    rk_fb_init(&fbm, pm, RK_MINI_W, RK_MINI_H);
    world_init();
    t0 = SDL_GetTicks();

    puts("teclas: 1-4 seleccionar nodo | W regar | M ciclar animo");
    puts("        R soltar animo | G ceremonia | +/- velocidad");
    puts("        O marcar caido | S captura | ESC salir");

    while (running) {
        SDL_Event ev;
        int i, my;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                int hit = rk_prime_hit(&g_kit,
                                       ev.button.x / SIM_ESC - GAP,
                                       ev.button.y / SIM_ESC - GAP);
                if (hit >= 0) {
                    g_kit.selected = hit;
                }
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE || k == SDLK_q) {
                    running = 0;
                } else if (k >= SDLK_1 && k <= SDLK_6) {
                    int nn = k - SDLK_1;
                    if (nn < g_kit.count) { g_kit.selected = nn; }
                } else if (k == SDLK_w) {
                    g_env[g_kit.selected].soil_x10 += 320;
                    if (g_env[g_kit.selected].soil_x10 > 1000) {
                        g_env[g_kit.selected].soil_x10 = 1000;
                    }
                } else if (k == SDLK_m) {
                    g_force = (rk_mood_t)((g_force + 1) % (RK_MOOD_COUNT + 1));
                } else if (k == SDLK_r) {
                    g_force = RK_MOOD_COUNT;
                } else if (k == SDLK_g) {
                    gacha = g_kit.selected;
                    gacha_t0 = t_ms;
                } else if (k == SDLK_o) {
                    g_kit.nodes[g_kit.selected].tel.age_s = RK_LINK_CAIDO_S + 1u;
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
        if (t_ms - last_tick > 250u) {     /* un ciclo de medicion simulado */
            world_tick(t_ms);
            last_tick = t_ms;
        }

        rk_fb_clear(&big, RK_RGB(8, 10, 9));

        /* ---- Prime ---- */
        if (gacha >= 0 && !rk_gacha_termino(t_ms - gacha_t0 + 800u)) {
            rk_gacha_draw(&fbp, g_kit.nodes[gacha].comp, t_ms - gacha_t0);
        } else {
            gacha = -1;
            rk_prime_draw(&fbp, &g_kit, t_ms);
        }
        pegar(&big, pp, RK_PRIME_W, RK_PRIME_H, GAP, GAP);
        rk_rect(&big, GAP - 1, GAP - 1, RK_PRIME_W + 2, RK_PRIME_H + 2,
                RK_RGB(60, 70, 62));

        /* ---- Minis ---- */
        my = GAP;
        for (i = 0; i < g_kit.count; i++) {
            if (g_kit.nodes[i].role != RK_ROLE_MINI) {
                continue;
            }
            rk_mini_draw(&fbm, &g_kit.nodes[i], t_ms);
            pegar(&big, pm, RK_MINI_W, RK_MINI_H,
                  GAP * 2 + RK_PRIME_W + GAP - GAP, my);
            rk_rect(&big, GAP * 2 + RK_PRIME_W - 1, my - 1,
                    RK_MINI_W + 2, RK_MINI_H + 2,
                    i == g_kit.selected ? RK_RGB(72, 214, 190)
                                        : RK_RGB(60, 70, 62));
            my += RK_MINI_H + GAP;
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
    fprintf(stderr, "compilado sin SDL: usa --sheet, --minis, --brotes o --shot\n");
    return 1;
}
#endif

int main(int argc, char **argv)
{
    if (argc >= 3 && strcmp(argv[1], "--sheet") == 0) {
        return hoja(argv[2], RK_MOOD_COUNT, 4, RK_PRIME_W, RK_PRIME_H,
                    cel_prime, et_mood, NULL);
    }
    if (argc >= 3 && strcmp(argv[1], "--minis") == 0) {
        return hoja(argv[2], RK_MOOD_COUNT, 4, RK_MINI_W, RK_MINI_H,
                    cel_mini, et_mood, NULL);
    }
    if (argc >= 3 && strcmp(argv[1], "--brotes") == 0) {
        return hoja(argv[2], rk_companion_count * RK_ETAPA_COUNT,
                    RK_ETAPA_COUNT, 96, 80, cel_brote, et_brote, NULL);
    }
    if (argc >= 3 && strcmp(argv[1], "--gacha") == 0) {
        return hoja(argv[2], RK_RAR_COUNT * 5, 5, RK_PRIME_W, RK_PRIME_H,
                    cel_gacha, et_gacha, bd_gacha);
    }
    if (argc >= 2 && strcmp(argv[1], "--bench") == 0) {
        return do_bench();
    }
    if (argc >= 3 && strcmp(argv[1], "--shot") == 0) {
        return do_shot(argv[2],
                       argc >= 4 ? argv[3] : NULL,
                       argc >= 5 ? (uint32_t)strtoul(argv[4], NULL, 10) : 1200u);
    }
    return run_window();
}
