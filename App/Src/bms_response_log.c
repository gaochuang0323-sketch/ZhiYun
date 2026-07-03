#include "bms_response_log.h"

#include <string.h>

static BmsResponseLog_Status responseLog;

void BmsResponseLog_RecordFaultTrigger(BmsResponseLog_TriggerType type,
                                       uint8_t primaryCell,
                                       uint16_t primaryMillivolts,
                                       uint8_t secondaryCell,
                                       uint16_t secondaryMillivolts)
{
  memset(&responseLog, 0, sizeof(responseLog));

  responseLog.armed = 1U;
  responseLog.triggerType = type;
  responseLog.primaryCell = primaryCell;
  responseLog.secondaryCell = secondaryCell;
  responseLog.primaryMillivolts = primaryMillivolts;
  responseLog.secondaryMillivolts = secondaryMillivolts;
  responseLog.triggerTick = HAL_GetTick();
}

void BmsResponseLog_Clear(void)
{
  memset(&responseLog, 0, sizeof(responseLog));
}

BmsResponseLog_Status BmsResponseLog_GetStatus(void)
{
  return responseLog;
}

const char *BmsResponseLog_GetTriggerName(BmsResponseLog_TriggerType type)
{
  switch (type)
  {
    case BMS_RESPONSE_LOG_TRIGGER_OV:
      return "OV";

    case BMS_RESPONSE_LOG_TRIGGER_UV:
      return "UV";

    case BMS_RESPONSE_LOG_TRIGGER_DIFF:
      return "DIFF";

    case BMS_RESPONSE_LOG_TRIGGER_NONE:
    default:
      return "NONE";
  }
}

void BspCan_OnRxFrame(const BspCan_RxFrame *frame)
{
  if ((frame == NULL) || (responseLog.armed == 0U))
  {
    return;
  }

  responseLog.canFramesAfterTrigger++;

  if (responseLog.responseCaptured != 0U)
  {
    return;
  }

  responseLog.responseCaptured = 1U;
  responseLog.responseTick = frame->tick;
  responseLog.responseDelayMs = frame->tick - responseLog.triggerTick;
  responseLog.responseCanId = frame->id;
  responseLog.responseCanIsExtended = frame->isExtended;
  responseLog.responseCanLength = frame->length;
  memcpy(responseLog.responseCanData, frame->data, frame->length);
}
