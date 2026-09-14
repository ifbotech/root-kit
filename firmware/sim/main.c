/* main.c — simulador de escritorio de la Terminal ROOTKIT.
 *
 *   ./build/rootkit_sim              interactivo, ventana 320x480
 *   ./build/rootkit_sim --sheet F    hoja de contacto con todos los ánimos
 *   ./build/rootkit_sim --shot F [M] [T] un cuadro suelto, forzando el ánimo
 *                                    M en el instante T de animación
 *
 * Los dos últimos modos no abren ventana: sirven para revisar el arte en
 * cualquier lado, y para dejar capturas en el repo.
 */
#define _POSIX_C_SOURCE 199309L   /* clock_gettime y struct timespec */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../gfx/fb.h"
#include "../gfx/font.h"
#include "../ui/screen.h"
#include "../art/tuga.h"
#include "../core/mood.h"

/* ------------------------------------------------------------- BMP ------- */
/* Escritor propio de 24 bits: así los modos de captura no dependen de SDL. */
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

/* --------------------------------------------------- mundo simulado ------ */
/* Cada maceta tiene un ambiente que deriva solo: la tierra se seca, el sol
 * sale y se pone. Sin esto el simulador muestra estados congelados y no se
 * puede juzgar si las transiciones se sienten bien. */
typedef struct {
    int soil_x10;     /* humedad en décimas, para que la deriva sea suave */
    int temp_dc;
    int rh;
    int dry_rate;     /* décimas de punto por ciclo                       */
    int lux_peak;
} env_t;

static rk_state_t  g_st;
static env_t       g_env[RK_MAX_PLANTS];
static rk_mood_t   g_force = RK_MOOD_COUNT;   /* COUNT = no forzar */

static void world_init(void)
{
    static const struct { const char *nom; const char *sp; int soil, t, rh, dry, lux; }
    SEED[] = {
        { "MONSTERA",  "monstera",    42, 235, 58,  4, 6200  },
        { "POTUS",     "pothos",      33, 228, 46,  6, 3100  },
        { "CACTUS",    "cactus",      12, 262, 24,  2, 26000 },
    };
    int i;

    memset(&g_st, 0, sizeof g_st);
    g_st.count        = 3;
    g_st.selected     = 0;
    g_st.wifi         = true;
    g_st.hub_batt_pct = 78;

    for (i = 0; i < g_st.count; i++) {
        rk_plant_t *p = &g_st.plants[i];
        snprintf(p->nombre, sizeof p->nombre, "%s", SEED[i].nom);
        p->sp = rk_species_find(SEED[i].sp);
        rk_mood_state_init(&p->mst);
        g_env[i].soil_x10 = SEED[i].soil * 10;
        g_env[i].temp_dc  = SEED[i].t;
        g_env[i].rh       = SEED[i].rh;
        g_env[i].dry_rate = SEED[i].dry;
        g_env[i].lux_peak = SEED[i].lux;
    }
}

/* Un "día" simulado dura 48 s de reloj real: suficiente para ver el ciclo
 * completo sin esperar, y lento como para que no maree. */
#define DAY_MS 48000u

static void world_tick(uint32_t t_ms)
{
    int i;
    int phase = (int)((t_ms % DAY_MS) * 256 / DAY_MS);
    int sun   = rk_sin8((uint8_t)phase);          /* -127..127 */

    for (i = 0; i < g_st.count; i++) {
        rk_plant_t *p = &g_st.plants[i];
        env_t      *e = &g_env[i];

        e->soil_x10 -= e->dry_rate;
        if (e->soil_x10 < 0) {
            e->soil_x10 = 0;
        }

        p->tel.valid    = true;
        p->tel.age_s    = 60;
        p->tel.soil_pct = (uint8_t)(e->soil_x10 / 10);
        p->tel.temp_dc  = (int16_t)(e->temp_dc + sun * 35 / 127);
        p->tel.rh_pct   = (uint8_t)(e->rh - sun * 8 / 127);
        p->tel.lux      = sun > 0 ? (uint32_t)(sun * e->lux_peak / 127) : 0u;
        p->tel.batt_mv  = 3900;

        p->verdict = rk_mood_eval(&p->mst, p->sp, &p->tel);
        if (g_force != RK_MOOD_COUNT) {
            p->verdict.mood     = g_force;
            p->verdict.severity = RK_SEV_WATCH;
            p->verdict.reason   = rk_mood_reason(g_force);
        }
    }
    g_st.hub_batt_pct = (uint8_t)(40 + rk_sin8((uint8_t)(t_ms / 400)) * 35 / 127);
}

