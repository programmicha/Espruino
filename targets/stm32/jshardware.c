/*
 * jshardware.c
 *
 *  Created on: 8 Aug 2012
 *      Author: gw
 */

#if USB
 #ifdef STM32F1
  #include "usb_utils.h"
  #include "usb_lib.h"
  #include "usb_conf.h"
  #include "usb_pwr.h"
 #endif
#endif

#include "jshardware.h"
#include "jsutils.h"
#include "jsparse.h"
#include "jsinteractive.h"


 #if defined(STM32F4)
  #define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */
  #define FLASH_MEMORY_SIZE (1024*1024)
  #define FLASH_PAGE_SIZE (128*1024)
  #define FLASH_PAGES 1
 #elif defined(OLIMEXINO_STM32)
  #define FLASH_MEMORY_SIZE (128*1024)
  #define FLASH_PAGE_SIZE 1024
  #define FLASH_PAGES 14
 #else
  #define FLASH_MEMORY_SIZE (128*1024)
  #define FLASH_PAGE_SIZE 1024
  #define FLASH_PAGES 6
 #endif

#define FLASH_LENGTH (FLASH_PAGE_SIZE*FLASH_PAGES)
#if FLASH_LENGTH < 8+JSVAR_CACHE_SIZE*20
#error NOT ENOUGH ROOM IN FLASH - UNLESS WE ARE ONLY USING 16 bytes forJsVarRef ? FLASH_PAGES pages at FLASH_PAGE_SIZE bytes
#endif

#define FLASH_START (0x08000000 + FLASH_MEMORY_SIZE - FLASH_LENGTH)
#define FLASH_MAGIC_LOCATION (FLASH_START+FLASH_LENGTH-8)
#define FLASH_MAGIC 0xDEADBEEF

// ----------------------------------------------------------------------------
//                                                                        PINS
typedef struct IOPin {
  uint16_t pin;      // GPIO_Pin_1
  GPIO_TypeDef *gpio; // GPIOA
  uint8_t adc; // 0xFF or ADC_Channel_1
  /* timer - bits 0-1 = channel
             bits 2-5 = timer (NOTE - so only 0-15!)
             bit 6    = remap - for F1 part crazy stuff
             bit 7    = negated */
  uint8_t timer;
} PACKED_FLAGS IOPin;

#define TIMER(TIM,CH) (TIM<<2)|(CH-1)
#define TIMERN(TIM,CH) (TIM<<2)|(CH-1)|0x80
#define TIMNONE 0
#define TIMREMAP 64
#define TIMER_REMAP(X) ((X)&64)
#define TIMER_CH(X) (((X)&3)+1)
#define TIMER_TMR(X) (((X)>>2)&15)
#define TIMER_NEG(X) (((X)>>7)&1)

