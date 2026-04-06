// 6.04.2026

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

// --- AYARLAMALAR ---
#define LEDC_TIMER              LEDC_TIMER_0            // Kullanacağımız Timer
#define LEDC_MODE               LEDC_LOW_SPEED_MODE     // Düşük hız modu 
#define LEDC_OUTPUT_IO          (16)                    // LED'i bağlayacağımız GPIO Pini
#define LEDC_CHANNEL            LEDC_CHANNEL_0          // Kullanacağımız PWM kanalı
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT       // 13-bit çözünürlük (0 - 8191 arası değer alır)
#define LEDC_FREQUENCY          (5000)                  // 5 kHz frekans (LED için gayet yeterli)
#define FADE_TIME_MS            (3000)                  // Yavaş yanma/sönme süresi (Milisaniye cinsinden, 3 saniye)

static void ledc_init(void)
{
    // 1. ADIM: Timer (Zamanlayıcı) Konfigürasyonu
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  
        .clk_cfg          = LEDC_AUTO_CLK
    };
    // Ayarları donanıma uygula
    ledc_timer_config(&ledc_timer);

    // 2. ADIM: Channel (Kanal) Konfigürasyonu
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE, // Kesmeleri kapalı tutuyoruz
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0,                 // Başlangıçta LED sönük (Duty = 0)
        .hpoint         = 0
    };
    // Kanal ayarlarını donanıma uygula
    ledc_channel_config(&ledc_channel);
}

void app_main(void)
{
    // Öncelikle LEDC ayarlarımızı başlatan fonksiyonu çağırıyoruz
    ledc_init();

    // 3. ADIM: Fade (Yumuşak geçiş) servisini kur. 
    // Parametre 0, varsayılan ayarlar için kullanılır. Bu olmadan fading komutları çalışmaz!
    ledc_fade_func_install(0);

    printf("LED PWM Kontrolu basliyor...\n");

    while (1) {
        printf("LED yavasca yaniyor...\n");
        // Donanıma diyoruz ki: "Hedef parlaklık 8191 (maksimum). Bu hedefe FADE_TIME_MS sürede ulaş."
        ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, 8191, FADE_TIME_MS);
        
        // Komutu başlat. "LEDC_FADE_NO_WAIT" parametresi sayesinde işlemci bu satırda beklemez, arka planda donanım LED'i yakar.
        ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
        
        // Donanım LED'i yavaşça yakarken bizim de o süre kadar beklememiz lazım, yoksa hemen aşağıdaki sönme koduna geçer.
        vTaskDelay(FADE_TIME_MS / portTICK_PERIOD_MS);

        printf("LED yavasca sonuyor...\n");
        // Hedef parlaklık 0 (tamamen sönük). 
        ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, 0, FADE_TIME_MS);
        ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
        vTaskDelay(FADE_TIME_MS / portTICK_PERIOD_MS);
    }
}