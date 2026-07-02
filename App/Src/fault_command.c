#include "fault_command.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "voltage_sim.h"

#define FAULT_COMMAND_DEFAULT_DURATION_MS 100U
#define FAULT_COMMAND_DEFAULT_SLEW_MV_PER_MS 3000U

static void FaultCommand_Write(FaultCommand_WriteFn write, void *context, const char *text)
{
  if ((write != NULL) && (text != NULL))
  {
    write(text, context);
  }
}

static void FaultCommand_WriteFormat(FaultCommand_WriteFn write,
                                     void *context,
                                     const char *format,
                                     ...)
{
  char buffer[192];
  va_list args;

  va_start(args, format);
  (void)vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  FaultCommand_Write(write, context, buffer);
}

static const char *FaultCommand_SkipSpaces(const char *text)
{
  while ((text != NULL) && (*text == ' '))
  {
    text++;
  }

  return text;
}

static void FaultCommand_WriteTextHelp(FaultCommand_WriteFn write, void *context)
{
  FaultCommand_Write(write, context, "\r\n[cmd] help\r\n");
  FaultCommand_Write(write, context, "  help\r\n");
  FaultCommand_Write(write, context, "  status\r\n");
  FaultCommand_Write(write, context, "  normal [mv]\r\n");
  FaultCommand_Write(write, context, "  set <cell> <mv>\r\n");
  FaultCommand_Write(write, context, "  ov <cell> [mv] [duration_ms] [slew_mv_per_ms]\r\n");
  FaultCommand_Write(write, context, "  uv <cell> [mv] [duration_ms] [slew_mv_per_ms]\r\n");
  FaultCommand_Write(write, context, "  diff <high_cell> <high_mv> <low_cell> <low_mv> [duration_ms] [slew_mv_per_ms]\r\n");
  FaultCommand_Write(write, context, "  clear [cell]\r\n");
  FaultCommand_Write(write, context, "  JSON examples: {\"cmd\":\"status\"}, {\"cmd\":\"ov\",\"cell\":7}\r\n");
}

static void FaultCommand_WriteTextStatus(FaultCommand_WriteFn write, void *context)
{
  FaultCommand_WriteFormat(write, context, "\r\n[status] active_faults=%u\r\n",
                           VoltageSim_GetActiveFaultCount());

  for (uint8_t cell = 1U; cell <= VOLTAGE_SIM_CELL_COUNT; cell++)
  {
    VoltageSim_CellFaultInfo info = VoltageSim_GetCellFaultInfo(cell);

    FaultCommand_WriteFormat(write, context, "  C%02u=%umV",
                             cell,
                             VoltageSim_GetLastCellVoltageMv(cell));
    if (info.type != VOLTAGE_SIM_FAULT_NONE)
    {
      FaultCommand_WriteFormat(write,
                               context,
                               " fault=%s target=%umV restore=%umV duration=%lums slew=%umV/ms",
                               VoltageSim_GetFaultTypeName(info.type),
                               info.targetMillivolts,
                               info.restoreMillivolts,
                               (unsigned long)info.durationMs,
                               info.slewRateMvPerMs);
    }
    FaultCommand_Write(write, context, "\r\n");
  }
}

static void FaultCommand_WriteTextResult(FaultCommand_WriteFn write,
                                         void *context,
                                         const char *name,
                                         HAL_StatusTypeDef status)
{
  FaultCommand_WriteFormat(write, context, "[cmd] %s %s\r\n",
                           name,
                           (status == HAL_OK) ? "OK" : "ERR");
}

static void FaultCommand_WriteJsonResult(FaultCommand_WriteFn write,
                                         void *context,
                                         const char *cmd,
                                         HAL_StatusTypeDef status)
{
  FaultCommand_WriteFormat(write, context, "{\"ok\":%s,\"cmd\":\"%s\"}\r\n",
                           (status == HAL_OK) ? "true" : "false",
                           cmd);
}

