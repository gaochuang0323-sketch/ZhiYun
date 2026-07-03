#ifndef VOLTAGE_SIM_H
#define VOLTAGE_SIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dac81416.h"

#define VOLTAGE_SIM_CELL_COUNT        13U
#define VOLTAGE_SIM_MIN_MV            500U
#define VOLTAGE_SIM_MAX_MV            5000U
#define VOLTAGE_SIM_DEFAULT_NORMAL_MV 3200U
#define VOLTAGE_SIM_DEFAULT_OV_MV     4500U
#define VOLTAGE_SIM_DEFAULT_UV_MV     1000U
#define VOLTAGE_SIM_CAL_OFFSET_LIMIT_MV 500

typedef enum
{
  VOLTAGE_SIM_FAULT_NONE = 0,
  VOLTAGE_SIM_FAULT_OVER_VOLTAGE,
  VOLTAGE_SIM_FAULT_UNDER_VOLTAGE
} VoltageSim_FaultType;

typedef struct
{
  VoltageSim_FaultType type;
  uint16_t targetMillivolts;
  uint16_t restoreMillivolts;
  uint32_t durationMs;
  uint16_t slewRateMvPerMs;
} VoltageSim_CellFaultInfo;

HAL_StatusTypeDef VoltageSim_Init(uint16_t defaultMillivolts);
HAL_StatusTypeDef VoltageSim_SetCellVoltageMv(uint8_t cell, uint16_t millivolts);
HAL_StatusTypeDef VoltageSim_SetAllCellsVoltageMv(uint16_t millivolts);
HAL_StatusTypeDef VoltageSim_InjectCellOverVoltage(uint8_t cell, uint16_t targetMillivolts);
HAL_StatusTypeDef VoltageSim_InjectCellUnderVoltage(uint8_t cell, uint16_t targetMillivolts);
HAL_StatusTypeDef VoltageSim_InjectCellOverVoltageFor(uint8_t cell,
                                                      uint16_t targetMillivolts,
                                                      uint32_t durationMs);
HAL_StatusTypeDef VoltageSim_InjectCellUnderVoltageFor(uint8_t cell,
                                                       uint16_t targetMillivolts,
                                                       uint32_t durationMs);
HAL_StatusTypeDef VoltageSim_InjectCellOverVoltageRamp(uint8_t cell,
                                                       uint16_t targetMillivolts,
                                                       uint32_t durationMs,
                                                       uint16_t slewRateMvPerMs);
HAL_StatusTypeDef VoltageSim_InjectCellUnderVoltageRamp(uint8_t cell,
                                                        uint16_t targetMillivolts,
                                                        uint32_t durationMs,
                                                        uint16_t slewRateMvPerMs);
HAL_StatusTypeDef VoltageSim_InjectVoltageDifference(uint8_t highCell,
                                                     uint16_t highMillivolts,
                                                     uint8_t lowCell,
                                                     uint16_t lowMillivolts,
                                                     uint32_t durationMs,
                                                     uint16_t slewRateMvPerMs);
HAL_StatusTypeDef VoltageSim_ClearCellFault(uint8_t cell);
HAL_StatusTypeDef VoltageSim_ClearAllFaults(void);
HAL_StatusTypeDef VoltageSim_Process(uint32_t nowMs);
HAL_StatusTypeDef VoltageSim_SetCellCalibrationOffsetMv(uint8_t cell, int16_t offsetMillivolts);
HAL_StatusTypeDef VoltageSim_ClearCellCalibrationOffset(uint8_t cell);
void VoltageSim_ClearAllCalibrationOffsets(void);
int16_t VoltageSim_GetCellCalibrationOffsetMv(uint8_t cell);
uint16_t VoltageSim_MillivoltsToDacCode(uint16_t millivolts);
uint16_t VoltageSim_GetLastCellVoltageMv(uint8_t cell);
VoltageSim_CellFaultInfo VoltageSim_GetCellFaultInfo(uint8_t cell);
uint8_t VoltageSim_GetActiveFaultCount(void);
const char *VoltageSim_GetFaultTypeName(VoltageSim_FaultType type);

#ifdef __cplusplus
}
#endif

#endif /* VOLTAGE_SIM_H */