/* ------------------------------------------------- modos de captura ------ */
static void render_one(rk_color_t *px, rk_mood_t mood, uint32_t t_ms)
{
    rk_fb_t fb;
    int i;

    rk_fb_init(&fb, px, RK_CANVAS_W, RK_CANVAS_H);
    world_init();
    for (i = 0; i < 40; i++) {           /* dejar que el mundo se asiente */
        world_tick(t_ms);
    }
    if (mood != RK_MOOD_COUNT) {
        rk_plant_t *p = &g_st.plants[0];
        p->verdict.mood     = mood;
        p->verdict.severity = (mood == RK_MOOD_THIRSTY) ? RK_SEV_URGENT
                                                        : RK_SEV_WATCH;
        p->verdict.reason   = rk_mood_reason(mood);
    }
    rk_screen_draw(&fb, &g_st, t_ms);
}

static int do_sheet(const char *path)
{
    enum { COLS = 4, ROWS = 3, PAD = 6, LAB = 10 };
    int cw = RK_CANVAS_W + PAD, ch = RK_CANVAS_H + PAD + LAB;
    int W = COLS * cw + PAD, H = ROWS * ch + PAD;
    rk_color_t *sheet = calloc((size_t)W * H, sizeof(rk_color_t));
    rk_color_t *cell  = calloc(RK_CANVAS_W * RK_CANVAS_H, sizeof(rk_color_t));
    rk_fb_t sfb;
    int m, x, y;

    if (sheet == NULL || cell == NULL) {
        return 1;
    }
    rk_fb_init(&sfb, sheet, W, H);
    rk_fb_clear(&sfb, RK_RGB(12, 14, 13));

    for (m = 0; m < RK_MOOD_COUNT; m++) {
        int col = m % COLS, row = m / COLS;
        int ox  = PAD + col * cw, oy = PAD + row * ch + LAB;

        render_one(cell, (rk_mood_t)m, 1200u + (uint32_t)m * 137u);
        for (y = 0; y < RK_CANVAS_H; y++) {
            for (x = 0; x < RK_CANVAS_W; x++) {
                rk_px(&sfb, ox + x, oy + y, cell[y * RK_CANVAS_W + x]);
            }
        }
        rk_rect(&sfb, ox - 1, oy - 1, RK_CANVAS_W + 2, RK_CANVAS_H + 2,
                RK_RGB(60, 70, 62));
        rk_text(&sfb, ox, oy - LAB + 1, rk_mood_name((rk_mood_t)m),
                RK_RGB(227, 165, 74), 1);
    }

    if (save_bmp(path, sheet, W, H) != 0) {
        fprintf(stderr, "no pude escribir %s\n", path);
        return 1;
    }
    printf("hoja de contacto %dx%d -> %s\n", W, H, path);
    free(sheet);
    free(cell);
    return 0;
}

static int do_shot(const char *path, const char *mood_name, uint32_t t_ms)
{
    rk_color_t *px = calloc(RK_CANVAS_W * RK_CANVAS_H, sizeof(rk_color_t));
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
    render_one(px, m, t_ms);
    if (save_bmp(path, px, RK_CANVAS_W, RK_CANVAS_H) != 0) {
        return 1;
    }
    printf("cuadro %dx%d -> %s\n", RK_CANVAS_W, RK_CANVAS_H, path);
    free(px);
    return 0;
}

