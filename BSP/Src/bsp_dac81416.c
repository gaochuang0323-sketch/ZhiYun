#include "bsp_dac81416.h"

#include <stddef.h>

#include "cmsis_os2.h"

static osMutexId_t dacSpiMutex;
static uint32_t dacSpiTimeoutMs = DAC_SPI_DEFAULT_TIMEOUT_MS;

static void DAC_SPI_EnsureMutex(void)
{
  if (dacSpiMutex == NULL)
  {
    int32_t lockState = osKernelLock();

    if (dacSpiMutex == NULL)
    {
      dacSpiMutex = osMutexNew(NULL);
      __DSB();  /* Ensure mutex object is fully committed to memory */
    }

    (void)osKernelRestoreLock(lockState);
  }
}

static HAL_StatusTypeDef DAC_SPI_Lock(void)
{
  if (osKernelGetState() == osKernelRunning)
  {
    DAC_SPI_EnsureMutex();

    if ((dacSpiMutex != NULL) && (osMutexAcquire(dacSpiMutex, dacSpiTimeoutMs) == osOK))
    {
      return HAL_OK;
    }

    return HAL_TIMEOUT;
  }

  return HAL_OK;
}

static void DAC_SPI_Unlock(void)
{
  if ((osKernelGetState() == osKernelRunning) && (dacSpiMutex != NULL))
  {
    (void)osMutexRelease(dacSpiMutex);
  }
}

void BSP_DAC81416_Init(void)
{
  DAC_CS_HIGH();
  DAC_SetResetPin(GPIO_PIN_SET);
  DAC_SetLDACPin(GPIO_PIN_SET);
}

void BSP_DAC81416_SetTimeout(uint32_t timeoutMs)
{
  dacSpiTimeoutMs = timeoutMs;
}

uint32_t BSP_DAC81416_GetTimeout(void)
{
  return dacSpiTimeoutMs;
}

HAL_StatusTypeDef DAC_SPI_Transmit(uint8_t *txData, uint16_t size)
{
  uint32_t frame;

  if ((txData == NULL) || (size != 3U))
  {
    return HAL_ERROR;
  }

  frame = ((uint32_t)txData[0] << 16) |
          ((uint32_t)txData[1] << 8) |
          (uint32_t)txData[2];

  return DAC_SPI_TransmitFrames24(&frame, 1U);
}

HAL_StatusTypeDef DAC_SPI_TransmitReceive(uint8_t *txData, uint8_t *rxData, uint16_t size)
{
  HAL_StatusTypeDef status;
  uint32_t txFrame;
  uint32_t rxFrame = 0U;

  if ((txData == NULL) || (rxData == NULL) || (size != 3U))
  {
    return HAL_ERROR;
  }

  txFrame = ((uint32_t)txData[0] << 16) |
            ((uint32_t)txData[1] << 8) |
            (uint32_t)txData[2];

  status = DAC_SPI_TransmitReceiveFrames24(&txFrame, &rxFrame, 1U);
  if (status == HAL_OK)
  {
    rxData[0] = (uint8_t)(rxFrame >> 16);
    rxData[1] = (uint8_t)(rxFrame >> 8);
    rxData[2] = (uint8_t)rxFrame;
  }

  return status;
}

HAL_StatusTypeDef DAC_SPI_TransmitFrames24(const uint32_t *txFrames, uint16_t frameCount)
{
  HAL_StatusTypeDef status;

  if ((txFrames == NULL) || (frameCount == 0U))
  {
    return HAL_ERROR;
  }

  status = DAC_SPI_Lock();
  if (status != HAL_OK)
  {
    return status;
  }

  DAC_CS_LOW();
  status = HAL_SPI_Transmit(&hspi1, (const uint8_t *)txFrames, frameCount, dacSpiTimeoutMs);
  DAC_CS_HIGH();
  DAC_SPI_Unlock();

  return status;
}

HAL_StatusTypeDef DAC_SPI_TransmitReceiveFrames24(const uint32_t *txFrames,
                                                  uint32_t *rxFrames,
                                                  uint16_t frameCount)
{
  HAL_StatusTypeDef status;

  if ((txFrames == NULL) || (rxFrames == NULL) || (frameCount == 0U))
  {
    return HAL_ERROR;
  }

  status = DAC_SPI_Lock();
  if (status != HAL_OK)
  {
    return status;
  }

  DAC_CS_LOW();
  status = HAL_SPI_TransmitReceive(&hspi1,
                                   (const uint8_t *)txFrames,
                                   (uint8_t *)rxFrames,
                                   frameCount,
                                   dacSpiTimeoutMs);
  DAC_CS_HIGH();
  DAC_SPI_Unlock();

  return status;
}

void DAC_SetResetPin(GPIO_PinState state)
{
  HAL_GPIO_WritePin(DAC_RESET_GPIO_Port, DAC_RESET_Pin, state);
}

void DAC_ResetHardware(void)
{
  DAC_SetResetPin(GPIO_PIN_RESET);
  HAL_Delay(10U);
  DAC_SetResetPin(GPIO_PIN_SET);
  HAL_Delay(10U);
}

void DAC_SetLDACPin(GPIO_PinState state)
{
  HAL_GPIO_WritePin(DAC_nLDAC_GPIO_Port, DAC_nLDAC_Pin, state);
}

void DAC_LDAC_Update(void)
{
  DAC_SetLDACPin(GPIO_PIN_RESET);
  HAL_Delay(1U);
  DAC_SetLDACPin(GPIO_PIN_SET);
}

uint8_t DAC_ReadAlarmPin(void)
{
  return (HAL_GPIO_ReadPin(DAC_ALMOUT_GPIO_Port, DAC_ALMOUT_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}
