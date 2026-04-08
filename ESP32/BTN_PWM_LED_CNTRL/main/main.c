#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" 
#include "driver/ledc.h"
#include "esp_err.h"
//pin ayarları
#define BUTTON_ART_GPIO 15 //artırma butonu
#define BUTTON_AZLT_GPIO 4 //azaltma butonu
#define LED_GPIO    5 // led pini


// ayarlamalar
#define LEDC_TIMER  LEDC_TIMER_0
#define LEDC_MODE   LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO  LED_GPIO
#define LEDC_CHANNEL   LEDC_CHANNEL_0
#define LEDC_DUTY_RES  LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY   (5000)
volatile int dc = 0;

static void ledc_init(void)
{
    // timer ayarları 
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  
        .clk_cfg          = LEDC_AUTO_CLK
    };
    // Ayarları donanıma uygula
    ledc_timer_config(&ledc_timer);

    // channel ayarları
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE, 
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0,                 // Başlangıçta LED sönük (Duty = 0)
        .hpoint         = 0
    };
    // Kanal ayarlarını donanıma uygula
    ledc_channel_config(&ledc_channel);
}






static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t pin = (uint32_t) arg;/*burada pointer casting işlemi yapıldı. Neden?
    kesme fonksiyonuna gelen arg tipi belli olmayan genel bir bellek addresidir
    her türlü veri tipini kabul etmek içn böyle tasarlanmıştır. Biz bu fonksiyona bir 
    pin numarası gönderdik. Bu pin numarasını tekrar kullanaiblmek için bu gelen bellekadresini bir sayı
    olarak oku ve pin değişkenine at*/

    if(pin == BUTTON_ART_GPIO){
        dc += 410;
        if(dc > 8191) dc = 8191;
    }
    else if (pin == BUTTON_AZLT_GPIO){
        dc -= 410;
        if(dc < 0) dc = 0;
    }
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, dc);//yeni duty değerini yazmak için kullanıyoruz bu fonksiyonu
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL); // bu yazılan yeni değeri pine göndermek için kullanıyoruz
    
}


void app_main(void) {
    
    ledc_init();

    // LED Ayarı 
    //gpio_reset_pin(LED_GPIO);
    //gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
   
    /*led ayarları kısmında ledimi normal gpıo olarak ayarladım fakat ledc içind  ledi pwm olarak ayarlamıştım  zaten.
    ikinci defa gpıo olarak ayarladığım için pwm olarak çıktı alamadım ledim hiç yanmadı.*/

    // Buton Ayarı
    gpio_reset_pin(BUTTON_ART_GPIO);
    gpio_reset_pin(BUTTON_AZLT_GPIO);

    gpio_set_direction(BUTTON_ART_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(BUTTON_AZLT_GPIO, GPIO_MODE_INPUT);

    gpio_set_pull_mode(BUTTON_ART_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(BUTTON_AZLT_GPIO, GPIO_PULLDOWN_ONLY);
    
    // Kesme Tipi: Yükselen Kenar (Posedge)
    gpio_set_intr_type(BUTTON_ART_GPIO, GPIO_INTR_POSEDGE);
    gpio_set_intr_type(BUTTON_AZLT_GPIO, GPIO_INTR_POSEDGE);

    // Kesme Servisini Kur ve Fonksiyonu Bağla
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_ART_GPIO, gpio_isr_handler, (void*) BUTTON_ART_GPIO);
    gpio_isr_handler_add(BUTTON_AZLT_GPIO, gpio_isr_handler, (void*) BUTTON_AZLT_GPIO);

    

    while (1) {
        printf("Güncel DC Değeri: %d\n", dc);
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}