/* ------------------------------------------------------------- bench ----- */
/* Mide el costo de renderizar. El número que importa no es el absoluto de
 * esta PC sino la proporción entre etapas: dice dónde conviene optimizar
 * antes de tener la Guition en la mano, y queda como línea de base para
 * comparar cuando llegue. Un ESP32-S3 a 240 MHz es groseramente un orden de
 * magnitud más lento que un x86 de escritorio. */
static double ahora_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

#define BENCH_N 2000

static int do_bench(void)
{
    static rk_color_t px[RK_CANVAS_W * RK_CANVAS_H];
    rk_fb_t fb;
    double  t0, ms, base = 0.0;
    int     i, m;

    rk_fb_init(&fb, px, RK_CANVAS_W, RK_CANVAS_H);
    world_init();
    for (i = 0; i < 40; i++) {
        world_tick(1200u);
    }

    printf("lienzo %dx%d = %d pixeles, %d bytes por cuadro\n",
           RK_CANVAS_W, RK_CANVAS_H, RK_CANVAS_W * RK_CANVAS_H,
           RK_CANVAS_W * RK_CANVAS_H * (int)sizeof(rk_color_t));
    printf("empujado a 320x480 por QSPI son %d bytes\n\n",
           320 * 480 * (int)sizeof(rk_color_t));
    printf("%-14s %11s %8s %8s\n", "ANIMO", "MS/CUADRO", "FPS", "REL");

    for (m = 0; m < RK_MOOD_COUNT; m++) {
        rk_plant_t *p = &g_st.plants[0];

        p->verdict.mood     = (rk_mood_t)m;
        p->verdict.severity = RK_SEV_WATCH;
        p->verdict.reason   = rk_mood_reason((rk_mood_t)m);

        for (i = 0; i < 100; i++) {          /* calentar cache */
            rk_screen_draw(&fb, &g_st, (uint32_t)i * 37u);
        }
        t0 = ahora_ms();
        for (i = 0; i < BENCH_N; i++) {
            rk_screen_draw(&fb, &g_st, (uint32_t)i * 37u);
        }
        ms = (ahora_ms() - t0) / BENCH_N;
        if (m == 0) {
            base = ms;
        }
        printf("%-14s %11.4f %8.0f %7.2fx\n",
               rk_mood_name((rk_mood_t)m), ms, 1000.0 / ms,
               base > 0.0 ? ms / base : 1.0);
    }

    printf("\n%-26s %11s\n", "ETAPA", "MS");

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_fb_clear(&fb, 0x1234);
    }
    printf("%-26s %11.4f\n", "clear del framebuffer",
           (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_vgradient(&fb, 0, 16, RK_CANVAS_W, 132, 0x1234, 0x4321);
    }
    printf("%-26s %11.4f\n", "gradiente de la escena",
           (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_tuga_draw(&fb, 80, 80, RK_MOOD_HAPPY, RK_SEV_OK, (uint32_t)i * 37u);
    }
    printf("%-26s %11.4f\n", "el simbionte completo",
           (ahora_ms() - t0) / BENCH_N);

    t0 = ahora_ms();
    for (i = 0; i < BENCH_N; i++) {
        rk_text(&fb, 4, 4, "ABCDEFGHIJ 0123456789", 0xFFFF, 1);
    }
    printf("%-26s %11.4f\n", "21 glifos de texto",
           (ahora_ms() - t0) / BENCH_N);

    /* Lo que de verdad limita en la placa no es rasterizar sino empujar.
     * A 80 MHz por cuatro lineas, el QSPI mueve unos 40 MB/s, asi que el
     * cuadro completo tarda ~7,7 ms sin importar cuanto tardo en dibujarse.
     * Conclusion: seguir optimizando el rasterizado rinde poco; el margen
     * esta en solapar el envio por DMA con el dibujo del cuadro siguiente. */
    printf("\n  cota del bus: %d bytes por QSPI a ~40 MB/s = %.1f ms/cuadro\n",
           320 * 480 * 2, (320.0 * 480.0 * 2.0) / (40.0 * 1024.0 * 1024.0) * 1000.0);
    printf("  el framebuffer logico son %d KB: entra en la SRAM interna del\n",
           RK_CANVAS_W * RK_CANVAS_H * (int)sizeof(rk_color_t) / 1024);
    printf("  S3 (512 KB) y no hace falta tocar PSRAM para rasterizar.\n\n");

    return 0;
}

/* ------------------------------------------------------- interactivo ----- */
#ifdef RK_WITH_SDL
#include <SDL2/SDL.h>

static int run_window(void)
{
    rk_color_t  px[RK_CANVAS_W * RK_CANVAS_H];
    rk_fb_t     fb;
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    uint32_t     t0, t_ms = 0, last_tick = 0;
    int          speed = 1, running = 1;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");   /* vecino más cercano */
    win = SDL_CreateWindow("ROOTKIT / Terminal",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           RK_CANVAS_W * RK_SCALE, RK_CANVAS_H * RK_SCALE,
                           SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565,
                            SDL_TEXTUREACCESS_STREAMING,
                            RK_CANVAS_W, RK_CANVAS_H);
    if (win == NULL || ren == NULL || tex == NULL) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }

    rk_fb_init(&fb, px, RK_CANVAS_W, RK_CANVAS_H);
    world_init();
    t0 = SDL_GetTicks();

    puts("teclas: 1-3 planta | W regar | M ciclar animo | R soltar animo");
    puts("        +/- velocidad | O offline | S captura | ESC salir");

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                int hit = rk_screen_hit(&g_st, ev.button.x / RK_SCALE,
                                        ev.button.y / RK_SCALE);
                if (hit >= 0) {
                    g_st.selected = hit;
                }
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE || k == SDLK_q) {
                    running = 0;
                } else if (k >= SDLK_1 && k <= SDLK_6) {
                    int n = k - SDLK_1;
                    if (n < g_st.count) { g_st.selected = n; }
                } else if (k == SDLK_w) {
                    g_env[g_st.selected].soil_x10 += 320;
                    if (g_env[g_st.selected].soil_x10 > 1000) {
                        g_env[g_st.selected].soil_x10 = 1000;
                    }
                } else if (k == SDLK_m) {
                    g_force = (rk_mood_t)((g_force + 1) % (RK_MOOD_COUNT + 1));
                } else if (k == SDLK_r) {
                    g_force = RK_MOOD_COUNT;
                } else if (k == SDLK_o) {
                    g_st.plants[g_st.selected].tel.age_s = 99999;
                } else if (k == SDLK_EQUALS || k == SDLK_PLUS) {
                    if (speed < 64) { speed *= 2; }
                } else if (k == SDLK_MINUS) {
                    if (speed > 1) { speed /= 2; }
                } else if (k == SDLK_s) {
                    save_bmp("captura.bmp", px, RK_CANVAS_W, RK_CANVAS_H);
                    puts("captura.bmp escrito");
                }
            }
        }

        t_ms = (SDL_GetTicks() - t0) * (uint32_t)speed;
        if (t_ms - last_tick > 250u) {     /* un ciclo de medición simulado */
            world_tick(t_ms);
            last_tick = t_ms;
        }

        rk_screen_draw(&fb, &g_st, t_ms);
        SDL_UpdateTexture(tex, NULL, px, RK_CANVAS_W * (int)sizeof(rk_color_t));
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
    fprintf(stderr, "compilado sin SDL: usa --sheet o --shot\n");
    return 1;
}
#endif

int main(int argc, char **argv)
{
    if (argc >= 3 && strcmp(argv[1], "--sheet") == 0) {
        return do_sheet(argv[2]);
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
