/* Helper function declarations for test stubs */
#ifndef STUBS_H
#define STUBS_H

#include <stdint.h>
#include "stm32h7xx_hal.h"
#include "fdcan.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tick manipulation */
void HAL_SetTick(uint32_t tick);

/* DAC write result control */
void DAC_SetWriteResult(HAL_StatusTypeDef result);
uint8_t DAC_GetWriteChannelCalled(void);
void DAC_ClearWriteChannelCalled(void);
uint8_t DAC_GetLastChannel(void);
uint16_t DAC_GetLastValue(void);
uint8_t DAC_GetLdacCalled(void);
void DAC_ClearLdacCalled(void);

/* Alarm pin simulation */
void DAC_SetAlarmPinValue(uint8_t value);

<<<<<<< HEAD
/* UART test buffer control */
void UART_PushData(const uint8_t *data, uint16_t length);
void UART_Reset(void);
void UART_SetReceiveResult(HAL_StatusTypeDef result);

=======
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
/* FDCAN test helpers */
void FDCAN_SetConfigFilterResult(HAL_StatusTypeDef result);
void FDCAN_SetConfigGlobalResult(HAL_StatusTypeDef result);
void FDCAN_SetStartResult(HAL_StatusTypeDef result);
void FDCAN_SetAddMessageResult(HAL_StatusTypeDef result);
void FDCAN_SetRxFifoFillLevel(uint32_t level);
void FDCAN_SetGetRxMessageResult(HAL_StatusTypeDef result);
void FDCAN_SetRxData(uint32_t id, uint32_t idType, uint32_t dlc, const uint8_t *data);
void FDCAN_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* STUBS_H */