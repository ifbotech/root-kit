#include "pantalla.h"
#include "placa.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class PanelRootkit : public lgfx::LGFX_Device {
    lgfx::Panel_ST7735S _panel;
    lgfx::Bus_SPI   _bus;
    lgfx::Light_PWM _luz;

public:
    PanelRootkit(void)
    {
        {
            auto cfg = _bus.config();
            cfg.spi_host = RK_SPI_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = true;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = RK_PIN_SCK;
            cfg.pin_mosi = RK_PIN_MOSI;
            cfg.pin_miso = -1;
            cfg.pin_dc = RK_PIN_TFT_DC;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs = RK_PIN_TFT_CS;
            cfg.pin_rst = RK_PIN_TFT_RST;
            cfg.pin_busy = -1;
            cfg.panel_width = RK_TFT_W;
            cfg.panel_height = RK_TFT_H;
            /* El 1,44" usa la memoria de 132x162 del controlador. */
            cfg.memory_width = 132;
            cfg.memory_height = 162;
            cfg.offset_x = RK_TFT_OFS_X;
            cfg.offset_y = RK_TFT_OFS_Y;
            cfg.offset_rotation = 0;
            cfg.readable = false;
            cfg.invert = RK_TFT_INVERT != 0;
            cfg.rgb_order = RK_TFT_BGR != 0;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel.config(cfg);
        }
        {
            auto cfg = _luz.config();
            cfg.pin_bl = RK_PIN_TFT_BL;
            cfg.invert = false;
            cfg.freq = 22000;
            cfg.pwm_channel = 7;
            _luz.config(cfg);
            _panel.setLight(&_luz);
        }
        setPanel(&_panel);
    }
};

static PanelRootkit g_tft;
static rk_fb_t      g_fb;
static uint32_t    *g_hash_filas;
static int          g_oy;          /* dónde empieza el cuadrado en el panel  */
static bool         g_todo = true;
static rk_color_t   g_fondo_franjas;
static uint8_t      g_brillo;

bool pantalla_iniciar(uint8_t brillo_inicial)
{
    int lado = RK_TFT_W < RK_TFT_H ? RK_TFT_W : RK_TFT_H;
    rk_color_t *px;

    px = (rk_color_t *)heap_caps_malloc((size_t)lado * (size_t)lado * sizeof(rk_color_t),
                                        MALLOC_CAP_8BIT);
    g_hash_filas = (uint32_t *)calloc((size_t)lado, sizeof(uint32_t));
    if (px == NULL || g_hash_filas == NULL) {
        return false;
    }
    rk_fb_init(&g_fb, px, lado, lado);
    g_oy = (RK_TFT_H - lado) / 2;

    g_tft.init();
    g_tft.setRotation(0);
    g_tft.fillScreen(0x0000);
    g_brillo = 0;
    if (brillo_inicial > 0) {
        pantalla_brillo(brillo_inicial);
    } else {
        g_tft.sleep();
    }
    return true;
}

rk_fb_t *pantalla_fb(void)
{
    return &g_fb;
}

void pantalla_invalidar(void)
{
    g_todo = true;
}

static uint32_t hash_fila(const rk_color_t *p, int n)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < n; i++) {
        h = (h ^ p[i]) * 16777619u;
    }
    return h;
}

void pantalla_presentar(rk_color_t fondo)
{
    const int w = g_fb.w, h = g_fb.h;

    if (g_fb.px == NULL || g_brillo == 0) {
        return;
    }
    g_tft.startWrite();
    if (g_oy > 0 && (g_todo || fondo != g_fondo_franjas)) {
        g_tft.fillRect(0, 0, RK_TFT_W, g_oy, (lgfx::rgb565_t)fondo);
        g_tft.fillRect(0, g_oy + h, RK_TFT_W, RK_TFT_H - g_oy - h, (lgfx::rgb565_t)fondo);
        g_fondo_franjas = fondo;
    }
    /* Tramos contiguos de filas cambiadas, cada uno en una sola ráfaga. */
    int desde = -1;
    for (int y = 0; y <= h; y++) {
        bool cambio = false;
        if (y < h) {
            uint32_t hs = hash_fila(g_fb.px + y * w, w);
            cambio = g_todo || hs != g_hash_filas[y];
            g_hash_filas[y] = hs;
        }
        if (cambio && desde < 0) {
            desde = y;
        } else if (!cambio && desde >= 0) {
            g_tft.pushImage(0, g_oy + desde, w, y - desde,
                            (const lgfx::rgb565_t *)(g_fb.px + desde * w));
            desde = -1;
        }
    }
    g_tft.endWrite();
    g_todo = false;
}

void pantalla_brillo(uint8_t pct)
{
    if (pct > 100) {
        pct = 100;
    }
    if (pct == g_brillo) {
        return;
    }
    if (pct == 0) {
        g_tft.setBrightness(0);
        g_tft.sleep();
    } else {
        if (g_brillo == 0) {
            g_tft.wakeup();
            g_todo = true;
        }
        /* Curva perceptual: el 10 % de brillo tiene que verse como 10 %. */
        uint32_t v = (uint32_t)pct * pct * 255u / 10000u;
        g_tft.setBrightness((uint8_t)(v < 4u ? 4u : v));
    }
    g_brillo = pct;
}

uint8_t pantalla_brillo_actual(void)
{
    return g_brillo;
}
