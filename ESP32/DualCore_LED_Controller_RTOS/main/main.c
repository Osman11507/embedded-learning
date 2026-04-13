#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" 
#include "driver/ledc.h"
#include "esp_err.h"
// GPIO Ayarları
#define BTN1_GPIO 15
#define BTN0_GPIO 4 
#define LED1_GPIO 5 
#define LED2_GPIO 18
// Toggle değişkenleri
bool is_task_blink_active = true;
bool is_task_pwm_active = true;

// ledc değişkenleri
#define LEDC_TIMER              LEDC_TIMER_0            // Kullanacağımız Timer
#define LEDC_MODE               LEDC_LOW_SPEED_MODE     // Düşük hız modu 
                    
#define LEDC_CHANNEL            LEDC_CHANNEL_0          // Kullanacağımız PWM kanalı
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT       // 13-bit çözünürlük (0 - 8191 arası değer alır)
#define LEDC_FREQUENCY          (5000)                  // 5 kHz frekans (LED için gayet yeterli)
#define FADE_TIME_MS            (3000)                  // Yavaş yanma/sönme süresi (Milisaniye cinsinden, 3 saniye)


static void ledc_init(void); //fonksiyon bildirimi
// handle değişkenlerini global tanımladık çünkü kesme içindde kullanıyoruz
TaskHandle_t blnkHandle = NULL;
TaskHandle_t pwmHandle = NULL;



// blink taskı
void led_blnk(void *pvParam){
    ulTaskNotifyTake(pdTRUE,0);/*bildirim fonksiyonu ile taksı kontrol ediyoruz. eğer bildirim gelirse pdFalse oluyor ve işlemi tekrar bildirimgelene kadar askıya alıyor*/
    while(1){
        if(ulTaskNotifyTake(pdTRUE,0) >0 ){
            is_task_blink_active =! is_task_blink_active;
        }
        if(is_task_blink_active){
            gpio_set_level(LED2_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(500)); 
            gpio_set_level(LED2_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        else{
            gpio_set_level(LED2_GPIO, 0);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
 
    }
}




//pwm taskı
void pwm(void *pvParam){
    ledc_init();//ledc ayarları
    ledc_fade_func_install(0);//fade yapılandırması
    while(1){
        if(ulTaskNotifyTake(pdTRUE,0)>0){
            is_task_pwm_active =! is_task_pwm_active;// burada toggle yaparak aynı buton ile hem açma hem kapama işlemini gerçekleştirdik
        }

        if(is_task_pwm_active){
            printf("led yavasca yanıyor... \n");
            ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, 8191, FADE_TIME_MS);
            ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
            vTaskDelay(FADE_TIME_MS / portTICK_PERIOD_MS);
            printf("led yavas yavas sonuyor...\n");
            ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, 0, FADE_TIME_MS);
            ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
            vTaskDelay(FADE_TIME_MS / portTICK_PERIOD_MS);
        }
        else{
            ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
}








static void IRAM_ATTR gpio_isr_handler(void* arg){
    
    BaseType_t xHigherPriorityTaskWoken  = pdFALSE;/*FreeRTOS un ISR içinde kullandığı acil geçiş bayrağı
    başlangıçta her zaman pdFALSE alır. freertos çekirdeği kesme anında handi görevlerin uyandığını bilir
    . Fonksiyonun içine bu değişkenin adresini göndererk çekirdeğin bu değişkenin değerini içeriden
    değiştirmesine izin veririz. Eğer uyandırılan görev o anki görevden daha öncelikliyseçekirdek bu değişkeni pdTRUE yapar*/
    
    uint32_t pin = (uint32_t) arg;
    if(pin == BTN0_GPIO){
        
        
        vTaskNotifyGiveFromISR(blnkHandle, &xHigherPriorityTaskWoken);//bildirim gönderdiğimiz alan
        
    }
    else if (pin == BTN1_GPIO){
       
            vTaskNotifyGiveFromISR(pwmHandle, &xHigherPriorityTaskWoken);//bildirim gönderdiğimiz alan
    
            }
        


    if(xHigherPriorityTaskWoken == pdTRUE){
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);/*yol verme. Bir kesme bittiğinde işlemci normalde kesilmeden önceki
        taska geri döner. Eğer biz xHigherPriorityTaskWoken kontrolü yapıp yol vermezsek gecikme(uyandırdığımız yüksek öncelikli göre
        bir sonraki RTOS Tick gelene kadar bekeler) ve jitter (zaman sapması, görevimiz bazen 1ms sonra bazen 10 ms sonra uyanır) yaşanır*/
    }
}

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
        .gpio_num       = LED1_GPIO,
        .duty           = 0,                 // Başlangıçta LED sönük (Duty = 0)
        .hpoint         = 0
    };
    // Kanal ayarlarını donanıma uygula
    ledc_channel_config(&ledc_channel);
}



void app_main(void)
{   //gpio_set_level(LED2_GPIO, 1); bu iki satırı donanımında (bağlantı şeklimde ) hata olup olmadığını anlamak için yazdım
    //gpio_set_level(LED1_GPIO, 1);
    //buton ayarları
    gpio_reset_pin(BTN1_GPIO);
    gpio_reset_pin(BTN0_GPIO);

    gpio_set_direction(BTN0_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(BTN1_GPIO, GPIO_MODE_INPUT);

    gpio_set_pull_mode(BTN0_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_pull_mode(BTN1_GPIO, GPIO_PULLDOWN_ONLY);

    //led ayarı
    gpio_reset_pin(LED2_GPIO);
    gpio_set_direction(LED2_GPIO, GPIO_MODE_OUTPUT);

    // kesme tipi
    gpio_set_intr_type(BTN0_GPIO, GPIO_INTR_POSEDGE);
    gpio_set_intr_type(BTN1_GPIO, GPIO_INTR_POSEDGE);

    //kesme servisi kurma ve fonksiyona bağlama
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN0_GPIO, gpio_isr_handler, (void*) BTN0_GPIO);
    gpio_isr_handler_add(BTN1_GPIO,gpio_isr_handler, (void*) BTN1_GPIO);


    // task oluşturma
    xTaskCreatePinnedToCore(led_blnk, "led_blnk1", 4096, NULL, 1, &blnkHandle, 0);
    
    
    xTaskCreatePinnedToCore(pwm, "pwm1", 4096, NULL, 1, &pwmHandle, 1);
}