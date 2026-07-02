#include "voltage_sim.h"

static uint16_t cellVoltageMv[VOLTAGE_SIM_CELL_COUNT];
static VoltageSim_CellFaultInfo cellFaultInfo[VOLTAGE_SIM_CELL_COUNT];
static uint32_t faultLastUpdateMs[VOLTAGE_SIM_CELL_COUNT];
static uint32_t faultHoldStartedMs[VOLTAGE_SIM_CELL_COUNT];
static uint8_t faultRestoring[VOLTAGE_SIM_CELL_COUNT];

static uint8_t VoltageSim_IsValidCell(uint8_t cell)
{
  return ((cell >= 1U) && (cell <= VOLTAGE_SIM_CELL_COUNT)) ? 1U : 0U;
}

static uint8_t VoltageSim_IsValidVoltage(uint16_t millivolts)
{
  return ((millivolts >= VOLTAGE_SIM_MIN_MV) && (millivolts <= VOLTAGE_SIM_MAX_MV)) ? 1U : 0U;
}

static void VoltageSim_ClearFaultRecord(uint8_t cell)
{
  VoltageSim_CellFaultInfo *info = &cellFaultInfo[cell - 1U];

  info->type = VOLTAGE_SIM_FAULT_NONE;
  info->targetMillivolts = 0U;
  info->restoreMillivolts = 0U;
  info->durationMs = 0U;
  info->slewRateMvPerMs = 0U;
  faultLastUpdateMs[cell - 1U] = 0U;
  faultHoldStartedMs[cell - 1U] = 0U;
  faultRestoring[cell - 1U] = 0U;
}

static HAL_StatusTypeDef VoltageSim_WriteCellVoltageMv(uint8_t cell,
                                                       uint16_t millivolts,
                                                       uint8_t updateOutput)
{
  HAL_StatusTypeDef status;
  uint16_t dacCode;

  if ((VoltageSim_IsValidCell(cell) == 0U) || (VoltageSim_IsValidVoltage(millivolts) == 0U))
  {
    return HAL_ERROR;
  }

  dacCode = VoltageSim_MillivoltsToDacCode(millivolts);
  status = DAC_WriteChannel((uint8_t)(cell - 1U), dacCode);
  if (status != HAL_OK)
  {
    return status;
  }

  cellVoltageMv[cell - 1U] = millivolts;

  if (updateOutput != 0U)
  {
    DAC_LDAC_Update();
  }

  return HAL_OK;
}

static HAL_StatusTypeDef VoltageSim_InjectCellFault(uint8_t cell,
                                                    uint16_t targetMillivolts,
                                                    VoltageSim_FaultType type,
                                                    uint32_t durationMs,
                                                    uint16_t slewRateMvPerMs)
{
  HAL_StatusTypeDef status;
  uint16_t restoreMillivolts;
  uint32_t nowMs;
  VoltageSim_CellFaultInfo *info;

  if ((VoltageSim_IsValidCell(cell) == 0U) || (VoltageSim_IsValidVoltage(targetMillivolts) == 0U))
  {
    return HAL_ERROR;
  }

  info = &cellFaultInfo[cell - 1U];
  restoreMillivolts = (info->type == VOLTAGE_SIM_FAULT_NONE) ?
                      cellVoltageMv[cell - 1U] :
                      info->restoreMillivolts;

  if (((type == VOLTAGE_SIM_FAULT_OVER_VOLTAGE) && (targetMillivolts <= restoreMillivolts)) ||
      ((type == VOLTAGE_SIM_FAULT_UNDER_VOLTAGE) && (targetMillivolts >= restoreMillivolts)))
  {
    return HAL_ERROR;
  }

  nowMs = HAL_GetTick();

  info->type = type;
  info->targetMillivolts = targetMillivolts;
  info->restoreMillivolts = restoreMillivolts;
  info->durationMs = durationMs;
  info->slewRateMvPerMs = slewRateMvPerMs;
  faultLastUpdateMs[cell - 1U] = nowMs;
  faultHoldStartedMs[cell - 1U] = 0U;
  faultRestoring[cell - 1U] = 0U;

  if (slewRateMvPerMs == 0U)
  {
    status = VoltageSim_WriteCellVoltageMv(cell, targetMillivolts, 1U);
    if (status != HAL_OK)
    {
      VoltageSim_ClearFaultRecord(cell);
      return status;
    }

    faultHoldStartedMs[cell - 1U] = nowMs;
  }

  return HAL_OK;
}