static void FaultCommand_WriteJsonStatus(FaultCommand_WriteFn write, void *context)
{
  FaultCommand_WriteFormat(write,
                           context,
                           "{\"ok\":true,\"active_faults\":%u,\"cells\":[",
                           VoltageSim_GetActiveFaultCount());

  for (uint8_t cell = 1U; cell <= VOLTAGE_SIM_CELL_COUNT; cell++)
  {
    VoltageSim_CellFaultInfo info = VoltageSim_GetCellFaultInfo(cell);

    FaultCommand_WriteFormat(write,
                             context,
                             "%s{\"cell\":%u,\"mv\":%u,\"fault\":\"%s\",\"target_mv\":%u,\"restore_mv\":%u,\"duration_ms\":%lu,\"slew_mv_per_ms\":%u}",
                             (cell == 1U) ? "" : ",",
                             cell,
                             VoltageSim_GetLastCellVoltageMv(cell),
                             VoltageSim_GetFaultTypeName(info.type),
                             info.targetMillivolts,
                             info.restoreMillivolts,
                             (unsigned long)info.durationMs,
                             info.slewRateMvPerMs);
  }

  FaultCommand_Write(write, context, "]}\r\n");
}

static HAL_StatusTypeDef FaultCommand_RunNormal(uint16_t millivolts)
{
  return VoltageSim_SetAllCellsVoltageMv(millivolts);
}

static HAL_StatusTypeDef FaultCommand_RunSet(uint8_t cell, uint16_t millivolts)
{
  return VoltageSim_SetCellVoltageMv(cell, millivolts);
}

static HAL_StatusTypeDef FaultCommand_RunOverVoltage(uint8_t cell,
                                                     uint16_t target,
                                                     uint32_t duration,
                                                     uint16_t slew)
{
  return VoltageSim_InjectCellOverVoltageRamp(cell, target, duration, slew);
}

static HAL_StatusTypeDef FaultCommand_RunUnderVoltage(uint8_t cell,
                                                      uint16_t target,
                                                      uint32_t duration,
                                                      uint16_t slew)
{
  return VoltageSim_InjectCellUnderVoltageRamp(cell, target, duration, slew);
}

static HAL_StatusTypeDef FaultCommand_RunDiff(uint8_t highCell,
                                              uint16_t highMv,
                                              uint8_t lowCell,
                                              uint16_t lowMv,
                                              uint32_t duration,
                                              uint16_t slew)
{
  return VoltageSim_InjectVoltageDifference(highCell, highMv, lowCell, lowMv, duration, slew);
}

static void FaultCommand_ExecuteText(const char *line, FaultCommand_WriteFn write, void *context)
{
  unsigned int cell;
  unsigned int millivolts;
  unsigned int target;
  unsigned long duration;
  unsigned int slew;

  if ((strcmp(line, "help") == 0) || (strcmp(line, "?") == 0))
  {
    FaultCommand_WriteTextHelp(write, context);
  }
  else if (strcmp(line, "status") == 0)
  {
    FaultCommand_WriteTextStatus(write, context);
  }
  else if (strncmp(line, "normal", 6U) == 0)
  {
    millivolts = VOLTAGE_SIM_DEFAULT_NORMAL_MV;
    (void)sscanf(line, "normal %u", &millivolts);
    FaultCommand_WriteTextResult(write,
                                 context,
                                 "normal",
                                 FaultCommand_RunNormal((uint16_t)millivolts));
  }
  else if (sscanf(line, "set %u %u", &cell, &millivolts) == 2)
  {
    FaultCommand_WriteTextResult(write,
                                 context,
                                 "set",
                                 FaultCommand_RunSet((uint8_t)cell, (uint16_t)millivolts));
  }
  else if (strncmp(line, "ov ", 3U) == 0)
  {
    cell = 0U;
    target = VOLTAGE_SIM_DEFAULT_OV_MV;
    duration = FAULT_COMMAND_DEFAULT_DURATION_MS;
    slew = FAULT_COMMAND_DEFAULT_SLEW_MV_PER_MS;
    if (sscanf(line, "ov %u %u %lu %u", &cell, &target, &duration, &slew) < 1)
    {
      FaultCommand_WriteTextResult(write, context, "ov", HAL_ERROR);
      return;
    }

    FaultCommand_WriteTextResult(write,
                                 context,
                                 "ov",
                                 FaultCommand_RunOverVoltage((uint8_t)cell,
                                                             (uint16_t)target,
                                                             (uint32_t)duration,
                                                             (uint16_t)slew));
  }
  else if (strncmp(line, "uv ", 3U) == 0)
  {
    cell = 0U;
    target = VOLTAGE_SIM_DEFAULT_UV_MV;
    duration = FAULT_COMMAND_DEFAULT_DURATION_MS;
    slew = FAULT_COMMAND_DEFAULT_SLEW_MV_PER_MS;
    if (sscanf(line, "uv %u %u %lu %u", &cell, &target, &duration, &slew) < 1)
    {
      FaultCommand_WriteTextResult(write, context, "uv", HAL_ERROR);
      return;
    }

    FaultCommand_WriteTextResult(write,
                                 context,
                                 "uv",
                                 FaultCommand_RunUnderVoltage((uint8_t)cell,
                                                              (uint16_t)target,
                                                              (uint32_t)duration,
                                                              (uint16_t)slew));
  }
  else if (strncmp(line, "diff ", 5U) == 0)
  {
    unsigned int highCell;
    unsigned int highMv;
    unsigned int lowCell;
    unsigned int lowMv;

    duration = FAULT_COMMAND_DEFAULT_DURATION_MS;
    slew = FAULT_COMMAND_DEFAULT_SLEW_MV_PER_MS;
    if (sscanf(line,
               "diff %u %u %u %u %lu %u",
               &highCell,
               &highMv,
               &lowCell,
               &lowMv,
               &duration,
               &slew) < 4)
    {
      FaultCommand_WriteTextResult(write, context, "diff", HAL_ERROR);
      return;
    }

    FaultCommand_WriteTextResult(write,
                                 context,
                                 "diff",
                                 FaultCommand_RunDiff((uint8_t)highCell,
                                                      (uint16_t)highMv,
                                                      (uint8_t)lowCell,
                                                      (uint16_t)lowMv,
                                                      (uint32_t)duration,
                                                      (uint16_t)slew));
  }
  else if (strncmp(line, "clear", 5U) == 0)
  {
    if (sscanf(line, "clear %u", &cell) == 1)
    {
      FaultCommand_WriteTextResult(write, context, "clear", VoltageSim_ClearCellFault((uint8_t)cell));
    }
    else
    {
      FaultCommand_WriteTextResult(write, context, "clear", VoltageSim_ClearAllFaults());
    }
  }
  else if (*line != '\0')
  {
    FaultCommand_WriteFormat(write, context, "[cmd] unknown: %s\r\n", line);
  }
}