#ifdef STM32F4
 #define IOPINS 82 // 16*5+2
 // some of these use the same timer!
 const IOPin IOPIN_DATA[IOPINS] = {
 { GPIO_Pin_0,  GPIOA, ADC_Channel_0, TIMER(5,1) },
 { GPIO_Pin_1,  GPIOA, ADC_Channel_1, TIMER(5,2) },
 { GPIO_Pin_2,  0, 0xFF, TIMNONE },//A2 - Serial
 { GPIO_Pin_3,  0, 0xFF, TIMNONE },//A3 - Serial
 { GPIO_Pin_4,  GPIOA, ADC_Channel_4, TIMNONE },
 { GPIO_Pin_5,  GPIOA, ADC_Channel_5, TIMNONE },
 { GPIO_Pin_6,  GPIOA, ADC_Channel_6, TIMER(13,1) }, //+ TIMER(3,1)
 { GPIO_Pin_7,  GPIOA, ADC_Channel_7, TIMER(14,1) }, //+ TIMERN(8,1) + TIMER(3,2) + TIMERN(1,1)
 { GPIO_Pin_8,  GPIOA, 0xFF, TIMER(1,1) },
 { GPIO_Pin_9,  GPIOA, 0xFF, TIMER(1,2) }, 
 { GPIO_Pin_10, GPIOA, 0xFF, TIMER(1,3) }, 
 { GPIO_Pin_11, GPIOA, 0xFF, TIMER(1,4) },
 { GPIO_Pin_12, GPIOA, 0xFF, TIMNONE },
 { GPIO_Pin_13, GPIOA, 0xFF, TIMNONE },
 { GPIO_Pin_14, GPIOA, 0xFF, TIMNONE },
 { GPIO_Pin_15, GPIOA, 0xFF, TIMNONE },

 { GPIO_Pin_0,  GPIOB, ADC_Channel_8, TIMER(3,3) }, //+ TIMERN(8,2) + TIMERN(1,2)
 { GPIO_Pin_1,  GPIOB, ADC_Channel_9, TIMER(3,4) }, //+ TIMERN(8,3) + TIMERN(1,3)
 { GPIO_Pin_2,  GPIOB, 0xFF, TIMNONE },
 { GPIO_Pin_3,  GPIOB, 0xFF, TIMER(2,2) },
 { GPIO_Pin_4,  GPIOB, 0xFF, TIMER(3,1) },
 { GPIO_Pin_5,  GPIOB, 0xFF, TIMER(3,2) },
 { GPIO_Pin_6,  GPIOB, 0xFF, TIMER(4,1) },
 { GPIO_Pin_7,  GPIOB, 0xFF, TIMER(4,2) },
 { GPIO_Pin_8,  GPIOB, 0xFF, TIMER(4,3) }, //+TIMER(10,1)
 { GPIO_Pin_9,  GPIOB, 0xFF, TIMER(4,4) }, //+TIMER(11,1)
 { GPIO_Pin_10, GPIOB, 0xFF, TIMER(2,3) },
 { GPIO_Pin_11, GPIOB, 0xFF, TIMER(2,4) },
 { GPIO_Pin_12, GPIOB, 0xFF, TIMNONE },
 { GPIO_Pin_13, GPIOB, 0xFF, TIMERN(1,1) },
 { GPIO_Pin_14, GPIOB, 0xFF, TIMER(12,1) }, //+TIMERN(1,2)+TIMERN(8,2)
 { GPIO_Pin_15, GPIOB, 0xFF, TIMER(12,2) }, //+TIMERN(1,3)+TIMERN(8,3)

 { GPIO_Pin_0,  GPIOC, ADC_Channel_10, TIMNONE },
 { GPIO_Pin_1,  GPIOC, ADC_Channel_11, TIMNONE },
 { GPIO_Pin_2,  GPIOC, ADC_Channel_12, TIMNONE },
 { GPIO_Pin_3,  GPIOC, ADC_Channel_13, TIMNONE },
 { GPIO_Pin_4,  GPIOC, ADC_Channel_14, TIMNONE },
 { GPIO_Pin_5,  GPIOC, ADC_Channel_15, TIMNONE },
 { GPIO_Pin_6,  GPIOC, 0xFF, TIMER(8,1) }, //+, TIMER(3,1)
 { GPIO_Pin_7,  GPIOC, 0xFF, TIMER(8,2) },//+, TIMER(3,2)
 { GPIO_Pin_8,  GPIOC, 0xFF, TIMER(8,3) },//+, TIMER(3,3)
 { GPIO_Pin_9,  GPIOC, 0xFF, TIMER(8,4) }, //+, TIMER(3,4)
 { GPIO_Pin_10, GPIOC, 0xFF, TIMNONE },
 { GPIO_Pin_11, GPIOC, 0xFF, TIMNONE },
 { GPIO_Pin_12, GPIOC, 0xFF, TIMNONE },
 { GPIO_Pin_13, GPIOC, 0xFF, TIMNONE },
 { GPIO_Pin_14, GPIOC, 0xFF, TIMNONE },
 { GPIO_Pin_15, GPIOC, 0xFF, TIMNONE },

 { GPIO_Pin_0,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_1,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_2,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_3,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_4,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_5,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_6,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_7,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_8,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_9,  GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_10, GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_11, GPIOD, 0xFF, TIMNONE },
 { GPIO_Pin_12, GPIOD, 0xFF, TIMER(4,1) },
 { GPIO_Pin_13, GPIOD, 0xFF, TIMER(4,2) },
 { GPIO_Pin_14, GPIOD, 0xFF, TIMER(4,3) },
 { GPIO_Pin_15, GPIOD, 0xFF, TIMER(4,4) },

 { GPIO_Pin_0,  GPIOE, 0xFF },
 { GPIO_Pin_1,  GPIOE, 0xFF },
 { GPIO_Pin_2,  GPIOE, 0xFF },
 { GPIO_Pin_3,  GPIOE, 0xFF },
 { GPIO_Pin_4,  GPIOE, 0xFF },
 { GPIO_Pin_5,  GPIOE, 0xFF, TIMER(9,1) },
 { GPIO_Pin_6,  GPIOE, 0xFF, TIMER(9,2) },
 { GPIO_Pin_7,  GPIOE, 0xFF },
 { GPIO_Pin_8,  GPIOE, 0xFF, TIMERN(1,1) },
 { GPIO_Pin_9,  GPIOE, 0xFF, TIMER(1,1) },
 { GPIO_Pin_10, GPIOE, 0xFF, TIMERN(1,2) },
 { GPIO_Pin_11, GPIOE, 0xFF, TIMER(1,2) },
 { GPIO_Pin_12, GPIOE, 0xFF, TIMERN(1,3) },
 { GPIO_Pin_13, GPIOE, 0xFF, TIMER(1,3) },
 { GPIO_Pin_14, GPIOE, 0xFF, TIMER(1,4) },
 { GPIO_Pin_15, GPIOE, 0xFF },

 { GPIO_Pin_0,  GPIOH, 0xFF },
 { GPIO_Pin_1,  GPIOH, 0xFF },
};
#else
#if OLIMEXINO_STM32
#define IOPINS 39 // 16*3+2
const IOPin IOPIN_DATA[IOPINS] = {
    // A0-A5 are hared with D15-D20
#define A0_OFFSET 15
    // D0
{ GPIO_Pin_3,  GPIOA, ADC_Channel_3, TIMER(2,4) }, //, TIMER(15,2)
{ GPIO_Pin_2,  GPIOA, ADC_Channel_2, TIMER(2,3) }, // , TIMER(15,1)
{ GPIO_Pin_0,  GPIOA, ADC_Channel_0, TIMNONE },
{ GPIO_Pin_1,  GPIOA, ADC_Channel_1, TIMER(2,2) },
    // D4
{ GPIO_Pin_5,  GPIOB, 0xFF, TIMER(3,2)|TIMREMAP },
{ GPIO_Pin_6,  GPIOB, 0xFF, TIMER(4,1) },
{ GPIO_Pin_8,  GPIOA, 0xFF, TIMER(1,1) },
{ GPIO_Pin_9,  GPIOA, 0xFF, TIMER(1,2) }, // A9 - Serial
    // D8
{ GPIO_Pin_10, GPIOA, 0xFF, TIMER(1,3) }, // A10 - Serial
{ GPIO_Pin_7,  GPIOB, 0xFF, TIMER(4,2) },
{ GPIO_Pin_4,  GPIOA, ADC_Channel_4, TIMNONE },
{ GPIO_Pin_7,  GPIOA, ADC_Channel_7, TIMER(3,2) }, //, TIMER(17,1)
    // D12
{ GPIO_Pin_6,  GPIOA, ADC_Channel_6, TIMER(3,1) }, //, TIMER(16,1)
{ GPIO_Pin_5,  GPIOA, ADC_Channel_5, TIMNONE },
{ GPIO_Pin_8,  GPIOB, 0xFF, TIMER(4,3) }, //, TIMER(16,1)
    // D15-20 shared with A0-15
{ GPIO_Pin_0,  GPIOC, ADC_Channel_10, TIMNONE  },
{ GPIO_Pin_1,  GPIOC, ADC_Channel_11, TIMNONE  },
{ GPIO_Pin_2,  GPIOC, ADC_Channel_12, TIMNONE  },
{ GPIO_Pin_3,  GPIOC, ADC_Channel_13, TIMNONE  },
{ GPIO_Pin_4,  GPIOC, ADC_Channel_14, TIMNONE  },
{ GPIO_Pin_5,  GPIOC, ADC_Channel_15, TIMNONE  },
    // D21
{ GPIO_Pin_13, GPIOC, 0xFF, TIMNONE/*?*/ },
{ GPIO_Pin_14, GPIOC, 0xFF, TIMNONE/*?*/ },
{ GPIO_Pin_15, GPIOC, 0xFF, TIMNONE/*?*/ },
    // D24
{ GPIO_Pin_9,  GPIOB, 0xFF, TIMER(4,4) }, //, TIMER(17,1)
{ GPIO_Pin_2,  GPIOD, 0xFF, TIMNONE/*?*/ },
{ GPIO_Pin_10, GPIOC, 0xFF, TIMNONE },
{ GPIO_Pin_0,  GPIOB, ADC_Channel_8, TIMER(3,3) }, // , TIMERN(1,2)
    // D28
{ GPIO_Pin_1,  GPIOB, ADC_Channel_9, TIMER(3,4) }, // , TIMER(1,3)
{ GPIO_Pin_10, GPIOB, 0xFF, TIMER(2,3) },
{ GPIO_Pin_11, GPIOB, 0xFF, TIMER(2,4)|TIMREMAP },
{ GPIO_Pin_12, GPIOB, 0xFF, TIMNONE },
    // D32
{ GPIO_Pin_13, GPIOB, 0xFF, TIMERN(1,1) },
{ GPIO_Pin_14, GPIOB, 0xFF, TIMER(15,1)|TIMREMAP },// , TIMERN(1,2)
{ GPIO_Pin_15, GPIOB, 0xFF, TIMER(15,2)|TIMREMAP },// , TIMERN(1,3), TIMERN(15,1)
{ GPIO_Pin_6,  GPIOC, 0xFF, TIMER(3,1)|TIMREMAP },
   // D36
{ GPIO_Pin_7,  GPIOC, 0xFF, TIMER(3,2)|TIMREMAP },
{ GPIO_Pin_8,  GPIOC, 0xFF, TIMER(3,3)|TIMREMAP },
   // 38 - the index of the button
{ GPIO_Pin_9,  GPIOC, 0xFF, TIMER(3,4)|TIMREMAP },
// D2 ?
};
#else
 // STM32VLDISCOVERY
 #define IOPINS 50 // 16*3+2
 const IOPin IOPIN_DATA[IOPINS] = {
 { GPIO_Pin_0,  GPIOA, ADC_Channel_0, TIMNONE },
 { GPIO_Pin_1,  GPIOA, ADC_Channel_1, TIMER(2,2) },
 { GPIO_Pin_2,  GPIOA, ADC_Channel_2, TIMER(2,3) }, // , TIMER(15,1)
 { GPIO_Pin_3,  GPIOA, ADC_Channel_3, TIMER(2,4) }, //, TIMER(15,2)
 { GPIO_Pin_4,  GPIOA, ADC_Channel_4, TIMNONE },
 { GPIO_Pin_5,  GPIOA, ADC_Channel_5, TIMNONE },
 { GPIO_Pin_6,  GPIOA, ADC_Channel_6, TIMER(3,1) }, //, TIMER(16,1)
 { GPIO_Pin_7,  GPIOA, ADC_Channel_7, TIMER(3,2) }, //, TIMER(17,1)
 { GPIO_Pin_8,  GPIOA, 0xFF, TIMER(1,1) },
 { GPIO_Pin_9,  0,     0xFF, TIMER(1,2) }, // A9 - Serial
 { GPIO_Pin_10, 0,     0xFF, TIMER(1,3) }, // A10 - Serial
 { GPIO_Pin_11, GPIOA, 0xFF, TIMER(1,4) },
 { GPIO_Pin_12, GPIOA, 0xFF, TIMNONE },
 { GPIO_Pin_13, GPIOA, 0xFF, TIMNONE },
 { GPIO_Pin_14, GPIOA, 0xFF, TIMNONE },
 { GPIO_Pin_15, GPIOA, 0xFF, TIMNONE },

 { GPIO_Pin_0,  GPIOB, ADC_Channel_8, TIMER(3,3) }, // , TIMERN(1,2)
 { GPIO_Pin_1,  GPIOB, ADC_Channel_9, TIMER(3,4) }, // , TIMER(1,3)
 { GPIO_Pin_2,  GPIOB, 0xFF, TIMNONE },
 { GPIO_Pin_3,  GPIOB, 0xFF, TIMER(2,2) },
 { GPIO_Pin_4,  GPIOB, 0xFF, TIMER(3,1)|TIMREMAP },
 { GPIO_Pin_5,  GPIOB, 0xFF, TIMER(3,2)|TIMREMAP },
 { GPIO_Pin_6,  GPIOB, 0xFF, TIMER(4,1) },
 { GPIO_Pin_7,  GPIOB, 0xFF, TIMER(4,2) },
 { GPIO_Pin_8,  GPIOB, 0xFF, TIMER(4,3) }, //, TIMER(16,1)
 { GPIO_Pin_9,  GPIOB, 0xFF, TIMER(4,4) }, //, TIMER(17,1)
 { GPIO_Pin_10, GPIOB, 0xFF, TIMER(2,3) },
 { GPIO_Pin_11, GPIOB, 0xFF, TIMER(2,4)|TIMREMAP },
 { GPIO_Pin_12, GPIOB, 0xFF, TIMNONE },
 { GPIO_Pin_13, GPIOB, 0xFF, TIMERN(1,1) },
 { GPIO_Pin_14, GPIOB, 0xFF, TIMER(15,1)|TIMREMAP },// , TIMERN(1,2)
 { GPIO_Pin_15, GPIOB, 0xFF, TIMER(15,2)|TIMREMAP },// , TIMERN(1,3), TIMERN(15,1)

 { GPIO_Pin_0,  GPIOC, ADC_Channel_10, TIMNONE  },
 { GPIO_Pin_1,  GPIOC, ADC_Channel_11, TIMNONE  },
 { GPIO_Pin_2,  GPIOC, ADC_Channel_12, TIMNONE  },
 { GPIO_Pin_3,  GPIOC, ADC_Channel_13, TIMNONE  },
 { GPIO_Pin_4,  GPIOC, ADC_Channel_14, TIMNONE  },
 { GPIO_Pin_5,  GPIOC, ADC_Channel_15, TIMNONE  },
 { GPIO_Pin_6,  GPIOC, 0xFF, TIMER(3,1)|TIMREMAP },
 { GPIO_Pin_7,  GPIOC, 0xFF, TIMER(3,2)|TIMREMAP },
 { GPIO_Pin_8,  GPIOC, 0xFF, TIMER(3,3)|TIMREMAP },
 { GPIO_Pin_9,  GPIOC, 0xFF, TIMER(3,4)|TIMREMAP },
 { GPIO_Pin_10, GPIOC, 0xFF, TIMNONE },
 { GPIO_Pin_11, GPIOC, 0xFF, TIMNONE },
 { GPIO_Pin_12, GPIOC, 0xFF, TIMNONE },
 { GPIO_Pin_13, GPIOC, 0xFF, TIMNONE/*?*/ },
 { GPIO_Pin_14, GPIOC, 0xFF, TIMNONE/*?*/ },
 { GPIO_Pin_15, GPIOC, 0xFF, TIMNONE/*?*/ },

 { GPIO_Pin_0,  GPIOD, 0xFF, TIMNONE/*?*/ },
 { GPIO_Pin_1,  GPIOD, 0xFF, TIMNONE/*?*/ }, 
 // D2 ?
};
#endif// OLIMEXINO_STM32
#endif