static uint16_t VoltageSim_CalculateNextVoltage(uint16_t currentMv,
                                                uint16_t targetMv,
                                                uint32_t maxStepMv)
{
  uint32_t delta;

  if (currentMv == targetMv)
  {
    return currentMv;
  }

  delta = (currentMv > targetMv) ? ((uint32_t)currentMv - targetMv) :
                                  ((uint32_t)targetMv - currentMv);

  if ((maxStepMv == 0U) || (maxStepMv >= delta))
  {
    return targetMv;
  }

  if (currentMv < targetMv)
  {
    return (uint16_t)(currentMv + maxStepMv);
  }

  return (uint16_t)(currentMv - maxStepMv);
}

HAL_StatusTypeDef VoltageSim_Init(uint16_t defaultMillivolts)
{
  return VoltageSim_SetAllCellsVoltageMv(defaultMillivolts);
}

HAL_StatusTypeDef VoltageSim_SetCellVoltageMv(uint8_t cell, uint16_t millivolts)
{
  HAL_StatusTypeDef status = VoltageSim_WriteCellVoltageMv(cell, millivolts, 1U);

  if (status == HAL_OK)
  {
    VoltageSim_ClearFaultRecord(cell);
  }

  return status;
}

HAL_StatusTypeDef VoltageSim_SetAllCellsVoltageMv(uint16_t millivolts)
{
  HAL_StatusTypeDef status;

  if (VoltageSim_IsValidVoltage(millivolts) == 0U)
  {
    return HAL_ERROR;
  }

  for (uint8_t cell = 1U; cell <= VOLTAGE_SIM_CELL_COUNT; cell++)
  {
    status = VoltageSim_WriteCellVoltageMv(cell, millivolts, 0U);
    if (status != HAL_OK)
    {
      return status;
    }

    VoltageSim_ClearFaultRecord(cell);
  }

  DAC_LDAC_Update();
  return HAL_OK;
}

HAL_StatusTypeDef VoltageSim_InjectCellOverVoltage(uint8_t cell, uint16_t targetMillivolts)
{
  return VoltageSim_InjectCellFault(cell,
                                    targetMillivolts,
                                    VOLTAGE_SIM_FAULT_OVER_VOLTAGE,
                                    0U,
                                    0U);
}

HAL_StatusTypeDef VoltageSim_InjectCellUnderVoltage(uint8_t cell, uint16_t targetMillivolts)
{
  return VoltageSim_InjectCellFault(cell,
                                    targetMillivolts,
                                    VOLTAGE_SIM_FAULT_UNDER_VOLTAGE,
                                    0U,
                                    0U);
}

HAL_StatusTypeDef VoltageSim_InjectCellOverVoltageFor(uint8_t cell,
                                                      uint16_t targetMillivolts,
                                                      uint32_t durationMs)
{
  return VoltageSim_InjectCellFault(cell,
                                    targetMillivolts,
                                    VOLTAGE_SIM_FAULT_OVER_VOLTAGE,
                                    durationMs,
                                    0U);
}

HAL_StatusTypeDef VoltageSim_InjectCellUnderVoltageFor(uint8_t cell,
                                                       uint16_t targetMillivolts,
                                                       uint32_t durationMs)
{
  return VoltageSim_InjectCellFault(cell,
                                    targetMillivolts,
                                    VOLTAGE_SIM_FAULT_UNDER_VOLTAGE,
                                    durationMs,
                                    0U);
}

