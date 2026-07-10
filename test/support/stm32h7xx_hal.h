/* Stub STM32H7xx HAL header for unit testing */
#ifndef __STM32H7XX_HAL_H
#define __STM32H7XX_HAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HAL Status definitions */
typedef enum
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

typedef enum
{
  GPIO_PIN_RESET = 0U,
  GPIO_PIN_SET   = 1U
} GPIO_PinState;

/* HAL Tick */
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

/* GPIO */
void HAL_GPIO_WritePin(uint16_t GPIO_Pin, uint16_t GPIO_Port, GPIO_PinState PinState);
GPIO_PinState HAL_GPIO_ReadPin(uint16_t GPIO_Pin, uint16_t GPIO_Port);

/* SPI */
typedef struct { uint32_t dummy; } SPI_HandleTypeDef;

/* UART */
typedef struct { uint32_t dummy; } UART_HandleTypeDef;
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);

#ifdef __cplusplus
}
#endif

#endif /* __STM32H7XX_HAL_H */