uint8_t pinToPinSource(uint16_t pin) {
  if (pin==GPIO_Pin_0 ) return GPIO_PinSource0;
  if (pin==GPIO_Pin_1 ) return GPIO_PinSource1;
  if (pin==GPIO_Pin_2 ) return GPIO_PinSource2;
  if (pin==GPIO_Pin_3 ) return GPIO_PinSource3;
  if (pin==GPIO_Pin_4 ) return GPIO_PinSource4;
  if (pin==GPIO_Pin_5 ) return GPIO_PinSource5;
  if (pin==GPIO_Pin_6 ) return GPIO_PinSource6;
  if (pin==GPIO_Pin_7 ) return GPIO_PinSource7;
  if (pin==GPIO_Pin_8 ) return GPIO_PinSource8;
  if (pin==GPIO_Pin_9 ) return GPIO_PinSource9;
  if (pin==GPIO_Pin_10) return GPIO_PinSource10;
  if (pin==GPIO_Pin_11) return GPIO_PinSource11;
  if (pin==GPIO_Pin_12) return GPIO_PinSource12;
  if (pin==GPIO_Pin_13) return GPIO_PinSource13;
  if (pin==GPIO_Pin_14) return GPIO_PinSource14;
  if (pin==GPIO_Pin_15) return GPIO_PinSource15;
  return GPIO_PinSource0;
}
uint8_t pinToEVEXTI(uint16_t pin) {
  if (pin==GPIO_Pin_0 ) return EV_EXTI0;
  if (pin==GPIO_Pin_1 ) return EV_EXTI1;
  if (pin==GPIO_Pin_2 ) return EV_EXTI2;
  if (pin==GPIO_Pin_3 ) return EV_EXTI3;
  if (pin==GPIO_Pin_4 ) return EV_EXTI4;
  if (pin==GPIO_Pin_5 ) return EV_EXTI5;
  if (pin==GPIO_Pin_6 ) return EV_EXTI6;
  if (pin==GPIO_Pin_7 ) return EV_EXTI7;
  if (pin==GPIO_Pin_8 ) return EV_EXTI8;
  if (pin==GPIO_Pin_9 ) return EV_EXTI9;
  if (pin==GPIO_Pin_10) return EV_EXTI10;
  if (pin==GPIO_Pin_11) return EV_EXTI11;
  if (pin==GPIO_Pin_12) return EV_EXTI12;
  if (pin==GPIO_Pin_13) return EV_EXTI13;
  if (pin==GPIO_Pin_14) return EV_EXTI14;
  if (pin==GPIO_Pin_15) return EV_EXTI15;
  return EV_NONE;
}
uint8_t portToPortSource(GPIO_TypeDef *port) {
 #ifdef STM32F4
  if (port == GPIOA) return EXTI_PortSourceGPIOA;
  if (port == GPIOB) return EXTI_PortSourceGPIOB;
  if (port == GPIOC) return EXTI_PortSourceGPIOC;
  if (port == GPIOD) return EXTI_PortSourceGPIOD;
  if (port == GPIOE) return EXTI_PortSourceGPIOE;
  if (port == GPIOF) return EXTI_PortSourceGPIOF;
  if (port == GPIOG) return EXTI_PortSourceGPIOG;
  if (port == GPIOH) return EXTI_PortSourceGPIOH;
  return EXTI_PortSourceGPIOA;
#else
  if (port == GPIOA) return GPIO_PortSourceGPIOA;
  if (port == GPIOB) return GPIO_PortSourceGPIOB;
  if (port == GPIOC) return GPIO_PortSourceGPIOC;
  if (port == GPIOD) return GPIO_PortSourceGPIOD;
  if (port == GPIOE) return GPIO_PortSourceGPIOE;
  if (port == GPIOF) return GPIO_PortSourceGPIOF;
  if (port == GPIOG) return GPIO_PortSourceGPIOG;
  return GPIO_PortSourceGPIOA;
#endif
}

