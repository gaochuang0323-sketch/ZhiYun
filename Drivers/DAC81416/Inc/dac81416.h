#ifndef DAC81416_H
#define DAC81416_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_dac81416.h"

#define DAC81416_CHANNEL_COUNT 16U

#define DAC81416_CMD_WRITE 0x00U
#define DAC81416_CMD_READ  0x80U

#define DAC81416_REG_NOP         0x00U
#define DAC81416_REG_DEVICEID    0x01U
#define DAC81416_REG_STATUS      0x02U
#define DAC81416_REG_SPICONFIG   0x03U
#define DAC81416_REG_GENCONFIG   0x04U
#define DAC81416_REG_BRDCONFIG   0x05U
#define DAC81416_REG_SYNCCONFIG  0x06U
#define DAC81416_REG_DACPWDWN    0x09U
#define DAC81416_REG_TRIGGER     0x0EU
#define DAC81416_REG_BRDCAST     0x0FU
#define DAC81416_REG_DAC0        0x10U

#define DAC81416_SPICONFIG_RESET_VALUE 0x0AA4U
#define DAC81416_SPICONFIG_ACTIVE_VALUE 0x0A84U
#define DAC81416_SPICONFIG_DEV_PWDWN   (1U << 5)
#define DAC81416_SPICONFIG_SDO_EN      (1U << 2)

#define DAC81416_GENCONFIG_REF_PWDWN   (1U << 14)

#define DAC81416_TRIGGER_LDAC          (1U << 4)
#define DAC81416_TRIGGER_SOFT_RESET    0x1010U

HAL_StatusTypeDef DAC81416_Init(void);
HAL_StatusTypeDef DAC_WriteRegister(uint8_t cmd, uint8_t addr, uint16_t data);
uint16_t DAC_ReadRegister(uint8_t cmd, uint8_t addr);
HAL_StatusTypeDef DAC_SoftwareReset(void);
HAL_StatusTypeDef DAC_WriteChannel(uint8_t channel, uint16_t value);
HAL_StatusTypeDef DAC_WriteAllChannels(uint16_t *values);
HAL_StatusTypeDef DAC_EnableInternalRef(uint8_t enable);
HAL_StatusTypeDef DAC_PowerDownChannels(uint16_t powerDownMask);
HAL_StatusTypeDef DAC_TriggerLDAC(void);
HAL_StatusTypeDef DAC_GetLastStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* DAC81416_H */
