/* Stub implementations for WaveformEngine functions.
   These replace the real hardware-dependent waveform_engine.c during testing. */
#include "waveform_engine.h"

void WaveformEngine_Init(void) {}
HAL_StatusTypeDef WaveformEngine_StartSine(WaveformEngine_Config *config) { (void)config; return HAL_OK; }
void WaveformEngine_Stop(void) {}
WaveformEngine_Status WaveformEngine_GetStatus(void) {
    WaveformEngine_Status s;
    s.running = 0;
    s.bufferIndex = 0;
    s.sampleCount = 0;
    s.errorCount = 0;
    return s;
}
HAL_StatusTypeDef WaveformEngine_GenerateSineTable(uint16_t *table, uint32_t points, uint16_t amplitude, uint16_t offset) {
    (void)table; (void)points; (void)amplitude; (void)offset;
    return HAL_OK;
}
void WaveformEngine_DmaHalfComplete(void) {}
void WaveformEngine_DmaComplete(void) {}
void WaveformEngine_DmaError(void) {}
void WaveformEngine_TimerElapsed(void) {}