// ----------------------------------------------------------------------------
JsSysTime SysTickMajor = SYSTICK_RANGE;

#ifdef USB
unsigned int SysTickUSBWatchdog = 0;
void jshKickUSBWatchdog() {
  SysTickUSBWatchdog = 0;
}
#endif //USB


void jshDoSysTick() {
  SysTickMajor+=SYSTICK_RANGE;
#ifdef USB
  if (SysTickUSBWatchdog < SYSTICKS_BEFORE_USB_DISCONNECT) {
    SysTickUSBWatchdog++;
  }
#endif //USB
#ifdef USE_FILESYSTEM
  disk_timerproc();
#endif
}

// ----------------------------------------------------------------------------
void jshInit() {
  /* Enable UART and  GPIOx Clock */
 #ifdef STM32F4
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA |
                         RCC_AHB1Periph_GPIOB |
                         RCC_AHB1Periph_GPIOC | 
                         RCC_AHB1Periph_GPIOD |
                         RCC_AHB1Periph_GPIOE |
                         RCC_AHB1Periph_GPIOF |
                         RCC_AHB1Periph_GPIOG |
                         RCC_AHB1Periph_GPIOH, ENABLE);
 #else
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_ADC1 |
        RCC_APB2Periph_GPIOA |
        RCC_APB2Periph_GPIOB |
        RCC_APB2Periph_GPIOC |
        RCC_APB2Periph_GPIOD |
        RCC_APB2Periph_AFIO, ENABLE);
 #endif
  // Slow the IO clocks down - we don't need them going so fast!
#ifdef STM32VLDISCOVERY
  RCC_PCLK1Config(RCC_HCLK_Div2); // PCLK1 must be >8 Mhz for USB to work
  RCC_PCLK2Config(RCC_HCLK_Div4);
#else
  RCC_PCLK1Config(RCC_HCLK_Div8); // PCLK1 must be >8 Mhz for USB to work
  RCC_PCLK2Config(RCC_HCLK_Div16);
#endif
  /* System Clock */
  SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
  SysTick_Config(SYSTICK_RANGE-1); // 24 bit
  /* Initialise LEDs LD3&LD4, both on */
  GPIO_InitTypeDef GPIO_InitStructure;
#ifdef STM32F4
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
#endif
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
#ifdef LED1_PORT
  GPIO_InitStructure.GPIO_Pin = LED1_PIN;  
  GPIO_Init(LED1_PORT, &GPIO_InitStructure);
#endif
#ifdef LED2_PORT
  GPIO_InitStructure.GPIO_Pin = LED2_PIN;  
  GPIO_Init(LED2_PORT, &GPIO_InitStructure);
#endif
#ifdef LED3_PORT
  GPIO_InitStructure.GPIO_Pin = LED3_PIN;  
  GPIO_Init(LED3_PORT, &GPIO_InitStructure);
#endif
#ifdef LED4_PORT
  GPIO_InitStructure.GPIO_Pin = LED4_PIN;  
  GPIO_Init(LED4_PORT, &GPIO_InitStructure);
#endif

#ifdef STM32F4
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
#endif
#ifdef BTN_PORT  
  GPIO_InitStructure.GPIO_Pin = BTN_PIN;  
  GPIO_Init(BTN_PORT, &GPIO_InitStructure);
#endif
  GPIO_SetBits(LED1_PORT,LED1_PIN);

  if (DEFAULT_CONSOLE_DEVICE != EV_USBSERIAL)
    jshUSARTSetup(DEFAULT_CONSOLE_DEVICE, DEFAULT_BAUD_RATE);

  NVIC_InitTypeDef NVIC_InitStructure;
  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_Init(&NVIC_InitStructure);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
  NVIC_Init(&NVIC_InitStructure);

