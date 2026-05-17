typedef unsigned int uint32_t;

// --- FLASH ---
#define FLASH_BASE      0x40023C00
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE + 0x00))

// --- RCC ---
#define RCC_BASE        0x40023800
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_PLLCFGR     (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44))//14. bit SYSCFGEN

// --- GPIO ---
#define GPIOA_BASE      0x40020000
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_OSPEEDER  (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOA_AFRL      (*(volatile uint32_t *)(GPIOA_BASE + 0x20))

#define GPIOC_BASE      0x40020800
#define GPIOC_MODER     (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_PUPDR     (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))
#define GPIOC_IDR       (*(volatile uint32_t *)(GPIOC_BASE + 0x10))

// --- TIM2 ---
#define TIM2_BASE       0x40000000
#define TIM2_CR1        (*(volatile uint32_t *)(TIM2_BASE + 0x00))
#define TIM2_CCMR1      (*(volatile uint32_t *)(TIM2_BASE + 0x18))
#define TIM2_CCER       (*(volatile uint32_t *)(TIM2_BASE + 0x20))
#define TIM2_PSC        (*(volatile uint32_t *)(TIM2_BASE + 0x28))
#define TIM2_ARR        (*(volatile uint32_t *)(TIM2_BASE + 0x2C))
#define TIM2_CCR2       (*(volatile uint32_t *)(TIM2_BASE + 0x38))

// --- SYSCFG ---
#define SYSCFG_BASE        0x40013800
#define SYSCFG_EXTICR1    (*(volatile uint32_t *)(SYSCFG_BASE + 0x08))



// --- NVIC ---
#define NVIC_BASE          0xE000E100
#define NVIC_ISER0         (*(volatile uint32_t *)(NVIC_BASE + 0x00))
#define NVIC_IPR           (*(volatile uint32_t *)(NVIC_BASE + 0x400))



// --- EXTI ---
#define EXTI_BASE          0x40013C00
#define EXTI_IMR           (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_PR            (*(volatile uint32_t *)(EXTI_BASE + 0x14))
#define EXTI_FTSR          (*(volatile uint32_t *)(EXTI_BASE + 0x0C))
#define EXTI_RTSR          (*(volatile uint32_t *)(EXTI_BASE + 0x08))

// --- STK ---
#define STK_CTRL          (*(volatile uint32_t *)0xE000E010)
#define STK_LOAD          (*(volatile uint32_t *)0xE000E014)
#define STK_VAL           (*(volatile uint32_t *)0xE000E018)








void Clock_Config_84MHz(void){
	FLASH_ACR &= ~0x7;//temizleme
	FLASH_ACR |= (2<<0);// flash gecikmesini 2ws olarak ayarlıyoruz


	RCC_CR &= ~(1 << 24);// PLLON değerini 0 yaparak kapatıyorum
	while(RCC_CR & (1<<25));// PLLRDY bitinin 0 olmasını bekleyerek emin oluyoruz

	RCC_PLLCFGR &= ~((0x3F<<0) | (0x1FF << 6) | (0x03 << 16));// M, N ve P alanlarını temizleme
	RCC_PLLCFGR |= (16<<0);//M değeri
	RCC_PLLCFGR |= (336 << 6);//N değeri
	RCC_PLLCFGR |= (1<<16);// P değeri p=4 binary karşılık
	RCC_PLLCFGR |= (0 << 22);// Kaynak olarak (HSI) 16 MHz kullanamsını söyledik bu biti 0 yaparak
/*PLL formülizasyonu = Fpll_out = (Fhsı/M) X (N/P)  16/16 = 1, 336/4 = 84, 1x84 = 84. */

	RCC_CR |= (1 << 24);//PLL yi tekrar açıyorum
	while(!(RCC_CR & (1<<25)));// açılana kadarbekliyor ve emin oluyorum (polling)
	RCC_CFGR &= ~0x3;// Alan tmeizleme
	RCC_CFGR |= (2<<0);//Yukarıda ayarladığım pll yi işlemcinin ana kaynağı olarak atıyorum
	while ((RCC_CFGR & (0xC << 0)) != (0x8 << 0)); // SWS bitleri  ile PLL kullanılana kadar bekliyorum.
}





void GPIO_PWM_INIT(void){
	RCC_AHB1ENR |= (1<<0);// GPIOA saatini aktif ediyorum

	GPIOA_MODER &= ~(0x3<<2);// Mod alanını temizliyorum
	GPIOA_MODER |= (0x2<<2);// mod olarak Altarnate Function ( özel görev Uart timer vs gibi) seçiyorum

	GPIOA_AFRL &= ~(0xF<<4);//görev alanını temizliyorum
	GPIOA_AFRL |= (0x1<<4);// Özel görev olarak TIM2 seçiyorum Yukardakinden farkı. Yukarıda sadece özel görev yapacaksın sen dedik. Burada ise senin görevin  artık TIM diyoruz.


}






void GPIO_BTN_INIT(void){
	RCC_AHB1ENR |= (1<<2);//GPIOC saatini aktif etme

	GPIOC_MODER &= ~(0xF<<2);// ınput olarak ayarlama

	GPIOC_PUPDR &= ~(0xF<<2);//pull up direnç ayarı
	GPIOC_PUPDR |= (0x5<<2);


}




void TIM2_PWM_INIT(void){
	RCC_APB1ENR &= ~0xF;//alan temizleme
	RCC_APB1ENR |=(1<<0);//TIM2 sayacını başlatdık
	TIM2_PSC = 83;//
	TIM2_ARR = 999;

	TIM2_CCMR1 &= ~(0xFF<<8);//Alan temizleme
	TIM2_CCMR1 |= (6<<12);//PWM mode 1
	TIM2_CCMR1 |= (1<<11);//OC2PE

	TIM2_CCER |=(1<<4);// Sİnyali bacağa veriyorum

	TIM2_CR1 |= (1<<0);// saati başlatıyorum.


}



void SYSCFG_INIT(void){
/*STM32 mimarisinde fiziksel pinler (PA, PB, PC...) ile işlemcinin kesme hatları
 * (EXTI) arasında doğrudan bir bağ yoktur. SYSCFG, bu pinlerden hangisinin kesme sinyalini EXTI birimine ileteceğini seçer*/
	RCC_APB2ENR |= (1<<14);

	SYSCFG_EXTICR1 &= ~(0xF << 4);  // EXTI1 parselini sıfırla
	SYSCFG_EXTICR1 |= (0x2 << 4);   // 0x2 = Port C demektir


	SYSCFG_EXTICR1 &= ~(0xF << 8);  // EXTI2 parselini sıfırla
	SYSCFG_EXTICR1 |= (0x2 << 8);   // 0x2 = Port C demektir

}



void EXTI_INIT(void){
/*SYSCFG bağlantıyı yaptı, ancak o hattın nasıl davranacağını,
 * hangi sinyalde tepki vereceğini henüz bilmiyoruz.
 * EXTI birimi, gelen sinyalin karakterini (düşen kenar mı, yükselen mi?)
 * inceler ve ona göre işlemciye haber verip vermeyeceğine karar verir.*/
	EXTI_IMR |= (1 << 1);//maske kaldırma pin 1
	EXTI_FTSR |= (1 << 1);// düşen kenar olarak ayarladık kesmeyi çünkü butonumuz pul up

	EXTI_IMR |= (1 << 2);
	EXTI_FTSR |= (1 << 2);
}



void NVIC_INIT(void){
/*Bu fonksiyon ile işlemciye kesmeyi ibldiriyoruz. Vector table a bolca bakmam gerek*/

    // EXTI1 (Pin 1) kesmesini işlemciye bildir. (7. biti 1 yap)
    NVIC_ISER0 |= (1 << 7);

    // EXTI2 (Pin 2) kesmesini işlemciye bildir. (8. biti 1 yap)
    NVIC_ISER0 |= (1 << 8);


}


void Update_PWM(int direction){
	static int pwm_value = 100;
	const int step = 50;

	pwm_value += (direction * step);

	if(pwm_value < 0){
		pwm_value = 0;
	}

	if(pwm_value > 999){
		pwm_value = 999;
	}

	TIM2_CCR2 = (uint32_t)pwm_value;
}


void Delay_ms(uint32_t ms){
	STK_LOAD = 84000 - 1;// 1 sn de 84000000 ise 1 ms için 84000-1 ( sayaç load değerinden geriye doğru sayar )
	STK_VAL = 0;//
	STK_CTRL = 5;// sayacı başlat ve işlemci ile senkron hale getir

	for (uint32_t i = 0; i < ms; i++) {
		while (!(STK_CTRL & (1 << 16)));//polling kontrolü ile 16. bit de bulunan COUNTFLAG ın 1 olmasını bekliyoruz ( 1 kere döngü tamamlandı ). ASıl geçikme işlemini bu kontrol işelmi yapıyor
	    }
	STK_CTRL = 0;//yük boşaltma

}






// PC1 Butonu (Artırma) tetiklendiğinde buraya girer
void EXTI1_IRQHandler(void) {
    // EXTI_PR register'ındaki 1. bite 1 yazarak "Kesme işlendi" mesajı veriyoruz.
    EXTI_PR |= (1 << 1);

    Delay_ms(20);// Switch bouncing yaşadığım için delay ekledim

    // Merkezi fonksiyonu "Artır" (1) komutuyla çağır.
    Update_PWM(1);
}

// PC2 Butonu (Azaltma) tetiklendiğinde buraya girer
void EXTI2_IRQHandler(void) {
    // EXTI_PR register'ındaki 2. bite 1 yazarak bayrağı temizliyoruz.
    EXTI_PR |= (1 << 2);

    // Merkezi fonksiyonu "Azalt" (-1) komutuyla çağır.
    Update_PWM(-1);
}


int main(void){
	Clock_Config_84MHz();
	GPIO_PWM_INIT();
	GPIO_BTN_INIT();
	TIM2_PWM_INIT();
	SYSCFG_INIT();
	EXTI_INIT();
	NVIC_INIT();

	while(1){

	}
	return 0;
}





/*IRQHANDLER fonksiyonlar teknik olarak ISR (Interrupt Service Routine) veya IRQ Handler (Kesme İşleyicisi) olarak adlandırılır.
 * Donanımsal Bir Entry Point (Giriş Noktasıdır): IRQ Handler'lar doğrudan donanım (CPU) tarafından tetiklenir.
 * Sen kodunun hiçbir yerinde EXTI1_IRQHandler(); şeklinde bir manuel çağrı yapmazsın.
 * Bu fonksiyonlar, işlemcinin donanım mimarisine gömülü özel yapılardır.
 * */






