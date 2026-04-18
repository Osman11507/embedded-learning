#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h" // JSON ayıklamak için gerekli kütüphane
#include "screts.h" // api, ssıd, password sakladığım header
static const char *TAG = "Sentinel_Cloud";

// --- KRİTİK AYARLAR (Burayı doldurmalısın) ---
#define WIFI_SSID      ssid
#define WIFI_PASS      password
#define API_KEY        apiKey
#define CITY           "Ankara,tr"

// --- Global Değişkenler ---
QueueHandle_t weather_queue; // queu tanımlama
bool is_internet_ready = false; //toggle

// 1. ADIM: Olay Yakalayıcı (Callback)
/*bu bir callback  fonksiyonudur. Bu fonksiyon parametreleri sistem tarafından gönderiliyor. Biz dışardan parametlereli manuel olarak vermiyoruz.
Yalnızca gelen pparametrelere  göre (EVENTS) işlem yapıyoruz. Call back sayesinde işlemci olay beklerken uyur, enerji tasarrufu yapılır. Peki bu parametreler nereden geliyor?
Bu parameteler bana ESP-IDF nin içindeki Wi-Fi sürücüsünden geliyor. Bu otomatik olarak gelme işlemine IoC(Inversion of Control) deniyor. Tam karşılığı "kontrolün tersine çevrilmesi"dir, yani kontrolü tamamen sürücüyebırakıyoruz*/
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        is_internet_ready = false;
        esp_wifi_connect();
        ESP_LOGW(TAG, "Modeme bağlanılamadı, tekrar deneniyor...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        is_internet_ready = true;
        ESP_LOGI(TAG, "İnternet bağlantısı başarılı!");
    }
}

// 2. ADIM: Veri Alma Taskı (Core 1)
void weather_fetch_task(void *pvParameters) {
    char *local_response_buffer = malloc(2048); // Heap bellekten 2KB yer ayır
    
    while (1) {
        if (is_internet_ready) {
            char url[256];
            snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/weather?q=%s&units=metric&appid=%s", CITY, API_KEY);//snprintf ile sabit URL ye şehir adı ve apı anahtarını ekleyerel (birleştirerek) tek bir metin haline getirir

            esp_http_client_config_t config = {
                .url = url,
                .method = HTTP_METHOD_GET,
                .timeout_ms = 5000,
                .buffer_size = 2048, // Dahili kütüphane tamponunu RAM kapasitemize göre set ettik
            };
            esp_http_client_handle_t client = esp_http_client_init(&config);

            // ADIM 1: Bağlantı Kanalını Aç
            // 'perform' yerine 'open' kullanarak süreci manuel kontrol moduna aldık (Low-level control)
            esp_err_t err = esp_http_client_open(client, 0); // hand-shake, kapıyı çalar ve sunucuya bağlanır, ama veri almaz
            
            if (err == ESP_OK) {
                // ADIM 2: Sunucu Başlıklarını (Headers) Oku
                
                int content_length = esp_http_client_fetch_headers(client);// zarfın üstünü okuma işlemi : verinin tipini ve boyutunu öğrenmek için
                
                // ADIM 3: HTTP Durum Kodunu Yakala
                
                int status_code = esp_http_client_get_status_code(client);// sunucunun ruh halini öğreniyoruz
                
                if (status_code == 200) {
                    // ADIM 4: Veriyi Fiilen Okuma
                    // Sunucunun gönderdiği 'content_length' kadar veriyi 'read' ile buffer'ımıza çekiyoruz.
                    int read_len = esp_http_client_read(client, local_response_buffer, 2047);// veriyi parça parça veya bütün halde ram e kopyalama (sunucuya kaç tane karakter geldi)
                    
                    if (read_len > 0) {// sunucuya karakter geldi ise kontrolü
                        // ADIM 5: Null Termination (En Kritik Adım!)
                        // C'de string sonu '\0' (0) ile biter. Okunan verinin sonuna bunu koyarak 
                        // cJSON_Parse fonksiyonunun nerede duracağını garanti altına alıyoruz.
                        local_response_buffer[read_len] = 0;  // gelen karakterin sonuna (örneğin 10 tane karakter geldiyse 10. kutuya ) dur işareti (sıfır, NULL) koyarız
                        
                        // Temizlenen veriyi işleme taskına (Core 1) gönder
                        xQueueSend(weather_queue, local_response_buffer, portMAX_DELAY);
                        /*Aklıma daha sağlam oturması için analoji:  Sen bir garsonsun, elinde 20 tabaklık büyük bir teğsi var
                        mutfaktan 5 tane tabak aldın. Bunları tepsinin başına dizdin. Tepsinin geri kalanı boş değil, eski müşterilerden kalan
                        artıklar var (peçete mendil vs). Sen bu tepsiyi masaya götürdün(cjson), müşteri hangisi çöp hangisi yemek anlamaz hepsini
                        yemeye çalışır ve rahatsız olur. Ama sen 5. tabaktan sonra "YEMEK BURADA SONALANDI" gibi bir uyarı bırakırsan(buffer[5]=0) müşteri tabeladan sonrasını yemez*/
                    }
                } else {
                    ESP_LOGE(TAG, "HTTP Sunucu Hatası! Durum Kodu: %d", status_code);
                }
            } else {
                ESP_LOGE(TAG, "Bağlantı Açılamadı: %s", esp_err_to_name(err));
            }
            // Her döngü sonunda belleği ve soketi temizle taşma yaşarsın!!
            esp_http_client_cleanup(client);
        }
        vTaskDelay(pdMS_TO_TICKS(10 *60 *1000)); // 10 dk yeterli
    }
}

