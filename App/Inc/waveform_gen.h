#ifndef WAVEFORM_GEN_H
#define WAVEFORM_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  WAVEFORM_GEN_NONE = 0,
  WAVEFORM_GEN_SQUARE,
  WAVEFORM_GEN_SINE
} WaveformGen_Type;

typedef struct
{
  WaveformGen_Type type;
  uint8_t active;
  uint8_t cell;
  uint16_t lowMillivolts;
  uint16_t highMillivolts;
  uint16_t offsetMillivolts;
  uint16_t amplitudeMillivolts;
  uint32_t periodMs;
  uint16_t lastMillivolts;
} WaveformGen_Status;

HAL_StatusTypeDef WaveformGen_StartSquare(uint8_t cell,
                                          uint16_t lowMillivolts,
                                          uint16_t highMillivolts,
                                          uint32_t periodMs);
HAL_StatusTypeDef WaveformGen_StartSine(uint8_t cell,
                                        uint16_t offsetMillivolts,
                                        uint16_t amplitudeMillivolts,
                                        uint32_t periodMs);
void WaveformGen_Stop(void);
HAL_StatusTypeDef WaveformGen_Process(uint32_t nowMs);
WaveformGen_Status WaveformGen_GetStatus(void);
const char *WaveformGen_GetTypeName(WaveformGen_Type type);

#ifdef __cplusplus
}
#endif

#endif /* WAVEFORM_GEN_H */
