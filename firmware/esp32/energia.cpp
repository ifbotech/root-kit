#include "energia.h"
#include "placa.h"
#include <esp_sleep.h>

RTC_DATA_ATTR static uint32_t g_reloj_base_s = 0;

static rk_despertar_causa_t g_causa = RK_DESPERTAR_FRIO;

void energia_iniciar(void)
{
    switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER:
        g_causa = RK_DESPERTAR_TIMER;
        break;
    case ESP_SLEEP_WAKEUP_EXT0:
    case ESP_SLEEP_WAKEUP_EXT1:
    case ESP_SLEEP_WAKEUP_GPIO:
        g_causa = RK_DESPERTAR_TOQUE;
        break;
    default:
        g_causa = RK_DESPERTAR_FRIO;
        g_reloj_base_s = 0;
        break;
    }
}

rk_despertar_causa_t energia_causa(void)
{
    return g_causa;
}

uint32_t energia_reloj_s(void)
{
    return g_reloj_base_s + (uint32_t)(millis() / 1000u);
}

void energia_reloj_minimo(uint32_t s)
{
    uint32_t ahora = energia_reloj_s();
    if (s > ahora) {
        g_reloj_base_s += s - ahora;
    }
}

void energia_dormir(uint32_t segundos)
{
    if (segundos < 1u) {
        segundos = 1u;
    }
    g_reloj_base_s = energia_reloj_s() + segundos;

    esp_sleep_enable_timer_wakeup((uint64_t)segundos * 1000000ull);
#if defined(RK_PLACA_C3)
    /* En el C3 sólo GPIO0-5 despiertan del deep sleep. */
    esp_deep_sleep_enable_gpio_wakeup(1ull << RK_PIN_TOQUE, ESP_GPIO_WAKEUP_GPIO_HIGH);
#else
    esp_sleep_enable_ext0_wakeup((gpio_num_t)RK_PIN_TOQUE, 1);
#endif
    Serial.flush();
    esp_deep_sleep_start();
}
