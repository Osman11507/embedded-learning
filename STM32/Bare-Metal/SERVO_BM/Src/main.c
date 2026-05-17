#include "stm32f401_regs.h" // register adreslerini sürekli tanımlamamak için bir header dosyası yazdım.

void CLK_CNFG_84MHz(void){
	FLASH_ACR &= ~0x7;
	FLASH_ACR |= (2<<0);

	RCC_CR |= (0<<24);
	while(RCC_CR &(1<<25));

	RCC_PLLCFGR &= ~((0x3F<<0) | (0x1FF<<6) | (0x03<<16));
	RCC_PLLCFGR |= (16<<0);//M değeri
	RCC_PLLCFGR |= (336<<6);//N değeri
	RCC_PLLCFGR |= (1<<16);//p değeri 4 ün binray karşılığı. ' bitlik alanımız olduğu için referansda hangi kombinasyonun neye böldiüi yazıyor. 01 4 e bölüyor
	RCC_PLLCFGR |= (0<<22);

	RCC_CR |= (1<<24);
	while(!(RCC_CR & (1<<25)));

	RCC_CR &= ~0x3;
	RCC_CR |= (2<<0);
	while ((RCC_CFGR & (0xC << 0)) != (0x8 << 0));// 0 ve 1. bitler sw bitleri bunları maskeliyorum C ile C= 1100. Ardından sws 3. ve 4. bitlerin 1000 yani 0x8 olana kadar bekliyorum
}

void GPIOA_INIT(void){
	RCC_AHB1ENR |= (1<<0);

	GPIOA_MODER &= ~(0x3<<2);
	GPIOA_MODER |= (0x2<<2);

	GPIOA_AFRL &= ~(0xF<<4);
	GPIOA_AFRL |= (0x1<<4);//RM0368 de afr1,2,3 ü nasıl yapacağımızı anlatıyor. Datasheet de ise istediğimiz pin için AFR1 in ne anlama geldiğini anlatıyor. ör PA1 bacağı için afrl1 = TIM2CH2, AFR7 ise USART



}


void TIM2_INIT(void){
	RCC_APB1ENR &= ~0xF;
	RCC_APB1ENR |= (1<<0);

	TIM2_PSC = 83;
	TIM2_ARR = 19999;

	TIM2_CCMR1 &= ~(0xFF<<8);//Alan temizleme
	TIM2_CCMR1 |= (6<<12);//PWM mode 1
	TIM2_CCMR1 |= (1<<11);//OC2PE

	TIM2_CCER |=(1<<4);// Sİnyali bacağa veriyorum

	TIM2_CR1 |= (1<<0);// saati başlatıyorum.

}

void Dummy_other_tasks(void){
	// buraya farklı tasklar yazabilirim ( şimdilik Non-Blocking state machine mantığını kavramam açısından örnek olarak açtım
}


int adim_sayaci = 0;
int yon_ileri = 1;



int main(void) {
    // 1. Sistem kurulumları
    CLK_CNFG_84MHz();
    GPIOA_INIT();
    TIM2_INIT();

    // Başlangıçta servo 0 derece
    TIM2_CCR2 = 1000;

    while(1) {
        // --- BLOKLAMASIZ ALAN ---
        // İşlemci her döngüde buradaki işleri sıfır gecikmeyle yapar
        Dummy_other_tasks();

        // --- ZAMAN TABANLI KONTROL (20ms RİTMİ) ---
        // TIM2_SR register'ının 0. biti UIF (Update Interrupt Flag) flagıdır.
        // Timer her 20ms'de bir sıfırlandığında bu bit donanımsal olarak 1 olur.
        if (TIM2_SR & (1 << 0)) {
            TIM2_SR &= ~(1 << 0); // Flagı temizliyoruz ki bir sonraki periyodu yapalayabilelim

            if (yon_ileri) {
                adim_sayaci++;
                // Her adımda CCR değerini rampa şeklinde artırıyoruz (1000'den 2000'e 15 adımda)
                TIM2_CCR2 = 1000 + (adim_sayaci * 1000 / 15);// toplam periyot 20ms. 180 derece için 300 ms gerekli. Yani 15 adım. Kat etmem gereken değer 1000. 1000/15=66,6666 artış miktarı
                //interpolation
                if (adim_sayaci >= 15) {
                    yon_ileri = 0; // 180 dereceye ulaştık, yönü geriye çevir
                }
            }
            else {
                adim_sayaci--;
                // Her adımda CCR değerini rampa şeklinde azaltıyoruz
                TIM2_CCR2 = 1000 + (adim_sayaci * 1000 / 15);

                if (adim_sayaci <= 0) {
                    yon_ileri = 1; // 0 dereceye ulaştık, yönü tekrar ileriye çevir
                }
            }
        }
    }
}



/*Burada basit bir şekilde delay ve for döngüleri kullanarak servo motorumu çalıştırmak yerine Non-Blocking state machine yazmaya çalıştım
 * Nedir bu Non-blocking state machine? işlemcinin ana döngüsünü (while(1)) 'delay' gibi bloke edici fonksiyonlarla kilitlemeden (non-blocking),
 * sistemin belirli durumlara (Yön İleri/Geri) ve donanımsal zamanlayıcı ritmine (timebase) göre otonom yönetilmesini sağlayan tasarım kalıbıdır.
 * Bu mimaride işlemci, servo motorun 300ms süren fiziksel dönüşünü 'adım_sayacı' (durum hafızası) ile her 20ms'de bir takip ederken; eşzamanlı olarak arka plandaki
 * dummy_other_task içerisinde bulunan hayati işlemleri yapabilir.
 * Kısaca timer bir davul çalar. Biz delay demek yerine davul 20. kez vurulduğunda şunu yap deriz */