static const char *FaultCommand_FindJsonKey(const char *json, const char *key)
{
  char pattern[32];

  (void)snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  return strstr(json, pattern);
}

static uint8_t FaultCommand_JsonGetString(const char *json,
                                          const char *key,
                                          char *value,
                                          uint32_t valueSize)
{
  const char *cursor = FaultCommand_FindJsonKey(json, key);
  uint32_t index = 0U;

  if ((cursor == NULL) || (value == NULL) || (valueSize == 0U))
  {
    return 0U;
  }

  cursor = strchr(cursor, ':');
  if (cursor == NULL)
  {
    return 0U;
  }

  cursor++;
  while (*cursor == ' ')
  {
    cursor++;
  }

  if (*cursor != '"')
  {
    return 0U;
  }

  cursor++;
  while ((*cursor != '\0') && (*cursor != '"') && (index < (valueSize - 1U)))
  {
    value[index++] = *cursor++;
  }
  value[index] = '\0';

  return (*cursor == '"') ? 1U : 0U;
}

static uint8_t FaultCommand_JsonGetUint(const char *json, const char *key, uint32_t *value)
{
  const char *cursor = FaultCommand_FindJsonKey(json, key);

  if ((cursor == NULL) || (value == NULL))
  {
    return 0U;
  }

  cursor = strchr(cursor, ':');
  if (cursor == NULL)
  {
    return 0U;
  }

  cursor++;
  while (*cursor == ' ')
  {
    cursor++;
  }

  *value = (uint32_t)strtoul(cursor, NULL, 10);
  return 1U;
}

static uint32_t FaultCommand_JsonGetUintDefault(const char *json, const char *key, uint32_t defaultValue)
{
  uint32_t value = defaultValue;

  (void)FaultCommand_JsonGetUint(json, key, &value);
  return value;
}

