#include "esp_lcd_ili9341.h"
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "Sentinel_UI";

// --- DONANIM TANIMLAMALARI ---
#define PIN_NUM_MOSI   23
#define PIN_NUM_CLK    18
#define PIN_NUM_CS     27
#define PIN_NUM_DC     21
#define PIN_NUM_RST    33
#define LCD_H_RES      240
#define LCD_V_RES      320

// --- BINARY RESİM ERİŞİMİ ---
extern const uint8_t bg_start[] asm("_binary_bg_bin_start");
extern const uint8_t bg_end[]   asm("_binary_bg_bin_end");

const lv_img_dsc_t my_bg_dsc = {
  .header.cf = LV_IMG_CF_TRUE_COLOR,
  .header.always_zero = 0,
  .header.w = 240,
  .header.h = 320,
  .data_size = 240 * 320 * 2,
  .data = bg_start,
};

// --- LVGL CALLBACKS ---
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

static void lvgl_tick_inc(void *arg) {
    lv_tick_inc(10);
}

void app_main(void) {
    ESP_LOGI(TAG, "Donanım başlatılıyor...");

    // 1. SPI BUS KURULUMU
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK, // saat pini, ritimbelirliyor
        .mosi_io_num = PIN_NUM_MOSI,//veri pini, pixellerin aktığı yol
        .miso_io_num = -1,// ekrandan veri almadığımız için giriş yok -1
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * 2,// DMA kapasitesi
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));//bu fonksiyon ayarladığımız config yapısını alıp donanıma işi başlat emri verir
/*SPI BUS nedir? BUS, birdenfazlabileşen arasında veri transferi yapmak için kullanılan paylaşımlı bir
iletişim yoludur. SPI cihazlar arasında kısamesafeli, yüksek hızlı ve tam çift yönlü iletişim sağlayansenkron seri veri protokolü
Fonksiyon parametreleri: LCD_HOST esp içerisindeki 3 tanespı motorundan hangisini kullanacağız
&buscfg: yaptığımız ayarların adresi. SPI_DMA_CH_AUTO: DMA kanalını otomatik seçer
DMA nedir? DMA bir donanım birimidir. İşlemci önemli ve ana işini yaparken diğer işlerini bağımsız bir donanım birimine devreder
DMA kullanmak için  1 Source adresi : verinin RAM'de nerede tutulduğu. 2 Destination adresi: verinin nereye gideceği. 3 Transfer count: kaç byte taşınacak
bilgileri gereklidir */

    // 2. LCD PANEL IO KURULUMU

/*spi_mode ik cihazın aynı dili kullanıp kullanamdığını belirleyen temel kural. 
Sİnyal boştayken saat pini ne yapsın. Veri sinyalın hangi kenarında okunsun gibi işlere bakar CPOL, CPHA*/
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,//komut mu veri mi olduğunu belirleyen pin (data/Command)
        .cs_gpio_num = PIN_NUM_CS,//ekranıseçen aktif eden pin
        .pclk_hz = 1 * 1000 * 1000, // HIZ
        .lcd_cmd_bits = 8,//komut uzunluğu
        .lcd_param_bits = 8,//parametre uzunluu
        .spi_mode = 3,// SPI zamanlama modu
        .trans_queue_depth = 10,// işlem sırası derinliği
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &io_handle));// bu fonksiyon hazırladığımız ayarları alıp SPI BUS üzerinden bir sanal IO cihazı oluşturur
    /*bu fonksiyon type casting ve katmanlı mimari ile ilgili aslında. esp_lcd_spi_bus_handle_t aslında bir veri tipidir, değişken değil
    LCD sürücülerini SPI donanımından soyutlamak için bu ismi (typedef) tanımlamışlar bu tip arka planda  sadece bir enum veya bir pointer tutar.
    Ancak kütüphane "benim fonksiyoun sadece SPI hattını temsil eden bir kimlik bekliyor" demek için bu özel ismi kullanır. SPI3_HOST esp32 nin donanımsal spı birimini numarasıdır
    Bu fonksiyon ise ilk parametre olarak düz bir sayı değil(birim numarası gibi) ..._handle__t tipinde tutmak (handle) bekler
    Biz de fonskiyonda diyoruz ki "Sen bu SPI3_HOST sayısını al ve sanki o bir esp_lcd_sp_bus_handle_t miş gibi davran (type casting) diyoruz
    Peki neden doğrudan SPI3_HOST yazmıyoruz? HAL yapısı. Eğer kütüphane doğrudan SPI3_HOST değeri alsaydı başka zaman esp2 yerine farklı bir spı birimi olan başka bir işlemci ile çalışsaydım tüm lcd kütüphanesi
    baştan yazılmak durumunda olurdu. Biz bu handle a spı3_host u al ve bunu kütüphanenin anlayacağı bir formata çevir diyoruz (kütüphane ile giriş kartı oluşturma )" */
    
    
    // 3. ILI9341 SÜRÜCÜSÜNÜ YÜKLEME

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,//reset pini
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,//renk sıralaması
        .bits_per_pixel = 16,// renk derinliği
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));//bu fonksiyon yukarda oluştrduğumuz ıo handle ile panel confgi bilgilerini birleştirir
    
    // Paneli Fiziksel Olarak Aktif Et
    esp_lcd_panel_reset(panel_handle);
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_lcd_panel_init(panel_handle);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // YÖN VE AYNA AYARLARI 
    esp_lcd_panel_mirror(panel_handle, true, false);
    esp_lcd_panel_swap_xy(panel_handle, false);
    esp_lcd_panel_set_gap(panel_handle, 0, 0);


    // *** EKRANI SİYAH YAPMA (TEMİZLEME) KODU ***

    // RAM'de küçük bir siyah satır oluşturup tüm ekrana basıyoruz.
    uint16_t *black_line = heap_caps_malloc(LCD_H_RES * sizeof(uint16_t), MALLOC_CAP_DMA);
    for(int i=0; i < LCD_H_RES; i++) black_line[i] = 0x0000; // Siyah renk
    for(int y=0; y < LCD_V_RES; y++) {
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, LCD_H_RES, y + 1, black_line);
    }
    free(black_line);
    // ====================================================================

    esp_lcd_panel_disp_on_off(panel_handle, true);

    // 4. LVGL KURULUMU
    lv_init();//lvgl nin iç mekanızmasını (hafiza,grafik motoru vs) uyandırır
    
    lv_color_t *buf1 = heap_caps_malloc(LCD_H_RES * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA); // Piksellerin ekrana gitmeden önce RAM de boyandığı kısım,DMA biriminin erişebileceği özel bir rem bölgesi ayırdık
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, LCD_H_RES * 40);//boyama yaparken bu buf1 kısmını kullanması için lvgl ye talimat veriyoruz

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;// pikselleri basacak olan call back fonksiyonumuz
    disp_drv.draw_buf = &draw_buf;// az önce ayırığımız çalışma alanı
    disp_drv.user_data = panel_handle;  // donanım anahtarımız. 3. kısımdaki lvgl nin cebine koyuyoruz
    lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t tick_timer_args = { .callback = &lvgl_tick_inc, .name = "lvgl_tick" };
    esp_timer_handle_t tick_timer = NULL;
    esp_timer_create(&tick_timer_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, 10 * 1000); // her 10 saniyede bir tick le (donanımsal timer)

    // 5. UI TASARIMI
