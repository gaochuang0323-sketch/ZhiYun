#ifndef BSP_SRAM_H
#define BSP_SRAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define BSP_SRAM_BASE_ADDRESS        0x60000000UL
#define BSP_SRAM_SIZE_BYTES          (1024UL * 1024UL)
#define BSP_SRAM_DEFAULT_TEST_BYTES  (64UL * 1024UL)
#define BSP_SRAM_BUS_WIDTH_BITS      16U

typedef struct
{
  uint32_t baseAddress;
  uint32_t sizeBytes;
  uint8_t busWidthBits;
  uint32_t lastTestBytes;
  uint32_t lastTestTick;
  uint32_t lastFailedAddress;
  uint32_t passCount;
  uint32_t failCount;
  uint16_t expected;
  uint16_t actual;
  HAL_StatusTypeDef lastResult;
} BspSram_Status;

void BspSram_Init(void);
HAL_StatusTypeDef BspSram_RunSelfTest(uint32_t bytes);
BspSram_Status BspSram_GetStatus(void);
const char *BspSram_GetResultName(HAL_StatusTypeDef result);

#ifdef __cplusplus
}
#endif

#endif /* BSP_SRAM_H */