static void FaultCommand_ExecuteJson(const char *json, FaultCommand_WriteFn write, void *context)
{
  char cmd[16];
  uint32_t cell;
  uint32_t mv;
  uint32_t duration;
  uint32_t slew;
  HAL_StatusTypeDef status = HAL_ERROR;

  if (FaultCommand_JsonGetString(json, "cmd", cmd, sizeof(cmd)) == 0U)
  {
    FaultCommand_Write(write, context, "{\"ok\":false,\"error\":\"missing_cmd\"}\r\n");
    return;
  }

  if (strcmp(cmd, "status") == 0)
  {
    FaultCommand_WriteJsonStatus(write, context);
    return;
  }

  if (strcmp(cmd, "help") == 0)
  {
    FaultCommand_Write(write,
                       context,
                       "{\"ok\":true,\"commands\":[\"status\",\"normal\",\"set\",\"ov\",\"uv\",\"diff\",\"clear\"]}\r\n");
    return;
  }

  if (strcmp(cmd, "normal") == 0)
  {
    mv = FaultCommand_JsonGetUintDefault(json, "mv", VOLTAGE_SIM_DEFAULT_NORMAL_MV);
    status = FaultCommand_RunNormal((uint16_t)mv);
  }
  else if (strcmp(cmd, "set") == 0)
  {
    if ((FaultCommand_JsonGetUint(json, "cell", &cell) != 0U) &&
        (FaultCommand_JsonGetUint(json, "mv", &mv) != 0U))
    {
      status = FaultCommand_RunSet((uint8_t)cell, (uint16_t)mv);
    }
  }
  else if ((strcmp(cmd, "ov") == 0) || (strcmp(cmd, "over_voltage") == 0))
  {
    if (FaultCommand_JsonGetUint(json, "cell", &cell) != 0U)
    {
      mv = FaultCommand_JsonGetUintDefault(json, "mv", VOLTAGE_SIM_DEFAULT_OV_MV);
      duration = FaultCommand_JsonGetUintDefault(json, "duration_ms", FAULT_COMMAND_DEFAULT_DURATION_MS);
      slew = FaultCommand_JsonGetUintDefault(json, "slew_mv_per_ms", FAULT_COMMAND_DEFAULT_SLEW_MV_PER_MS);
      status = FaultCommand_RunOverVoltage((uint8_t)cell, (uint16_t)mv, duration, (uint16_t)slew);
    }
  }
  else if ((strcmp(cmd, "uv") == 0) || (strcmp(cmd, "under_voltage") == 0))
  {
    if (FaultCommand_JsonGetUint(json, "cell", &cell) != 0U)
    {
      mv = FaultCommand_JsonGetUintDefault(json, "mv", VOLTAGE_SIM_DEFAULT_UV_MV);
      duration = FaultCommand_JsonGetUintDefault(json, "duration_ms", FAULT_COMMAND_DEFAULT_DURATION_MS);
      slew = FaultCommand_JsonGetUintDefault(json, "slew_mv_per_ms", FAULT_COMMAND_DEFAULT_SLEW_MV_PER_MS);
      status = FaultCommand_RunUnderVoltage((uint8_t)cell, (uint16_t)mv, duration, (uint16_t)slew);
    }
  }
  else if (strcmp(cmd, "diff") == 0)
  {
    uint32_t highCell;
    uint32_t highMv;
    uint32_t lowCell;
    uint32_t lowMv;

    if ((FaultCommand_JsonGetUint(json, "high_cell", &highCell) != 0U) &&
        (FaultCommand_JsonGetUint(json, "high_mv", &highMv) != 0U) &&
        (FaultCommand_JsonGetUint(json, "low_cell", &lowCell) != 0U) &&
        (FaultCommand_JsonGetUint(json, "low_mv", &lowMv) != 0U))
    {
      duration = FaultCommand_JsonGetUintDefault(json, "duration_ms", FAULT_COMMAND_DEFAULT_DURATION_MS);
      slew = FaultCommand_JsonGetUintDefault(json, "slew_mv_per_ms", FAULT_COMMAND_DEFAULT_SLEW_MV_PER_MS);
      status = FaultCommand_RunDiff((uint8_t)highCell,
                                    (uint16_t)highMv,
                                    (uint8_t)lowCell,
                                    (uint16_t)lowMv,
                                    duration,
                                    (uint16_t)slew);
    }
  }
  else if (strcmp(cmd, "clear") == 0)
  {
    if (FaultCommand_JsonGetUint(json, "cell", &cell) != 0U)
    {
      status = VoltageSim_ClearCellFault((uint8_t)cell);
    }
    else
    {
      status = VoltageSim_ClearAllFaults();
    }
  }
  else
  {
    FaultCommand_Write(write, context, "{\"ok\":false,\"error\":\"unknown_cmd\"}\r\n");
    return;
  }

  FaultCommand_WriteJsonResult(write, context, cmd, status);
}

void FaultCommand_ExecuteLine(const char *line, FaultCommand_WriteFn write, void *context)
{
  line = FaultCommand_SkipSpaces(line);

  if (line == NULL)
  {
    return;
  }

  if (*line == '{')
  {
    FaultCommand_ExecuteJson(line, write, context);
  }
  else
  {
    FaultCommand_ExecuteText(line, write, context);
  }
}
