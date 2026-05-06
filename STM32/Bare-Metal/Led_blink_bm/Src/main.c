/* Tip Tanımlamaları */
typedef unsigned int uint32_t;


/* Reset and Clock Control. STM32 de işlemcileri temelde, başlangıçta içerisindeki tüm birimlerin saatleri kapalı olarak gelir. RCC ile bu saatleri
 * açıp kapatabiliyoruz veya hızlarını ayarlayabiliyoruz. Bu işlemi yapmadan bir bacağa veya bir birime sinyal verip sinyal  alamayız. Rcc ile busların hızlarını da kontrol edderiz
 * burada yazdığımız AHB1 Advanced High Performance bus'ı anlamına geliyor. Bu bus da GPIO bulunuyor ENR ise Enable anlamına gelmekte yani saati açıyoruz*/
#define RCC_BASE          0x40023800
#define RCC_AHB1ENR       (*(volatile uint32_t *)(RCC_BASE + 0x30))


#define GPIOA_BASE        0x40020000
#define GPIOA_MODER       (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_ODR         (*(volatile uint32_t *)(GPIOA_BASE + 0x14))

/*STK systick anlamına gelmekte ARM cortex-m4 çekirdeklerinin içine yerleştirilmiş 24 bitlik bir geri sayım sayacı ( kısaca bu kadar devamı ve detayı PM0214 programing manual da */
#define STK_CTRL          (*(volatile uint32_t *)0xE000E010)
#define STK_LOAD          (*(volatile uint32_t *)0xE000E014)
#define STK_VAL           (*(volatile uint32_t *)0xE000E018)


void delay_ms(uint32_t ms) {
/*Sayaç load değerinden itibaren geriye doğru saymaya başlar. İşlemcimizin frekansı 16Mhz (balangıçta) yani 1 saniyede 16.000.000 cycle demektir
 * 1 mili saniye için ise bu değer 16.000 dır. Load değerimiz milisaniyeye göre ayarlaşmıştır. 16000-1; neden 1 çıkarıyoruz. Çünkü sayacımızın tam olarak
 * 16 bin kere vurmasını istiyoruz. -1  işlemini yapmazask 0 için de 1 vurum harcayarak 16001 vurum olacak. Bu istediğimiz vurum değildir. STK_CTRL = 5 ile (101 binary) bit 0 (enable = 1)
 * sayacı başlatma işlemi, bit 2 (CLKSOURCE = 1) sayacın işlemci bitiyle senkron şalışmasını sağlarız. ( Daha detayı için PM0214 programing manuala bakılmalı)  */

    STK_LOAD = 16000 - 1;
    STK_VAL = 0;
    STK_CTRL = 5;

    for (uint32_t i = 0; i < ms; i++) {

        while (!(STK_CTRL & (1 << 16)));/* Bu satır asıl gecikme işleminin gerçekleştiği satırdır.*/
    }
    STK_CTRL = 0;
}


int main(void) {


    RCC_AHB1ENR |= (1 << 0);




    GPIOA_MODER &= ~(3 << 2);
    GPIOA_MODER |= (1 << 2);

    while (1) {


        GPIOA_ODR ^= (1 << 1);


        delay_ms(500);
    }
}
/*STK_CTRL nin 16. biti countflagdir. Count flag donanımın saymaya işlemini bitirdiğini yazılıma bildirir.
 * biz bu işlemi while dngüsü içerisinde kontrol ederiz. Eğer bayrak kalktıysa(1) while döngüsü kırılır. for dönüsü
 * 1 artar. sonra sayaç 16.000 vurum sayana kadar while içerisi 1 olur ve döngü devam eder(işlemci orada bekler çünkü saymaya devam ediyor)*/
