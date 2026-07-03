#ifndef BSP_CAN_H
#define BSP_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define BSP_CAN_MAX_DATA_LEN 8U

typedef struct
{
  uint32_t id;
  uint32_t tick;
  uint8_t isExtended;
  uint8_t length;
  uint8_t data[BSP_CAN_MAX_DATA_LEN];
} BspCan_RxFrame;

typedef struct
{
  uint8_t started;
  uint32_t txCount;
  uint32_t rxCount;
  uint32_t errorCount;
  uint32_t lastRxId;
  uint32_t lastRxTick;
  uint8_t lastRxIsExtended;
  uint8_t lastRxLength;
  uint8_t lastRxData[BSP_CAN_MAX_DATA_LEN];
} BspCan_Status;

HAL_StatusTypeDef BspCan_Init(void);
HAL_StatusTypeDef BspCan_SendClassic(uint32_t id,
                                     uint8_t isExtended,
                                     const uint8_t *data,
                                     uint8_t length);
void BspCan_Process(void);
BspCan_Status BspCan_GetStatus(void);
void BspCan_OnRxFrame(const BspCan_RxFrame *frame);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CAN_H */
