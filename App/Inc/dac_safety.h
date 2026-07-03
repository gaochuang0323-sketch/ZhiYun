#ifndef DAC_SAFETY_H
#define DAC_SAFETY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "voltage_sim.h"

#define DAC_SAFETY_DEFAULT_SAFE_MV VOLTAGE_SIM_DEFAULT_NORMAL_MV

typedef struct
{
  uint8_t emergencyActive;
  uint8_t alarmPinState;
  uint8_t alarmActive;
  uint8_t alarmLatched;
  uint32_t alarmCount;
  uint32_t lastAlarmTick;
  uint16_t safeMillivolts;
} DacSafety_Status;

void DacSafety_Init(uint16_t safeMillivolts);
void DacSafety_Process(uint32_t nowMs);
HAL_StatusTypeDef DacSafety_EmergencyStop(uint16_t safeMillivolts);
void DacSafety_Release(void);
uint8_t DacSafety_IsOutputAllowed(void);
DacSafety_Status DacSafety_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* DAC_SAFETY_H */
