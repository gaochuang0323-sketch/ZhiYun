#ifndef BMS_RESPONSE_LOG_H
#define BMS_RESPONSE_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_can.h"
#include "main.h"

typedef enum
{
  BMS_RESPONSE_LOG_TRIGGER_NONE = 0,
  BMS_RESPONSE_LOG_TRIGGER_OV,
  BMS_RESPONSE_LOG_TRIGGER_UV,
  BMS_RESPONSE_LOG_TRIGGER_DIFF
} BmsResponseLog_TriggerType;

typedef struct
{
  uint8_t armed;
  BmsResponseLog_TriggerType triggerType;
  uint8_t primaryCell;
  uint8_t secondaryCell;
  uint16_t primaryMillivolts;
  uint16_t secondaryMillivolts;
  uint32_t triggerTick;
  uint32_t canFramesAfterTrigger;
  uint8_t responseCaptured;
  uint32_t responseTick;
  uint32_t responseDelayMs;
  uint32_t responseCanId;
  uint8_t responseCanIsExtended;
  uint8_t responseCanLength;
  uint8_t responseCanData[BSP_CAN_MAX_DATA_LEN];
} BmsResponseLog_Status;

void BmsResponseLog_RecordFaultTrigger(BmsResponseLog_TriggerType type,
                                       uint8_t primaryCell,
                                       uint16_t primaryMillivolts,
                                       uint8_t secondaryCell,
                                       uint16_t secondaryMillivolts);
void BmsResponseLog_Clear(void);
BmsResponseLog_Status BmsResponseLog_GetStatus(void);
const char *BmsResponseLog_GetTriggerName(BmsResponseLog_TriggerType type);

#ifdef __cplusplus
}
#endif

#endif /* BMS_RESPONSE_LOG_H */
