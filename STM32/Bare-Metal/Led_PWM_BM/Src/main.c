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

// --- GPIOA ---
#define GPIOA_BASE      0x40020000
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_OSPEEDER  (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOA_AFRL      (*(volatile uint32_t *)(GPIOA_BASE + 0x20))

// --- TIM2 ---
#define TIM2_BASE       0x40000000
#define TIM2_CR1        (*(volatile uint32_t *)(TIM2_BASE + 0x00))
#define TIM2_CCMR1      (*(volatile uint32_t *)(TIM2_BASE + 0x18))
#define TIM2_CCER       (*(volatile uint32_t *)(TIM2_BASE + 0x20))
#define TIM2_PSC        (*(volatile uint32_t *)(TIM2_BASE + 0x28))
#define TIM2_ARR        (*(volatile uint32_t *)(TIM2_BASE + 0x2C))
#define TIM2_CCR2       (*(volatile uint32_t *)(TIM2_BASE + 0x38))

//-----SysTıck------
#define STK_CTRL          (*(volatile uint32_t *)0xE000E010)
#define STK_LOAD          (*(volatile uint32_t *)0xE000E014)
#define STK_VAL           (*(volatile uint32_t *)0xE000E018)




void Clock_Config_84MHz(void){
/*Bu fonksyonun amacı işlemcinin kaynak hızını (16MHz) 84MHz ye çekmektr.*/
	FLASH_ACR &= ~0x7;//temizleme
	FLASH_ACR |= (2<<0);// flash gecikmesini 2ws olarak ayarlıyoruz
/*Flash ın 84Mhz de işlemci hızına yetişmesi mümkün değil. 84MHz de işlemci 11.9 ns de 1  işlem  yapar
 * Flash ise 30-40 ns arasında 1 veri sunar. Biz işlemciye sen 2 döngü sonrasındaki döngüde veri al. yani 3x11.9= 35.7ns de 1 flash dan veri al dedik. Bu istediğimiz aralıkta
 * RM0368 sayfa 48 de bulunan tablo*/

/*PLL (Phase Lock Loop) bizim işlemcimizin saatini istediğimiz değerde büyütme veya küçültmeye yarayan çarpan formülü. Bir nevi vites kutusu
 * */
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
/*Bu fonksiyonun amacı PA1 bacağını TIM2 nin bir uzantısı olarak ayarlamak*/
	RCC_AHB1ENR |= (1<<0);// GPIOA saatini aktif ediyorum

	GPIOA_MODER &= ~(0x3<<2);// Mod alanını temizliyorum
	GPIOA_MODER |= (0x2<<2);// mod olarak Altarnate Function ( özel görev Uart timer vs gibi) seçiyorum

	GPIOA_AFRL &= ~(0xF<<4);//görev alanını temizliyorum
	GPIOA_AFRL |= (0x1<<4);// Özel görev olarak TIM2 seçiyorum Yukardakinden farkı. Yukarıda sadece özel görev yapacaksın sen dedik. Burada ise senin görevin  artık TIM diyoruz.


}



void TIM2_PWM_Init(void){
/*Bu fok-nksiyonun amacı. İşlemcinin yüksek hızını 84MHz, LED'in pürüzsüz yanıp sönmesini sağlayacak 1kHz ye çevirmek*/
//PSC: İşlemci hızını böldüğümüz değer
// ARR TAvan değerimiz, kaça kadar sayacağız.
	RCC_APB1ENR &= ~0xF;//alan temizleme
	RCC_APB1ENR |=(1<<0);//TIM2 sayacını başlatdık
	TIM2_PSC = 83;//
	TIM2_ARR = 999;
/*Fpwm = Fclk/(PSC+1)x(ARR+1) yani 84Mhz/83+1 Mhz = 1Mhz, 1MHz/999+1 = 1kHz*/
	TIM2_CCMR1 &= ~(0xFF<<8);//Alan temizleme
	TIM2_CCMR1 |= (6<<12);//PWM mode 1
	TIM2_CCMR1 |= (1<<11);//OC2PE
/*CNT=0 dan ARR ye kadar sayan sayacım.
 * CCR2 Değeri eşik değer(main içerisinde)
 * OC2M sayacımın bu eşik değerine olan uzaklığına göre hangi işlemi yapmak istediğim( Bu kod için bu değer 110 yani PWM mode1
 * PWM mode 1 eğer  CNT CCR2 den küçükse pin high büyükse low yapar).
 * Mesela CCR2 = 100 ARR = 999 ve PWM mode 1 ise led 0 dan 100 e kadar high devamında 900 e kadar ise low sayar.  YANİ DUTY CYCLE AYARLIYORUM
 * Duty Cycle = (CCR2/ARR) X 100
 * OC2PE:  While döngüsü içeriinde CCR2 değerimi değiştirmek istediğimde bir glich oluşur . Biz burada CCR2 değerini değiştirmeden önce
 * o ankii CCR2 değerini ARR ye ulaşana kadar bekle diyoruz. Periyodu tamamla */

	TIM2_CCER |=(1<<4);// Sİnyali bacağa veriyorum

	TIM2_CR1 |= (1<<0);// saati başlatıyorum.

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





int main(void){
	Clock_Config_84MHz();
	GPIO_PWM_INIT();
	TIM2_PWM_Init();

	int i;

	while(1){
		for(i=0; i <= 999; i++){
			TIM2_CCR2 = i;
			Delay_ms(1);
		}


		for(i=999; i>=0; i--){
			TIM2_CCR2 = i;
			Delay_ms(1);
		}


	}

}

