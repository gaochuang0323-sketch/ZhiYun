#include "dac81416.h"

#include <stddef.h>

#define DAC81416_READ_TRANSACTION_FRAMES 2U

static HAL_StatusTypeDef dacLastStatus = HAL_OK;

static uint8_t DAC81416_IsValidRange(uint8_t range)
{
  switch (range)
  {
    case DAC81416_RANGE_0V_TO_5V:
    case DAC81416_RANGE_0V_TO_10V:
    case DAC81416_RANGE_0V_TO_20V:
    case DAC81416_RANGE_0V_TO_40V:
    case DAC81416_RANGE_NEG_5V_TO_5V:
    case DAC81416_RANGE_NEG_10V_TO_10V:
    case DAC81416_RANGE_NEG_20V_TO_20V:
    case DAC81416_RANGE_NEG_2V5_TO_2V5:
      return 1U;

    default:
      return 0U;
  }
}

static uint16_t DAC81416_BuildRangeRegister(uint8_t range)
{
  uint16_t value = (uint16_t)(range & 0x0FU);

  return (uint16_t)(value | (value << 4) | (value << 8) | (value << 12));
}

static uint32_t DAC81416_BuildFrame(uint8_t rw, uint8_t addr, uint16_t data)
{
  uint32_t command = (uint32_t)((rw & DAC81416_CMD_READ) | (addr & 0x3FU));

  return (uint32_t)((command << 16) | data);
}

HAL_StatusTypeDef DAC81416_Init(void)
{
  BSP_DAC81416_Init();
  DAC_ResetHardware();

  dacLastStatus = DAC_WriteRegister(DAC81416_CMD_WRITE,
                                    DAC81416_REG_SPICONFIG,
                                    DAC81416_SPICONFIG_ACTIVE_VALUE);
  if (dacLastStatus != HAL_OK)
  {
    return dacLastStatus;
  }

  dacLastStatus = DAC_EnableInternalRef(1U);
  if (dacLastStatus != HAL_OK)
  {
    return dacLastStatus;
  }

  HAL_Delay(1U);

  dacLastStatus = DAC_ConfigAllChannels0To5V();
  if (dacLastStatus != HAL_OK)
  {
    return dacLastStatus;
  }

  dacLastStatus = DAC_PowerDownChannels(0x0000U);
  return dacLastStatus;
}

HAL_StatusTypeDef DAC_WriteRegister(uint8_t cmd, uint8_t addr, uint16_t data)
{
  uint32_t tx;
  uint8_t rw = (cmd & DAC81416_CMD_READ) ? DAC81416_CMD_READ : DAC81416_CMD_WRITE;

  tx = DAC81416_BuildFrame(rw, addr, data);
  dacLastStatus = DAC_SPI_TransmitFrames24(&tx, 1U);
  return dacLastStatus;
}

uint16_t DAC_ReadRegister(uint8_t cmd, uint8_t addr)
{
  uint32_t tx[DAC81416_READ_TRANSACTION_FRAMES];
  uint32_t rx[DAC81416_READ_TRANSACTION_FRAMES] = {0U, 0U};
  uint8_t readCmd = (cmd & DAC81416_CMD_READ) ? cmd : DAC81416_CMD_READ;

  tx[0] = DAC81416_BuildFrame(readCmd, addr, 0x0000U);
  tx[1] = DAC81416_BuildFrame(DAC81416_CMD_WRITE, DAC81416_REG_NOP, 0x0000U);
  dacLastStatus = DAC_SPI_TransmitReceiveFrames24(tx, rx, DAC81416_READ_TRANSACTION_FRAMES);
  if (dacLastStatus != HAL_OK)
  {
    return 0U;
  }

  return (uint16_t)(rx[1] & 0xFFFFU);
}

HAL_StatusTypeDef DAC_SoftwareReset(void)
{
  dacLastStatus = DAC_WriteRegister(DAC81416_CMD_WRITE,
                                    DAC81416_REG_TRIGGER,
                                    DAC81416_TRIGGER_SOFT_RESET);
  HAL_Delay(1U);
  return dacLastStatus;
}

HAL_StatusTypeDef DAC_WriteChannel(uint8_t channel, uint16_t value)
{
  if (channel >= DAC81416_CHANNEL_COUNT)
  {
    dacLastStatus = HAL_ERROR;
    return dacLastStatus;
  }

  dacLastStatus = DAC_WriteRegister(DAC81416_CMD_WRITE,
                                    (uint8_t)(DAC81416_REG_DAC0 + channel),
                                    value);
  return dacLastStatus;
}

HAL_StatusTypeDef DAC_WriteAllChannels(uint16_t *values)
{
  HAL_StatusTypeDef status = HAL_OK;

  if (values == NULL)
  {
    dacLastStatus = HAL_ERROR;
    return dacLastStatus;
  }

  for (uint8_t channel = 0U; channel < DAC81416_CHANNEL_COUNT; channel++)
  {
    status = DAC_WriteChannel(channel, values[channel]);
    if (status != HAL_OK)
    {
      dacLastStatus = status;
      return status;
    }
  }

  DAC_LDAC_Update();
  dacLastStatus = HAL_OK;
  return dacLastStatus;
}

HAL_StatusTypeDef DAC_EnableInternalRef(uint8_t enable)
{
  uint16_t reg = DAC_ReadRegister(DAC81416_CMD_READ, DAC81416_REG_GENCONFIG);

  if (dacLastStatus != HAL_OK)
  {
    return dacLastStatus;
  }

  if (enable != 0U)
  {
    reg &= (uint16_t)~DAC81416_GENCONFIG_REF_PWDWN;
  }
  else
  {
    reg |= DAC81416_GENCONFIG_REF_PWDWN;
  }

  dacLastStatus = DAC_WriteRegister(DAC81416_CMD_WRITE, DAC81416_REG_GENCONFIG, reg);
  return dacLastStatus;
}

HAL_StatusTypeDef DAC_SetAllChannelsRange(uint8_t range)
{
  uint16_t rangeValue;

  if (DAC81416_IsValidRange(range) == 0U)
  {
    dacLastStatus = HAL_ERROR;
    return dacLastStatus;
  }

  rangeValue = DAC81416_BuildRangeRegister(range);

  for (uint8_t index = 0U; index < 4U; index++)
  {
    dacLastStatus = DAC_WriteRegister(DAC81416_CMD_WRITE,
                                      (uint8_t)(DAC81416_REG_DACRANGE0 + index),
                                      rangeValue);
    if (dacLastStatus != HAL_OK)
    {
      return dacLastStatus;
    }
  }

  return dacLastStatus;
}

HAL_StatusTypeDef DAC_ConfigAllChannels0To5V(void)
{
  return DAC_SetAllChannelsRange(DAC81416_RANGE_0V_TO_5V);
}

HAL_StatusTypeDef DAC_PowerDownChannels(uint16_t powerDownMask)
{
  dacLastStatus = DAC_WriteRegister(DAC81416_CMD_WRITE,
                                    DAC81416_REG_DACPWDWN,
                                    powerDownMask);
  return dacLastStatus;
}

HAL_StatusTypeDef DAC_TriggerLDAC(void)
{
  dacLastStatus = DAC_WriteRegister(DAC81416_CMD_WRITE,
                                    DAC81416_REG_TRIGGER,
                                    DAC81416_TRIGGER_LDAC);
  return dacLastStatus;
}

HAL_StatusTypeDef DAC_GetLastStatus(void)
{
  return dacLastStatus;
}