#ifdef STM32F4
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  ADC_CommonStructInit(&ADC_CommonInitStructure);
  ADC_CommonInitStructure.ADC_Mode			= ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler			= ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode		= ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay	        = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);
#endif

  // ADC Structure Initialization
  ADC_InitTypeDef ADC_InitStructure;
  ADC_StructInit(&ADC_InitStructure);

  // Preinit
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
#ifdef STM32F4
  ADC_InitStructure.ADC_ExternalTrigConvEdge		= ADC_ExternalTrigConvEdge_None;
#else
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
#endif
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
  ADC_Init(ADC1, &ADC_InitStructure);

  // Enable the ADC
  ADC_Cmd(ADC1, ENABLE);

#ifdef STM32F4
  // No calibration??
#else
  // Calibrate
  ADC_ResetCalibration(ADC1);
  while(ADC_GetResetCalibrationStatus(ADC1));
  ADC_StartCalibration(ADC1);
  while(ADC_GetCalibrationStatus(ADC1));
#endif

  jsiConsolePrintInt(SystemCoreClock/1000000);jsiConsolePrint(" Mhz\r\n");

  // Turn led off - so we know we have initialised
  GPIO_ResetBits(LED1_PORT,LED1_PIN);
}

void jshKill() {
}

void jshIdle() {
#ifdef USB
  static bool wasUSBConnected = false;
  bool USBConnected = jshIsUSBSERIALConnected();
  if (wasUSBConnected != USBConnected) {
    wasUSBConnected = USBConnected;
    if (USBConnected)
      jsiSetConsoleDevice(EV_USBSERIAL);
    else {
      jsiSetConsoleDevice(DEFAULT_CONSOLE_DEVICE);
      jshTransmitClearDevice(EV_USBSERIAL); // clear the transmit queue
    }
  }
#endif
}

// ----------------------------------------------------------------------------

bool jshIsUSBSERIALConnected() {
#ifdef USB
  return SysTickUSBWatchdog < SYSTICKS_BEFORE_USB_DISCONNECT;
  // not a check for connected - we just want to have some idea...
#else
  return false;
#endif
}

JsSysTime jshGetTimeFromMilliseconds(JsVarFloat ms) {
  return (JsSysTime)((ms*SystemCoreClock)/1000);
}

JsVarFloat jshGetMillisecondsFromTime(JsSysTime time) {
  return ((JsVarFloat)time)*1000/SystemCoreClock;
}


JsSysTime jshGetSystemTime() {
  JsSysTime t1 = SysTickMajor;
  JsSysTime time = (JsSysTime)SysTick->VAL;
  JsSysTime t2 = SysTickMajor;
  // times are different and systick has rolled over
  if (t1!=t2 && time > (SYSTICK_RANGE>>1)) 
    return t2 - time;
  return t1-time;
}

// ----------------------------------------------------------------------------

int jshGetPinFromString(const char *s) {
  // built in constants
#ifdef BTN_PININDEX
  if (s[0]=='B' && s[1]=='T' && s[2]=='N' && !s[3])
    return BTN_PININDEX;
#endif
#if defined(LED1_PININDEX) || defined(LED2_PININDEX) || defined(LED3_PININDEX) || defined(LED4_PININDEX)
  if (s[0]=='L' && s[1]=='E' && s[2]=='D') {
#ifdef LED1_PININDEX
    if (s[3]=='1' && !s[4]) return LED1_PININDEX;
#endif
#ifdef LED2_PININDEX
    if (s[3]=='2' && !s[4]) return LED2_PININDEX;
#endif
#ifdef LED3_PININDEX
    if (s[3]=='3' && !s[4]) return LED3_PININDEX;
#endif
#ifdef LED4_PININDEX
    if (s[3]=='4' && !s[4]) return LED4_PININDEX;
#endif
  }
#endif

#if defined(OLIMEXINO_STM32)
    // A0-A5 and D0-D37
    if (s[0]=='A' && s[1]) { // first 6 are analogs
      if (!s[2] && (s[1]>='0' && s[1]<='5'))
        return A0_OFFSET + (s[1]-'0');
    } else if (s[0]=='D' && s[1]) {
      if (!s[2] && (s[1]>='0' && s[1]<='9')) { // D0-D9
        return s[1]-'0';
      } else if (!s[3] && (s[1]>='1' && s[1]<='3' && s[2]>='0' && s[2]<='9')) { // D1X-D3X
        int n = (s[1]-'0')*10 + (s[2]-'0');
        if (n<38) return n;
      }
    }
#else // !OLIMEX
    if ((s[0]=='A' || s[0]=='B' || s[0]=='C' || s[0]=='D' || s[0]=='E' || s[0]=='H') && s[1]) { // first 6 are analogs
      int port = (s[0]=='H')?5:(s[0]-'A'); // GPIOH is strange because there is no F or G exposed!
      if (!s[2] && (s[1]>='0' && s[1]<='9')) { // D0-D9
        return (port*16) + (s[1]-'0');
      } else if (!s[3] && (s[1]>='1' && s[1]<='3' && s[2]>='0' && s[2]<='9')) { // D1X-D3X
        int n = (s[1]-'0')*10 + (s[2]-'0');
        int pin = (port*16)+n;
        if (pin<IOPINS) return pin;
      }
    }
#endif
  return -1;
}

bool jshPinInput(int pin) {
  bool value = false;
  if (pin>=0 && pin < IOPINS && IOPIN_DATA[pin].gpio) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = IOPIN_DATA[pin].pin;
#ifdef STM32F4    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;    
#else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
#endif
    GPIO_Init(IOPIN_DATA[pin].gpio, &GPIO_InitStructure);


    value = GPIO_ReadInputDataBit(IOPIN_DATA[pin].gpio, IOPIN_DATA[pin].pin) ? 1 : 0;
  } else jsError("Invalid pin!");
  return value;
}

JsVarFloat jshPinAnalog(int pin) {
  JsVarFloat value = 0;
  if (pin>=0 && pin < IOPINS && IOPIN_DATA[pin].gpio && IOPIN_DATA[pin].adc!=0xFF) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = IOPIN_DATA[pin].pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
#ifdef STM32F4    
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;    
#endif
    GPIO_Init(IOPIN_DATA[pin].gpio, &GPIO_InitStructure);

    // Configure chanel
#ifdef STM32F4    
    ADC_RegularChannelConfig(ADC1, IOPIN_DATA[pin].adc, 1, ADC_SampleTime_480Cycles);
#else
    ADC_RegularChannelConfig(ADC1, IOPIN_DATA[pin].adc, 1, ADC_SampleTime_239Cycles5/*ADC_SampleTime_55Cycles5*/);
#endif

    // Start the conversion
#ifdef STM32F4    
    ADC_SoftwareStartConv(ADC1);
#else
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
#endif

    // Wait until conversion completion
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);

    // Get the conversion value
    value = ADC_GetConversionValue(ADC1) / (JsVarFloat)65535;
  } else jsError("Invalid analog pin!");
  return value;
}