HAL_StatusTypeDef VoltageSim_InjectCellOverVoltageRamp(uint8_t cell,
                                                       uint16_t targetMillivolts,
                                                       uint32_t durationMs,
                                                       uint16_t slewRateMvPerMs)
{
  return VoltageSim_InjectCellFault(cell,
                                    targetMillivolts,
                                    VOLTAGE_SIM_FAULT_OVER_VOLTAGE,
                                    durationMs,
                                    slewRateMvPerMs);
}

HAL_StatusTypeDef VoltageSim_InjectCellUnderVoltageRamp(uint8_t cell,
                                                        uint16_t targetMillivolts,
                                                        uint32_t durationMs,
                                                        uint16_t slewRateMvPerMs)
{
  return VoltageSim_InjectCellFault(cell,
                                    targetMillivolts,
                                    VOLTAGE_SIM_FAULT_UNDER_VOLTAGE,
                                    durationMs,
                                    slewRateMvPerMs);
}

HAL_StatusTypeDef VoltageSim_InjectVoltageDifference(uint8_t highCell,
                                                     uint16_t highMillivolts,
                                                     uint8_t lowCell,
                                                     uint16_t lowMillivolts,
                                                     uint32_t durationMs,
                                                     uint16_t slewRateMvPerMs)
{
  HAL_StatusTypeDef status;

  if ((highCell == lowCell) || (highMillivolts <= lowMillivolts))
  {
    return HAL_ERROR;
  }

  status = VoltageSim_InjectCellOverVoltageRamp(highCell,
                                                highMillivolts,
                                                durationMs,
                                                slewRateMvPerMs);
  if (status != HAL_OK)
  {
    return status;
  }

  status = VoltageSim_InjectCellUnderVoltageRamp(lowCell,
                                                 lowMillivolts,
                                                 durationMs,
                                                 slewRateMvPerMs);
  if (status != HAL_OK)
  {
    (void)VoltageSim_ClearCellFault(highCell);
    return status;
  }

  return HAL_OK;
}

HAL_StatusTypeDef VoltageSim_ClearCellFault(uint8_t cell)
{
  HAL_StatusTypeDef status;
  VoltageSim_CellFaultInfo *info;

  if (VoltageSim_IsValidCell(cell) == 0U)
  {
    return HAL_ERROR;
  }

  info = &cellFaultInfo[cell - 1U];
  if (info->type == VOLTAGE_SIM_FAULT_NONE)
  {
    return HAL_OK;
  }

  status = VoltageSim_WriteCellVoltageMv(cell, info->restoreMillivolts, 1U);
  if (status == HAL_OK)
  {
    VoltageSim_ClearFaultRecord(cell);
  }

  return status;
}