// 3. ADIM: Veri İşleme Taskı (Core 1) - cJSON Analizi
void weather_process_task(void *pvParameters) {
    char received_buffer[2048];// gönderdiğimiz veri için depo

    while (1) {
        if (xQueueReceive(weather_queue, &received_buffer, portMAX_DELAY)) {//eğer veri geldiyse kontrolü
            // JSON ayıklanasının başladığı kısım
            cJSON *json = cJSON_Parse(received_buffer);// metini ağaç haline getirir
            if (json == NULL) {
                ESP_LOGE(TAG, "JSON Ayrıştırma Hatası!");
                continue;
            }

            // main -> temp yolunu takip et
            cJSON *main_obj = cJSON_GetObjectItem(json, "main");// ağaç haline getirdiği metinden bir dalı bulmak için kullanırız örn main.
            if (main_obj) {
                double temp = cJSON_GetObjectItem(main_obj, "temp")->valuedouble;// bulduğumuz daldan meyvesini " temp" topluyoruz. -> valuedouble; ifadesi alınan metin halindeki veriyi("15.5") matematiksel ondalıklı veriye(15.5) çevirir
                int humidity = cJSON_GetObjectItem(main_obj, "humidity")->valueint;
                
                ESP_LOGI(TAG, "***************************");
                ESP_LOGI(TAG, "Şehir: %s", CITY);
                ESP_LOGI(TAG, "Sıcaklık: %.2f °C", temp);
                ESP_LOGI(TAG, "Nem: %d%%", humidity);
                ESP_LOGI(TAG, "***************************");
            }
            cJSON_Delete(json); // Belleği temizle ÖNEMLİ!!. Bellek temizlenmezse taşma yaşarsın
        }
    }
}

// 4. ADIM: Donanım Başlatma
void wifi_init(void) {
    esp_netif_init();// TCP/Ip yığınını bellekte hazırlar. İnternet paketlerinin nasıl yönetileceğini sisteme öğretir.
    esp_event_loop_create_default();// işte IoC. Kontrolü sürücüye bıraktığımız kısım burası.
    esp_netif_create_default_wifi_sta();//ESP32 hem modem hem istasyon olarak kullanılabilmektedır. Burada biz "sen istasyonsun" diyoruz
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();//Wifi sürücüsünün RAM kullanımı, kanal ayarları ve güç yönetimi gibi donanım seviyesindeki ayarları yapar (WIFI_INIT_CONFİG_DEFAULT bir makrodur. ESP-IDF kütüphanesinden gelir ve güvenli varsayıla ndeğerleri cfg ye otomatik olarak doldurur)
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);// Adres bırakma işlemimiz. Sisteme " WIFI_EVENT veya IP_EVENT ailelerinden bir haber(event) gelirse git bizim event_handler fonksiyonumuzu(callback) çalıştır."diyoruz. Buraya haberler Ioc Kısmından geliyor, burası da callback fonksiyonumuzu tetikliyor.
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();/*NVS(Non-Volatile-Storge): ESP32'nin içindeki flash belleğin küçük bir bölümüdür. Elektrik kesilse bile buradaki bilgiler silinmez. 
    ESP-IDF wifi sürücüsü modeme bağlanırken kullandığı ssıd password ve fiziksel kalibrasyon verilerini(anten ayarları) buraya kayıt eder. Eğer NVS başlatılmazsa wifi sürücüsü bunları  yazacak bir yer bulamaz ve internete bağlanılamaz*/
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    weather_queue = xQueueCreate(1, 2048);

    wifi_init();

    // Taskları Çekirdek 1'e ata
    xTaskCreatePinnedToCore(weather_fetch_task, "fetch", 8192, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(weather_process_task, "process", 8192, NULL, 4, NULL, 1);
}
/*Bu projede app core olarak yalnıca bir çekirdek (CORE1) kullanmamıza rağmen neden queue kullandık? Queue bizim çekirdekler arasında bir hat oluşturmamıza yaramıyor muydu?
Bu projede queu'u senkronizasyon ve zamanlama kurabilmemiz için kullanıldık. Bizim elimizde iki tane app task var bunlar ; weather_fetch_task ve weather_process_task. fetch taskı internete gider
veriyi bekler, veri gelir gelmez işi biter ve kuyruğa gönderir. Procces taskı kuyruğun başında bekler, veri gelene kadar uykudadır. Veri geldiği an uyanır JSON'u alır parçalar ekranabasar ve tekrar kuyruk başında beklemeye başlar.
Eğer burada kuyruk kullanmasaydın yaşanacak senaryo şu olurdu: Aynı task önce veriyi alıyor ve veriyi alırken aynı zamanda işlemeye çalışıyor bu işleme sırasında başka bir veri gelebilir, bu veri geldiğinde task o kadar meşgul olurdu ki
sistem muhtemelen kilitlenirdi. Kısacası biz burada elimizde bulunan 1 çekirdek ile 2 taskı queue ile parçalayarak düzene soktuk ve  çekirdeği verimli kullandık.*/