static inline void jshSetPinValue(int pin, bool value) {
#ifdef STM32F4 
    if (value)
      GPIO_SetBits(IOPIN_DATA[pin].gpio, IOPIN_DATA[pin].pin);
    else
      GPIO_ResetBits(IOPIN_DATA[pin].gpio, IOPIN_DATA[pin].pin);
#else
    if (value)
      IOPIN_DATA[pin].gpio->BSRR = IOPIN_DATA[pin].pin;
    else
      IOPIN_DATA[pin].gpio->BRR = IOPIN_DATA[pin].pin;
#endif
}

void jshPinOutput(int pin, bool value) {
  if (pin>=0 && pin < IOPINS && IOPIN_DATA[pin].gpio) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = IOPIN_DATA[pin].pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
#ifdef STM32F4 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
#else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
#endif
    GPIO_Init(IOPIN_DATA[pin].gpio, &GPIO_InitStructure);

    jshSetPinValue(pin, value);
  } else jsError("Invalid pin!");
}

void jshPinAnalogOutput(int pin, JsVarFloat value) {
  if (value<0) value=0;
  if (value>1) value=1;
  if (pin>=0 && pin < IOPINS && IOPIN_DATA[pin].gpio && IOPIN_DATA[pin].timer!=TIMNONE) {
    TIM_TypeDef* TIMx;
#ifdef STM32F4
 #define STM32F4ONLY(X) X
    uint8_t afmap;
#else
 #define STM32F4ONLY(X)
#endif

    if (TIMER_TMR(IOPIN_DATA[pin].timer)==1) {
      TIMx = TIM1;
      STM32F4ONLY(afmap=GPIO_AF_TIM1);
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);  
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==2)  {
      TIMx = TIM2;
      STM32F4ONLY(afmap=GPIO_AF_TIM2);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); 
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==3)  {
      TIMx = TIM3;
      STM32F4ONLY(afmap=GPIO_AF_TIM3);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);  
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==4)  {
      TIMx = TIM4;
      STM32F4ONLY(afmap=GPIO_AF_TIM4);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);  
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==5) {
      TIMx = TIM5;
      STM32F4ONLY(afmap=GPIO_AF_TIM5);
       RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);  
/*    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==6)  { // Not used for outputs
      TIMx = TIM6;
      STM32F4ONLY(afmap=GPIO_AF_TIM6);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);  
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==7)  { // Not used for outputs
      TIMx = TIM7;
      STM32F4ONLY(afmap=GPIO_AF_TIM7);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE); */
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==8) {
      TIMx = TIM8;
      STM32F4ONLY(afmap=GPIO_AF_TIM8);
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);  
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==9)  {
      TIMx = TIM9;
      STM32F4ONLY(afmap=GPIO_AF_TIM9);
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, ENABLE);  
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==10)  {
      TIMx = TIM10;
      STM32F4ONLY(afmap=GPIO_AF_TIM10);
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM10, ENABLE); 
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==11)  {
      TIMx = TIM11;
      STM32F4ONLY(afmap=GPIO_AF_TIM11);
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM11, ENABLE); 
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==12)  {
      TIMx = TIM12;
      STM32F4ONLY(afmap=GPIO_AF_TIM12);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12, ENABLE); 
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==13)  {
      TIMx = TIM13;
      STM32F4ONLY(afmap=GPIO_AF_TIM13);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM13, ENABLE); 
    } else if (TIMER_TMR(IOPIN_DATA[pin].timer)==14)  {
      TIMx = TIM14;
      STM32F4ONLY(afmap=GPIO_AF_TIM14);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE); 
    } else return; // eep!
    //   /* Compute the prescaler value */
  

  /* Time base configuration */
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
//  PrescalerValue = (uint16_t) ((SystemCoreClock /2) / 28000000) - 1;
//  TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure);

  /* PWM1 Mode configuration*/
  TIM_OCInitTypeDef  TIM_OCInitStructure;
  TIM_OCStructInit(&TIM_OCInitStructure);
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = (uint32_t)(value*TIM_TimeBaseStructure.TIM_Period);
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

  if (TIMER_CH(IOPIN_DATA[pin].timer)==1) {
    TIM_OC1Init(TIMx, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIMx, TIM_OCPreload_Enable);
  } else if (TIMER_CH(IOPIN_DATA[pin].timer)==2) {
    TIM_OC2Init(TIMx, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIMx, TIM_OCPreload_Enable);
  } else if (TIMER_CH(IOPIN_DATA[pin].timer)==3) {
    TIM_OC3Init(TIMx, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIMx, TIM_OCPreload_Enable);
  } else if (TIMER_CH(IOPIN_DATA[pin].timer)==4) {
    TIM_OC4Init(TIMx, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIMx, TIM_OCPreload_Enable);
  }
  TIM_ARRPreloadConfig(TIMx, ENABLE); // ARR = Period. Not sure if we need preloads?

  // enable the timer
  TIM_Cmd(TIMx, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = IOPIN_DATA[pin].pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
#ifdef STM32F4 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ; // required?
#else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#endif
    GPIO_Init(IOPIN_DATA[pin].gpio, &GPIO_InitStructure);
#ifdef STM32F4
    // connect timer pin up
    GPIO_PinAFConfig(IOPIN_DATA[pin].gpio, pinToPinSource(IOPIN_DATA[pin].pin), afmap);
#else
    if (TIMER_REMAP(IOPIN_DATA[pin].timer)) {
      if (TIMER_TMR(IOPIN_DATA[pin].timer)==1) GPIO_PinRemapConfig( GPIO_FullRemap_TIM1, ENABLE );
      else if (TIMER_TMR(IOPIN_DATA[pin].timer)==2) GPIO_PinRemapConfig( GPIO_FullRemap_TIM2, ENABLE );
      else if (TIMER_TMR(IOPIN_DATA[pin].timer)==3) GPIO_PinRemapConfig( GPIO_FullRemap_TIM3, ENABLE );
      else if (TIMER_TMR(IOPIN_DATA[pin].timer)==4) GPIO_PinRemapConfig( GPIO_Remap_TIM4, ENABLE );
      else if (TIMER_TMR(IOPIN_DATA[pin].timer)==15) GPIO_PinRemapConfig( GPIO_Remap_TIM15, ENABLE );
      else jsError("(internal) Remap needed, but unknown timer."); 
    }
#endif

  } else jsError("Invalid pin, or pin not capable of analog output!");
}

