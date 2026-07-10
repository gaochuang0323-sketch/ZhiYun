/* Stub main.h for unit testing */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

void Error_Handler(void);

/* Pin defines needed by BSP headers */
#define DAC_CS_Pin         GPIO_PIN_4
#define DAC_CS_GPIO_Port   GPIOA
#define DAC_ALMOUT_Pin     GPIO_PIN_2
#define DAC_ALMOUT_GPIO_Port GPIOD
#define DAC_RESET_Pin      GPIO_PIN_3
#define DAC_RESET_GPIO_Port GPIOD
#define DAC_nLDAC_Pin      GPIO_PIN_6
#define DAC_nLDAC_GPIO_Port GPIOD

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */