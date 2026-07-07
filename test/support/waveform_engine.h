/* Stub WaveformEngine header for unit testing */
/* Must match the real header structure since fault_command.c uses struct members */
#ifndef WAVEFORM_ENGINE_H
#define WAVEFORM_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define WAVEFORM_ENGINE_SINE_TABLE_SIZE  1024U
#define WAVEFORM_ENGINE_DMA_BUFFER_SIZE  8192U

typedef struct
{
  uint8_t  active;
  uint8_t  channels;
  uint8_t  channelList[13];
  uint32_t sampleRate;
  uint32_t pointCount;
  uint32_t cycleCount;
  uint16_t amplitudeMv;
  uint16_t offsetMv;
} WaveformEngine_Config;

typedef struct
{
  volatile uint8_t  running;
  volatile uint8_t  bufferIndex;
  uint32_t sampleCount;
  uint32_t errorCount;
} WaveformEngine_Status;

void WaveformEngine_Init(void);
HAL_StatusTypeDef WaveformEngine_StartSine(WaveformEngine_Config *config);
void WaveformEngine_Stop(void);
WaveformEngine_Status WaveformEngine_GetStatus(void);
HAL_StatusTypeDef WaveformEngine_GenerateSineTable(uint16_t *table, uint32_t points, uint16_t amplitude, uint16_t offset);
void WaveformEngine_DmaHalfComplete(void);
void WaveformEngine_DmaComplete(void);
void WaveformEngine_DmaError(void);
void WaveformEngine_TimerElapsed(void);

#ifdef __cplusplus
}
#endif

#endif /* WAVEFORM_ENGINE_H */