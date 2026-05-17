/*register adreslerini sürekli yazmamak için burada tutacağım. İleride struct kullanarak bir CMSIS şeklinde tutarım. Ama şimdilik dfine ile ilerleyeceğım*/

#ifndef STM32F401_REGS_H_ //if not defined: hafizada daha önce bu .h dosyasının etiketi tanımlandı mı tanımlanmadı mı kontrol eder. Eğer proje ilk kez derleniyorsa tanımlanmamıştır. Derleyici içeriye girer
#define STM32F401_REGS_H_// derleyici içeriye girer girmez bu etiketi hafızasına kayıt eder, ve altındaki tüm satırları okumaya başlar

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
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44))

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
#define TIM2_SR         (*(volatile uint32_t *)(TIM2_BASE + 0x10))
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

#endif// koruma satırı. Bittiğini belirtiyoruz