void jshPinPulse(int pin, bool value, JsVarFloat time) {
 JsSysTime ticks = jshGetTimeFromMilliseconds(time);
 //jsPrintInt(ticks);jsPrint("\n");
  if (pin>=0 && pin < IOPINS && IOPIN_DATA[pin].gpio) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = IOPIN_DATA[pin].pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
#ifdef STM32F4 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
#else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
#endif
    GPIO_Init(IOPIN_DATA[pin].gpio, &GPIO_InitStructure);


    jshSetPinValue(pin, value);
    JsSysTime starttime = jshGetSystemTime();
    JsSysTime endtime = starttime + ticks;
    //jsPrint("----------- ");jsPrintInt(endtime>>16);jsPrint("\n");
    JsSysTime stime = jshGetSystemTime();
    while (stime>=starttime && stime<endtime && !jspIsInterrupted()) { // this stops rollover issue
      stime = jshGetSystemTime();
      //jsPrintInt(stime>>16);jsPrint("\n");
    }
    jshSetPinValue(pin, !value);
  } else jsError("Invalid pin!");
}

void jshPinWatch(int pin, bool shouldWatch) {
  if (pin>=0 && pin < IOPINS && IOPIN_DATA[pin].gpio) {
    // TODO: check for DUPs, also disable interrupt
    int idx = pinToPinSource(IOPIN_DATA[pin].pin);
    /*if (pinInterrupt[idx].pin>PININTERRUPTS) jsError("Interrupt already used");
    pinInterrupt[idx].pin = pin;
    pinInterrupt[idx].fired = false;
    pinInterrupt[idx].callbacks = ...;*/

    // set as input
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = IOPIN_DATA[pin].pin;
#ifdef STM32F4    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;    
#else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
#endif
    GPIO_Init(IOPIN_DATA[pin].gpio, &GPIO_InitStructure);

#ifdef STM32F4 
    SYSCFG_EXTILineConfig(portToPortSource(IOPIN_DATA[pin].gpio), pinToPinSource(IOPIN_DATA[pin].pin));
#else
    GPIO_EXTILineConfig(portToPortSource(IOPIN_DATA[pin].gpio), pinToPinSource(IOPIN_DATA[pin].pin));
#endif

    EXTI_InitTypeDef s;
    EXTI_StructInit(&s);
    s.EXTI_Line = IOPIN_DATA[pin].pin; //EXTI_Line0
    s.EXTI_Mode =  EXTI_Mode_Interrupt;
    s.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    s.EXTI_LineCmd = ENABLE;
    EXTI_Init(&s);
  } else jsError("Invalid pin!");
}

bool jshIsEventForPin(IOEvent *event, int pin) {
  return IOEVENTFLAGS_GETTYPE(event->flags) == pinToEVEXTI(IOPIN_DATA[pin].pin);
}

USART_TypeDef* getUsartFromDevice(IOEventFlags device) {
 switch (device) {
   case EV_SERIAL1 : return USART1;
   case EV_SERIAL2 : return USART2;
   case EV_SERIAL3 : return USART3;
#if USARTS>=4
   case EV_SERIAL4 : return UART4;
#endif
#if USARTS>=5
   case EV_SERIAL5 : return UART5;
#endif
#if USARTS>=6
   case EV_SERIAL6 : return USART6;
#endif
   default: return 0;
 }
}

void jshUSARTSetup(IOEventFlags device, int baudRate) {
  uint16_t pinRX, pinTX;// eg. GPIO_Pin_1
  GPIO_TypeDef *gpioRX, *gpioTX;   // eg. GPIOA

  USART_TypeDef *usart = getUsartFromDevice(device);
  uint8_t usartIRQ;

  if (device == EV_SERIAL1) {
    usartIRQ = USART1_IRQn;
    gpioRX = USART1_PORT;
    gpioTX = USART1_PORT;
    pinRX = USART1_PIN_RX;
    pinTX = USART1_PIN_TX;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  } else if (device == EV_SERIAL2) {
    usartIRQ = USART2_IRQn;
    gpioRX = USART2_PORT;
    gpioTX = USART2_PORT;
    pinRX = USART2_PIN_RX;
    pinTX = USART2_PIN_TX;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
#if USARTS>= 3
  } else if (device == EV_SERIAL3) {
    usartIRQ = USART3_IRQn;
    gpioRX = USART3_PORT;
    gpioTX = USART3_PORT;
    pinRX = USART3_PIN_RX;
    pinTX = USART3_PIN_TX;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
#endif
#if USARTS>= 4
  } else if (device == EV_SERIAL4) {
    usartIRQ = UART4_IRQn;
    gpioRX = USART4_PORT;
    gpioTX = USART4_PORT;
    pinRX = USART4_PIN_RX;
    pinTX = USART4_PIN_TX;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
#endif
#if USARTS>= 5
  } else if (device == EV_SERIAL5) {
    usartIRQ = UART5_IRQn;
    gpioRX = USART5_PORT_RX;
    gpioTX = USART5_PORT_TX;
    pinRX = USART5_PIN_RX;
    pinTX = USART5_PIN_TX;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
#endif
#if USARTS>= 6
  } else if (device == EV_SERIAL6) {
    usartIRQ = USART6_IRQn;
    gpioRX = USART6_PORT;
    gpioTX = USART6_PORT;
    pinRX = USART6_PIN_RX;
    pinTX = USART6_PIN_TX;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
#endif
  } else {
    jsError("internal: Unknown serial port device.");
    return;
  }

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = pinRX;
#ifdef STM32F4
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
#endif
  GPIO_Init(gpioRX, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = pinTX;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
#ifdef STM32F4
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; // alternate fn
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
#else
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
#endif
  GPIO_Init(gpioTX, &GPIO_InitStructure);

  if (device == EV_SERIAL1) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  } else if (device == EV_SERIAL2) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  }

#ifdef STM32F4
  if (device == EV_SERIAL1) {
    GPIO_PinAFConfig(gpioRX, pinToPinSource(pinRX), GPIO_AF_USART1);
    GPIO_PinAFConfig(gpioTX, pinToPinSource(pinTX), GPIO_AF_USART1);
  } else if (device == EV_SERIAL2) {
    GPIO_PinAFConfig(gpioRX, pinToPinSource(pinRX), GPIO_AF_USART2);
    GPIO_PinAFConfig(gpioTX, pinToPinSource(pinTX), GPIO_AF_USART2);
  } else if (device == EV_SERIAL3) {
    GPIO_PinAFConfig(gpioRX, pinToPinSource(pinRX), GPIO_AF_USART3);
    GPIO_PinAFConfig(gpioTX, pinToPinSource(pinTX), GPIO_AF_USART3);
  } else if (device == EV_SERIAL4) {
    GPIO_PinAFConfig(gpioRX, pinToPinSource(pinRX), GPIO_AF_UART4);
    GPIO_PinAFConfig(gpioTX, pinToPinSource(pinTX), GPIO_AF_UART4);
  } else if (device == EV_SERIAL5) {
    GPIO_PinAFConfig(gpioRX, pinToPinSource(pinRX), GPIO_AF_UART5);
    GPIO_PinAFConfig(gpioTX, pinToPinSource(pinTX), GPIO_AF_UART5);
  } else if (device == EV_SERIAL6) {
    GPIO_PinAFConfig(gpioRX, pinToPinSource(pinRX), GPIO_AF_USART6);
    GPIO_PinAFConfig(gpioTX, pinToPinSource(pinTX), GPIO_AF_USART6);
  }
