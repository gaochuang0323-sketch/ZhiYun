#include "dac81416.h"

#include <stddef.h>

static HAL_StatusTypeDef dacLastStatus = HAL_OK;

static void DAC81416_BuildFrame(uint8_t rw, uint8_t addr, uint16_t data, uint8_t frame[3])
{
  frame[0] = (uint8_t)((rw & DAC81416_CMD_READ) | (addr & 0x3FU));
  frame[1] = (uint8_t)(data >> 8);
  frame[2] = (uint8_t)(data & 0xFFU);
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

  dacLastStatus = DAC_PowerDownChannels(0x0000U);
  return dacLastStatus;
}

HAL_StatusTypeDef DAC_WriteRegister(uint8_t cmd, uint8_t addr, uint16_t data)
{
  uint8_t tx[3];
  uint8_t rw = (cmd & DAC81416_CMD_READ) ? DAC81416_CMD_READ : DAC81416_CMD_WRITE;

  DAC81416_BuildFrame(rw, addr, data, tx);
  dacLastStatus = DAC_SPI_Transmit(tx, sizeof(tx));
  return dacLastStatus;
}

uint16_t DAC_ReadRegister(uint8_t cmd, uint8_t addr)
{
  uint8_t tx[3];
  uint8_t rx[3] = {0};
  uint8_t readCmd = (cmd & DAC81416_CMD_READ) ? cmd : DAC81416_CMD_READ;

  DAC81416_BuildFrame(readCmd, addr, 0x0000U, tx);
  dacLastStatus = DAC_SPI_TransmitReceive(tx, rx, sizeof(tx));
  if (dacLastStatus != HAL_OK)
  {
    return 0U;
  }

  DAC81416_BuildFrame(DAC81416_CMD_WRITE, DAC81416_REG_NOP, 0x0000U, tx);
  dacLastStatus = DAC_SPI_TransmitReceive(tx, rx, sizeof(tx));
  if (dacLastStatus != HAL_OK)
  {
    return 0U;
  }

  return (uint16_t)(((uint16_t)rx[1] << 8) | rx[2]);
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
