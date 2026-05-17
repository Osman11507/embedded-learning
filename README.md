# Gömülü Sistemler Öğrenme Günlüğü (Embedded Learning Journey)

Bu depo, gömülü sistemler ve dijital tasarım alanındaki gelişimimi takip etmek ve yaptığım projeleri dökümante etmek amacıyla oluşturulmuştur. ESP32, STM32 ve FPGA platformlarında geliştirdiğim farklı seviyelerdeki uygulamaları içerir.

##  Donanım Envanteri
* **Mikrokontrolcüler:** ESP32, STM32 (NUCLEO-F401RE)
* **FPGA:** Tang Nano 4K


## DEPO YAPISI

Projeler platform bazlı olarak üç ana klasöre ayrılacaktır:

### 🔹 ESP32 Projeleri
* 'PWM_LED' : Projede genel olarak ESP-IDF kullanımı ile ilgili tecrübe edindim ve aynı zamanda bu projede Timer ve Channel arasındaki farkı öğrendim, Timer sinyalin kalbidir, Channel ise o sinyali pine taşıyan yoldur.

* 'BTN_PWM_LED_CNTRL': Projede timer ve channel kullanımını pekiştirdim aynı zamanda timer fonksiyonu kullanımını ve type casting işlemlerini öğrendim.

* 'DualCore_LED_Controller_RTOS' :  Bu projede ESP32'nin çift çekirdekli yapısı kullanılarak, bir çekirdekte h PWM (Fade) işlemleri, diğer çekirdekte ise standart I/O (Blink) işlemleri yönetilmiştir. Kesme (ISR) ve Task Notification mekanizmalarıyla senkronizasyon sağlanmıştır.

* 'Sentinel_Cloud' : ESP-IDF ve FreeRTOS kullanılarak geliştirilen bu IoT projesi, OpenWeatherMap API'sinden gerçek zamanlı veriler çekerek asenkron bir sistem mimarisi sunar. Proje kapsamında; çift çekirdekli (Dual-Core) yapıda görev dağılımı, FreeRTOS kuyrukları (Queues) ile görevler arası güvenli veri transferi, manuel HTTP akış yönetimi (open/fetch/read) ve cJSON ile dinamik bellek yönetimi gibi  disiplinleri uygulayarak sistem kararlılığını ve bellek güvenliğini optimize ettim. Mevcut mimariyi daha kararlı hale getirmek adına FreeRTOS kuyruk yönetimi, RTOS zamanlama dinamikleri, düşük seviyeli bellek yönetimi ve HTTP akış kontrolünün ince detayları üzerine derinlemesine çalışmalar ve öğrenimler yürütüyorum.

* 'Sentinal_UI' : Bu proje LVGL framework kullanılarak gelişştirilen, C dilinde nesne yönelimli programlama (OOP) mantığını simüle eden yüksek performanslı bir grafik arayüzü projesidir. Bu proje ile beraker OOP, HAL ve encapsulation prensiplerini örenmiş oldum. Projenin donanım kısmında birkaç teknik aksaklıklar olup çözümü içn hala uğraşmakltayım.


### 🔹 STM32 Projeleri
### HAL:STM32 de kütüphaneler ile yazdığım projeler.
* 'Led_blink' : HAL kütüphanesi ile yazdığım led yakıp södürme kodum.

* 'Led_PWM' : HAL kütüphanesi ile yazdığım led fade kodum.

* 'Led_PWM_BTN' : 'adet buton ile ledin parlaklığını artırıp azalttığım kodum.

* 'SERVO' : Servo motorunu 0->180 derece ve 180->0 derece arasında sürekli olarak hareket ettiren kodum.

### Bare Metal:STM32 de olabildiğince kütüphane kullanmadan register seviyesinde(Bare-metal) yazdığım projeler.
* 'Led_blink_BM' : Gpıo ve systick, RCC registerlarını kullanarak yazmış olduğum led blink projem. Bu proje kapsamında Systick kullanımı ile delay fonksiyonu yaprım. Countflag ve RCC yapılarının temellerini öğrenmiş oldum. Bu kısımlarda hala eksikliğini hissetiğim bazı şeyler var fakat çalışamalarım devam etmekte.

* 'Led_PWM_BM' : Bu projemde tamamen bare metal olarak PWM sinyali üretilmiştir. İlk aşamada Flash Latency ve PLL ayarları yapılarak işlemci saat hızı (HCLK) 84 MHz’e çekilmiştir. Ardından TIM2 zamanlayıcısı, Prescaler (PSC) ve Auto-Reload (ARR) değerleri üzerinden 1 kHz frekansında PWM üretecek şekilde yapılandırılmış ve PA1 pini Alternate Function moduyla bu sinyale atanmıştır. Ana döngü içerisinde CCR2 (Capture/Compare) kayıtçısı yazılımsal olarak güncellenerek duty cycle oranı değiştirilmiş ve bu sayede LED üzerinde pürüzsüz bir "breathing" efekti elde edilmiştir.

* 'Led_PWM_BTN_BM': Bu projede bare metal olarak işlemcinin hızı 84MHz ye çıkarılmış ve gerekli timer ayarları ile PWM sinyali ayarlanmıştır. Butonlara gerekli EXTI birimleri SYSCFG ile bağlanmış ve NVIC yapııs ile işlemciye aktarılmmıştır. EXTIIRQHandler fonksiyonları ile butonlar izlenip, işlemler atanmıştır.

* 'SERVO_BM' : Bu projede servo motorumun (SG90) datasheet inde bulunan değerlere göre bir TIM2 çıkışı ayarladım. İşlemcim gene 84MHz de çalışmakta. Bu sefer projemde basit delay işlemi kullanmak yerine Non-Blocking State Machine yapısını kullandım. Bu yapı iki kavramın birleşmesinden oluşuyor. 1. kavram Non-Blocking: Bir işi beklerken işlemciyi durdurmak (delay) yerine işlemcinin arka planda diğer işleri yapabilmesine imkan sağlamak. 2. kavram Time Base: Sistem zamanı ölçme işini ana iş yükünden alır ve donanımasl bir zamanlayıcıya devir eder. Bu zamanı biz istediğimiz gibi ayarlayabiliriz. Bu kavramların kesişim kümesinde Non-Blocking State Machine yer almakta. Bu proje özelinde; TIM2 zamanlayıcısını SG90 servonun standart 20ms'lik (50Hz) periyoduna göre yapılandırdım. Motorun 0° ile 180° arasındaki 300ms'lik fiziksel dönüş süresini bu periyoda bölerek 15 adımlık bir döngü oluşturdum. Yazılımsal adım sayacımız, donanımsal zamanlayıcının her 20ms'lik vuruşunda bir tetiklenerek servo konum karşılığını (CCR2) güncelliyor. Sayaç 15'e ulaştığında yönü otomatik olarak geriye, 0'a ulaştığında ise tekrar ileriye çevirerek işlemciyi tek bir milisaniye bile delay ile kilitlemeden otonom  bir servo döngüsü elde etmiş olduk. 
### 🔹 FPGA Projeleri