/*lvgl de ekrandaki her şey bir objedir. Bir buton, bir yazı veya arka plan. LVGL bunları bir ebeveyn çocuk ilişkisi içinde bellekte tutar*/
    lv_obj_t * img_bg = lv_img_create(lv_scr_act());//bellekte bir resim nesnesi için yer açıyor
    lv_img_set_src(img_bg, &my_bg_dsc);//resim nesnesinin içerisine asıl görselleri yani pikselleri enjekte ediyor
    lv_obj_set_size(img_bg, LCD_H_RES, LCD_V_RES);//ekran boyutunu ayarlaıyor
    lv_obj_align(img_bg, LV_ALIGN_CENTER, 0, 0);//ekranın tam ortasını hedefliyor

    lv_obj_t * label = lv_label_create(lv_scr_act());//bir metin nesnesi oluşturuyor ve içeriğini belirliyor
    lv_label_set_text(label, "Merhaba Osman!");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0); 
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 40); //yazı yerini belirliyor

    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
/*--------C dilinde nesne yönmelimi programlama (oop) taklidi---------------
NEDEN YAPIYORUZ? 
  C dili doğuştan "Nesne Yönelimli" (Java/C++ gibi) değildir. Ancak UI yapıları doğası gereği 
  hiyerarşiktir (Ebeveyn-Çocuk ilişkisi). Sentinel_UI projesinde LVGL kullanarak C'ye "OOP Maskesi" 
  taktırıyoruz. Bu sayede kod modüler, sürdürülebilir ve bellek dostu hale geliyor.
 NASIL YAPIYORUZ? (3 ANA TEKNİK):
  1. ENCAPSULATION (Kapsülleme): 'lv_obj_t' isimli devasa bir 'struct' (yapı) oluşturulur. 
  Bir objenin koordinatları, rengi ve tipi bu yapıda paketlenir. Biz sadece bu yapının 
  bellekteki ADRESİNİ (Pointer/Handle) elimizde tutarız.
   2. INHERITANCE (Miras Alma Taklidi): 'lv_img_create' veya 'lv_label_create' dediğimizde, 
  sistem RAM'de önce temel bir 'lv_obj_t' alanı açar, hemen peşine o nesneye özel (resim veya 
  metin verileri) ek alanı ekler. Yani her widget, temel objenin özelliklerini miras alır.
  3. POLYMORPHISM (Çok Biçimlilik): 'lv_obj_set_x()' fonksiyonuna ister bir butonu, ister 
  bir resmi gönderelim; fonksiyon hepsini 'lv_obj_t *' (temel adres) olarak kabul eder. 
  Çünkü hepsinin bellek başlangıcında aynı "kimlik kartı" (base struct) vardır.
 BELLEK YÖNETİMİ (THE POWER OF HANDLES):
  'lv_obj_t * label' dediğimizde, 'lvgl.h' içerisindeki şablonu kullanarak RAM'deki 
  o spesifik adresi "işaretlemiş" oluruz. Bu sayede tüm veriyi kopyalamak yerine sadece 
  4 byte'lık adres bilgisiyle devasa grafik objelerini yönetebiliriz. Ebeveyn bir obje 
  silindiğinde, bu hiyerarşi sayesinde ona bağlı tüm "çocuk" adresler de otomatik temizlenir.

*/