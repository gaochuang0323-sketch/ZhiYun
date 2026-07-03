#ifndef BSP_DAC81416_H
#define BSP_DAC81416_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "spi.h"

#define DAC_SPI_DEFAULT_TIMEOUT_MS 100U

#define DAC_CS_LOW()  HAL_GPIO_WritePin(DAC_CS_GPIO_Port, DAC_CS_Pin, GPIO_PIN_RESET)
#define DAC_CS_HIGH() HAL_GPIO_WritePin(DAC_CS_GPIO_Port, DAC_CS_Pin, GPIO_PIN_SET)

void BSP_DAC81416_Init(void);
void BSP_DAC81416_SetTimeout(uint32_t timeoutMs);
uint32_t BSP_DAC81416_GetTimeout(void);

HAL_StatusTypeDef DAC_SPI_Transmit(uint8_t *txData, uint16_t size);
HAL_StatusTypeDef DAC_SPI_TransmitReceive(uint8_t *txData, uint8_t *rxData, uint16_t size);

/* CS is held low for all frames in the call. With SPI_DATASIZE_24BIT,
 * frameCount=2 produces the 48 SCLK transaction required by DAC81416 reads.
 */
HAL_StatusTypeDef DAC_SPI_TransmitFrames24(const uint32_t *txFrames, uint16_t frameCount);
HAL_StatusTypeDef DAC_SPI_TransmitReceiveFrames24(const uint32_t *txFrames,
                                                  uint32_t *rxFrames,
                                                  uint16_t frameCount);

void DAC_SetResetPin(GPIO_PinState state);
void DAC_ResetHardware(void);
void DAC_SetLDACPin(GPIO_PinState state);
void DAC_LDAC_Update(void);
uint8_t DAC_ReadAlarmPin(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_DAC81416_H */