#endif

  USART_ClockInitTypeDef USART_ClockInitStructure;
  USART_ClockStructInit(&USART_ClockInitStructure);
  USART_ClockInit(usart, &USART_ClockInitStructure);

  USART_InitTypeDef USART_InitStructure;
  USART_InitStructure.USART_BaudRate = baudRate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No ;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_Init(usart, &USART_InitStructure);

  // enable uart interrupt
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = usartIRQ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init( &NVIC_InitStructure );
  // Enable RX interrupt (TX is done when we have bytes)
  USART_ClearITPendingBit(usart, USART_IT_RXNE);
  USART_ITConfig(usart, USART_IT_RXNE, ENABLE);
  //Enable USART
  USART_Cmd(usart, ENABLE);
}

/** Kick a device into action (if required). For instance we may need
 * to set up interrupts */
void jshUSARTKick(IOEventFlags device) {
  USART_TypeDef *uart = getUsartFromDevice(device);
    if (uart) USART_ITConfig(uart, USART_IT_TXE, ENABLE);
}


void jshSaveToFlash() {
#ifdef STM32F4 
  FLASH_Unlock();
#else
  FLASH_UnlockBank1();
#endif

  int i;
  /* Clear All pending flags */
#ifdef STM32F4 
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
#else
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
#endif

  jsiConsolePrint("Erasing Flash...");
#ifdef STM32F4 
  FLASH_EraseSector(FLASH_Sector_11, VoltageRange_3);
#else
  /* Erase the FLASH pages */
  for(i=0;i<FLASH_PAGES;i++) {
    FLASH_ErasePage(FLASH_START + (FLASH_PAGE_SIZE * i));
    jsiConsolePrint(".");
  }
#endif
  jsiConsolePrint("\nProgramming ");
  jsiConsolePrintInt(jsvGetVarDataSize());
  jsiConsolePrint(" Bytes...");

  int *basePtr = jsvGetVarDataPointer();
#ifdef STM32F4
  for (i=0;i<jsvGetVarDataSize();i+=4) {
      while (FLASH_ProgramWord(FLASH_START+i, basePtr[i>>2]) != FLASH_COMPLETE);
      if ((i&1023)==0) jsiConsolePrint(".");
  }
  while (FLASH_ProgramWord(FLASH_MAGIC_LOCATION, FLASH_MAGIC) != FLASH_COMPLETE);
#else
  /* Program Flash Bank1 */  
  for (i=0;i<jsvGetVarDataSize();i+=4) {
      FLASH_ProgramWord(FLASH_START+i, basePtr[i>>2]);
      if ((i&1023)==0) jsiConsolePrint(".");
  }
  FLASH_ProgramWord(FLASH_MAGIC_LOCATION, FLASH_MAGIC);
  FLASH_WaitForLastOperation(0x2000);
#endif
#ifdef STM32F4 
  FLASH_Lock();
#else
  FLASH_LockBank1();
#endif
  jsiConsolePrint("\nChecking...");

  int errors = 0;
  for (i=0;i<jsvGetVarDataSize();i+=4)
    if ((*(int*)(FLASH_START+i)) != basePtr[i>>2])
      errors++;

  if (errors) {
      jsiConsolePrint("\nThere were ");
      jsiConsolePrintInt(errors);
      jsiConsolePrint(" errors!\n>");
  } else
      jsiConsolePrint("\nDone!\n>");

//  This is nicer, but also broken!
//  FLASH_UnlockBank1();
//  /* Clear All pending flags */
//  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
//
//  size_t varDataSize = jsvGetVarDataSize();
//  int *basePtr = jsvGetVarDataPointer();
//
//  int page;
//  for(page=0;page<FLASH_PAGES;page++) {
//    jsPrint("Flashing Page ");jsPrintInt(page);jsPrint("...\n");
//    size_t pageOffset = (FLASH_PAGE_SIZE * page);
//    size_t pagePtr = FLASH_START + pageOffset;
//    size_t pageSize = varDataSize-pageOffset;
//    if (pageSize>FLASH_PAGE_SIZE) pageSize = FLASH_PAGE_SIZE;
//    jsPrint("Offset ");jsPrintInt(pageOffset);jsPrint(", Size ");jsPrintInt(pageSize);jsPrint(" bytes\n");
//    bool first = true;
//    int errors = 0;
//    int i;
//    for (i=pageOffset;i<pageOffset+pageSize;i+=4)
//      if ((*(int*)(FLASH_START+i)) != basePtr[i>>2])
//        errors++;
//    while (errors && !jspIsInterrupted()) {
//      if (!first) { jsPrintInt(errors);jsPrint(" errors - retrying...\n"); }
//      first = false;
//      /* Erase the FLASH page */
//      FLASH_ErasePage(pagePtr);
//      /* Program Flash Bank1 */
//      for (i=pageOffset;i<pageOffset+pageSize;i+=4)
//          FLASH_ProgramWord(FLASH_START+i, basePtr[i>>2]);
//      FLASH_WaitForLastOperation(0x20000);
//    }
//  }
//  // finally flash magic byte
//  FLASH_ProgramWord(FLASH_MAGIC_LOCATION, FLASH_MAGIC);
//  FLASH_WaitForLastOperation(0x20000);
//  FLASH_LockBank1();
//  if (*(int*)FLASH_MAGIC_LOCATION != FLASH_MAGIC)
//    jsPrint("Flash magic word not flashed correctly!\n");
//  jsPrint("Flashing Complete\n");

  /*jsPrint("Magic contains ");
  jsPrintInt(*(int*)FLASH_MAGIC_LOCATION);
  jsPrint(" we want ");
  jsPrintInt(FLASH_MAGIC);
  jsPrint("\n");*/
}

void jshLoadFromFlash() {
  jsiConsolePrint("\nLoading ");
  jsiConsolePrintInt(jsvGetVarDataSize());
  jsiConsolePrint(" bytes from flash...");
  memcpy(jsvGetVarDataPointer(), (int*)FLASH_START, jsvGetVarDataSize());
  jsiConsolePrint("\nDone!\n>");
}

bool jshFlashContainsCode() {
  /*jsPrint("Magic contains ");
  jsPrintInt(*(int*)FLASH_MAGIC_LOCATION);
  jsPrint("we want");
  jsPrintInt(FLASH_MAGIC);
  jsPrint("\n");*/
  return (*(int*)FLASH_MAGIC_LOCATION) == FLASH_MAGIC;
}


/// Enter simple sleep mode (can be woken up by interrupts)
void jshSleep() {
  //jshPinOutput(LED1_PININDEX,1);
  __WFI(); // Wait for Interrupt
  //jshPinOutput(LED1_PININDEX,0);
}
