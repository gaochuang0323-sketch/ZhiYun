/* Stub DAC81416 driver header for unit testing */
#ifndef DAC81416_H
#define DAC81416_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32h7xx_hal.h"

#define DAC81416_CHANNEL_COUNT 16U

HAL_StatusTypeDef DAC_WriteChannel(uint8_t channel, uint16_t value);
void DAC_LDAC_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* DAC81416_H */