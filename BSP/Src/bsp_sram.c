#include "bsp_sram.h"

#include <string.h>

static BspSram_Status sramStatus;

static void BspSram_RecordFailure(uint32_t index, uint16_t expected, uint16_t actual)
{
  sramStatus.lastFailedAddress = BSP_SRAM_BASE_ADDRESS + (index * sizeof(uint16_t));
  sramStatus.expected = expected;
  sramStatus.actual = actual;
  sramStatus.lastResult = HAL_ERROR;
  sramStatus.failCount++;
}

static HAL_StatusTypeDef BspSram_WriteAndVerifyPattern(uint32_t halfwordCount, uint16_t pattern)
{
  volatile uint16_t *memory = (volatile uint16_t *)BSP_SRAM_BASE_ADDRESS;

  for (uint32_t index = 0U; index < halfwordCount; index++)
  {
    memory[index] = pattern;
  }
  __DSB();

  for (uint32_t index = 0U; index < halfwordCount; index++)
  {
    uint16_t actual = memory[index];

    if (actual != pattern)
    {
      BspSram_RecordFailure(index, pattern, actual);
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

static HAL_StatusTypeDef BspSram_WriteAndVerifyAddressPattern(uint32_t halfwordCount)
{
  volatile uint16_t *memory = (volatile uint16_t *)BSP_SRAM_BASE_ADDRESS;

  for (uint32_t index = 0U; index < halfwordCount; index++)
  {
    memory[index] = (uint16_t)(0xA5A5U ^ index);
  }
  __DSB();

  for (uint32_t index = 0U; index < halfwordCount; index++)
  {
    uint16_t expected = (uint16_t)(0xA5A5U ^ index);
    uint16_t actual = memory[index];

    if (actual != expected)
    {
      BspSram_RecordFailure(index, expected, actual);
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

void BspSram_Init(void)
{
  memset(&sramStatus, 0, sizeof(sramStatus));
  sramStatus.baseAddress = BSP_SRAM_BASE_ADDRESS;
  sramStatus.sizeBytes = BSP_SRAM_SIZE_BYTES;
  sramStatus.busWidthBits = BSP_SRAM_BUS_WIDTH_BITS;
  sramStatus.lastResult = HAL_OK;
}

HAL_StatusTypeDef BspSram_RunSelfTest(uint32_t bytes)
{
  static const uint16_t patterns[] = {0x0000U, 0xFFFFU, 0xAAAAU, 0x5555U};
  uint32_t halfwordCount;

  if (bytes == 0U)
  {
    bytes = BSP_SRAM_DEFAULT_TEST_BYTES;
  }

  if (bytes > BSP_SRAM_SIZE_BYTES)
  {
    bytes = BSP_SRAM_SIZE_BYTES;
  }

  bytes &= ~1UL;
  if (bytes == 0U)
  {
    return HAL_ERROR;
  }

  sramStatus.lastTestBytes = bytes;
  sramStatus.lastTestTick = HAL_GetTick();
  sramStatus.lastFailedAddress = 0U;
  sramStatus.expected = 0U;
  sramStatus.actual = 0U;
  sramStatus.lastResult = HAL_BUSY;

  halfwordCount = bytes / sizeof(uint16_t);

  if (BspSram_WriteAndVerifyAddressPattern(halfwordCount) != HAL_OK)
  {
    return HAL_ERROR;
  }

  for (uint32_t index = 0U; index < (sizeof(patterns) / sizeof(patterns[0])); index++)
  {
    if (BspSram_WriteAndVerifyPattern(halfwordCount, patterns[index]) != HAL_OK)
    {
      return HAL_ERROR;
    }
  }

  sramStatus.lastResult = HAL_OK;
  sramStatus.passCount++;
  return HAL_OK;
}

BspSram_Status BspSram_GetStatus(void)
{
  return sramStatus;
}

const char *BspSram_GetResultName(HAL_StatusTypeDef result)
{
  switch (result)
  {
    case HAL_OK:
      return "OK";
    case HAL_ERROR:
      return "ERR";
    case HAL_BUSY:
      return "BUSY";
    case HAL_TIMEOUT:
      return "TIMEOUT";
    default:
      return "UNKNOWN";
  }
}