HAL_StatusTypeDef VoltageSim_ClearAllFaults(void)
{
  HAL_StatusTypeDef status;

  for (uint8_t cell = 1U; cell <= VOLTAGE_SIM_CELL_COUNT; cell++)
  {
    status = VoltageSim_ClearCellFault(cell);
    if (status != HAL_OK)
    {
      return status;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef VoltageSim_Process(uint32_t nowMs)
{
  HAL_StatusTypeDef status;
  uint8_t outputChanged = 0U;

  for (uint8_t cell = 1U; cell <= VOLTAGE_SIM_CELL_COUNT; cell++)
  {
    uint8_t index = (uint8_t)(cell - 1U);
    VoltageSim_CellFaultInfo *info = &cellFaultInfo[index];
    uint16_t targetMillivolts;
    uint16_t nextMillivolts;
    uint32_t elapsedMs;
    uint32_t maxStepMv;

    if (info->type == VOLTAGE_SIM_FAULT_NONE)
    {
      continue;
    }

    if (faultLastUpdateMs[index] == 0U)
    {
      faultLastUpdateMs[index] = nowMs;
    }

    elapsedMs = nowMs - faultLastUpdateMs[index];
    targetMillivolts = (faultRestoring[index] != 0U) ?
                       info->restoreMillivolts :
                       info->targetMillivolts;

    if (cellVoltageMv[index] != targetMillivolts)
    {
      maxStepMv = (info->slewRateMvPerMs == 0U) ?
                  0U :
                  ((uint32_t)info->slewRateMvPerMs * elapsedMs);

      if ((info->slewRateMvPerMs != 0U) && (maxStepMv == 0U))
      {
        continue;
      }

      if (maxStepMv > VOLTAGE_SIM_MAX_MV)
      {
        maxStepMv = VOLTAGE_SIM_MAX_MV;
      }

      nextMillivolts = VoltageSim_CalculateNextVoltage(cellVoltageMv[index],
                                                       targetMillivolts,
                                                       maxStepMv);
      status = VoltageSim_WriteCellVoltageMv(cell, nextMillivolts, 0U);
      if (status != HAL_OK)
      {
        return status;
      }

      outputChanged = 1U;
      faultLastUpdateMs[index] = nowMs;

      if ((faultRestoring[index] == 0U) &&
          (nextMillivolts == info->targetMillivolts) &&
          (faultHoldStartedMs[index] == 0U))
      {
        faultHoldStartedMs[index] = nowMs;
      }

      if ((faultRestoring[index] != 0U) && (nextMillivolts == info->restoreMillivolts))
      {
        VoltageSim_ClearFaultRecord(cell);
      }

      continue;
    }

    faultLastUpdateMs[index] = nowMs;

    if (faultRestoring[index] != 0U)
    {
      VoltageSim_ClearFaultRecord(cell);
      continue;
    }

    if (faultHoldStartedMs[index] == 0U)
    {
      faultHoldStartedMs[index] = nowMs;
    }

    if ((info->durationMs > 0U) && ((nowMs - faultHoldStartedMs[index]) >= info->durationMs))
    {
      faultRestoring[index] = 1U;
    }
  }

  if (outputChanged != 0U)
  {
    DAC_LDAC_Update();
  }

  return HAL_OK;
}

uint16_t VoltageSim_MillivoltsToDacCode(uint16_t millivolts)
{
  if (millivolts >= VOLTAGE_SIM_MAX_MV)
  {
    return 0xFFFFU;
  }

  return (uint16_t)((((uint32_t)millivolts * 0xFFFFU) + (VOLTAGE_SIM_MAX_MV / 2U)) /
                    VOLTAGE_SIM_MAX_MV);
}

uint16_t VoltageSim_GetLastCellVoltageMv(uint8_t cell)
{
  if (VoltageSim_IsValidCell(cell) == 0U)
  {
    return 0U;
  }

  return cellVoltageMv[cell - 1U];
}

VoltageSim_CellFaultInfo VoltageSim_GetCellFaultInfo(uint8_t cell)
{
  VoltageSim_CellFaultInfo emptyInfo = {VOLTAGE_SIM_FAULT_NONE, 0U, 0U};

  if (VoltageSim_IsValidCell(cell) == 0U)
  {
    return emptyInfo;
  }

  return cellFaultInfo[cell - 1U];
}

uint8_t VoltageSim_GetActiveFaultCount(void)
{
  uint8_t count = 0U;

  for (uint8_t index = 0U; index < VOLTAGE_SIM_CELL_COUNT; index++)
  {
    if (cellFaultInfo[index].type != VOLTAGE_SIM_FAULT_NONE)
    {
      count++;
    }
  }

  return count;
}

const char *VoltageSim_GetFaultTypeName(VoltageSim_FaultType type)
{
  switch (type)
  {
    case VOLTAGE_SIM_FAULT_OVER_VOLTAGE:
      return "OV";

    case VOLTAGE_SIM_FAULT_UNDER_VOLTAGE:
      return "UV";

    case VOLTAGE_SIM_FAULT_NONE:
    default:
      return "NONE";
  }
}
