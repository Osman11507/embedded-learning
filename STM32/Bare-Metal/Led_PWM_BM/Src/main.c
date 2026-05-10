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
	FLASH_ACR &= ~0x7;//temizleme
	FLASH_ACR |= (2<<0);

	RCC_CR &= ~(1 << 24);// PLLON değerini 0 yaparak kapatıyorum
	while(RCC_CR & (1<<25));

	RCC_PLLCFGR &= ~((0x3F<<0) | (0x1FF << 6) | (0x03 << 16));// M, N ve P alanlarını temizleme
	RCC_PLLCFGR |= (16<<0);
	RCC_PLLCFGR |= (336 << 6);
	RCC_PLLCFGR |= (1<<16);
	RCC_PLLCFGR |= (0 << 22);

	RCC_CR |= (1 << 24);//PLL yi tekrar açıyorum
	while(!(RCC_CR & (1<<25)));
	RCC_CFGR &= ~0x3;
	RCC_CFGR |= (2<<0);//saati pll olarak seçiyorum
	while ((RCC_CFGR & (0xC << 0)) != (0x8 << 0)); // SWS bitleri  ile PLL kullanılana kadar bekliyorum.

}



void GPIO_PWM_INIT(void){
	RCC_AHB1ENR |= (1<<0);

	GPIOA_MODER &= ~(0x3<<2);
	GPIOA_MODER |= (0x2<<2);

	GPIOA_AFRL &= ~(0xF<<4);
	GPIOA_AFRL |= (0x1<<4);


}



void TIM2_PWM_Init(void){
	RCC_APB1ENR &= ~0xF;
	RCC_APB1ENR |=(1<<0);
	TIM2_PSC = 83;
	TIM2_ARR = 999;

	TIM2_CCMR1 &= ~(0xFF<<8);
	TIM2_CCMR1 |= (6<<12);
	TIM2_CCMR1 |= (1<<11);

	TIM2_CCER |=(1<<4);

	TIM2_CR1 |= (1<<0);

}



void Delay_ms(uint32_t ms){
	STK_LOAD = 84000 - 1;
	STK_VAL = 0;
	STK_CTRL = 5;

	for (uint32_t i = 0; i < ms; i++) {
		while (!(STK_CTRL & (1 << 16)));
	    }
	STK_CTRL = 0;

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

