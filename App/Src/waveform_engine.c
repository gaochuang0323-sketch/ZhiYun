#include "waveform_engine.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bsp_dac81416.h"
#include "dac81416.h"
#include "spi.h"
#include "tim.h"
#include "voltage_sim.h"
#include "waveform_gen.h"

#define WAVEFORM_ENGINE_MAX_CHANNELS        13U
#define WAVEFORM_ENGINE_MIN_TIMER_RATE_HZ   1U
#define WAVEFORM_ENGINE_LDAC_NOP_COUNT      16U

typedef enum
{
  WAVEFORM_ENGINE_MODE_STOPPED = 0,
  WAVEFORM_ENGINE_MODE_IRQ_DMA,
  WAVEFORM_ENGINE_MODE_TIM_DMA_TURBO
} WaveformEngine_Mode;

static WaveformEngine_Status engineStatus;
static WaveformEngine_Config activeConfig;

static uint32_t *frameTable = (uint32_t *)BSP_SRAM_BASE_ADDRESS;
static uint32_t totalSamplePoints;
static uint32_t currentSamplePoint;
static volatile uint8_t txBusy;
static WaveformEngine_Mode engineMode;

static void WaveformEngine_CleanDCache(const void *address, uint32_t length)
{
  uint32_t start;
  uint32_t end;

  if ((address == NULL) || (length == 0U) || ((SCB->CCR & SCB_CCR_DC_Msk) == 0U))
  {
    return;
  }

  start = (uint32_t)address & ~31UL;
  end = ((uint32_t)address + length + 31UL) & ~31UL;
  SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
}

static uint8_t WaveformEngine_IsValidConfig(const WaveformEngine_Config *config)
{
  uint32_t frameCount;
  uint32_t maxFrameCount = WAVEFORM_ENGINE_DMA_BUFFER_SIZE / sizeof(uint32_t);

  if ((config == NULL) ||
      (config->sampleRate < WAVEFORM_ENGINE_MIN_TIMER_RATE_HZ) ||
      (config->pointCount == 0U) ||
      (config->pointCount > WAVEFORM_ENGINE_SINE_TABLE_SIZE) ||
      (config->channels == 0U) ||
      (config->channels > WAVEFORM_ENGINE_MAX_CHANNELS) ||
      (config->amplitudeMv == 0U))
  {
    return 0U;
  }

  frameCount = config->pointCount * config->channels;
  if ((frameCount == 0U) || (frameCount > maxFrameCount))
  {
    return 0U;
  }

  for (uint8_t index = 0U; index < config->channels; index++)
  {
    if ((config->channelList[index] == 0U) ||
        (config->channelList[index] > VOLTAGE_SIM_CELL_COUNT))
    {
      return 0U;
    }
  }

  return 1U;
}

static uint32_t WaveformEngine_BuildDacFrame(uint8_t cell, uint16_t dacCode)
{
  uint8_t channel = (uint8_t)(cell - 1U);
  uint8_t command = (uint8_t)(DAC81416_CMD_WRITE | ((DAC81416_REG_DAC0 + channel) & 0x3FU));

  return ((uint32_t)command << 16) | (uint32_t)dacCode;
}

static void WaveformEngine_PulseLdacFast(void)
{
  DAC_SetLDACPin(GPIO_PIN_RESET);
  for (uint32_t index = 0U; index < WAVEFORM_ENGINE_LDAC_NOP_COUNT; index++)
  {
    __NOP();
  }
  DAC_SetLDACPin(GPIO_PIN_SET);
}

static uint32_t WaveformEngine_GetTim3ClockHz(void)
{
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  uint32_t ppre1 = RCC->D2CFGR & RCC_D2CFGR_D2PPRE1;

  return (ppre1 == RCC_D2CFGR_D2PPRE1_DIV1) ? pclk1 : (pclk1 * 2U);
}

static HAL_StatusTypeDef WaveformEngine_SetSampleRate(uint32_t sampleRate)
{
  uint32_t timerClock;
  uint32_t prescaler;
  uint32_t period;
  uint64_t minDivider;

  if (sampleRate == 0U)
  {
    return HAL_ERROR;
  }

  timerClock = WaveformEngine_GetTim3ClockHz();
  if (sampleRate > timerClock)
  {
    return HAL_ERROR;
  }

  minDivider = (uint64_t)sampleRate * 65536ULL;
  prescaler = (uint32_t)(((uint64_t)timerClock + minDivider - 1ULL) / minDivider);
  if (prescaler > 0U)
  {
    prescaler--;
  }
  if (prescaler > 0xFFFFU)
  {
    return HAL_ERROR;
  }

  period = timerClock / ((prescaler + 1U) * sampleRate);
  if ((period == 0U) || (period > 0x10000U))
  {
    return HAL_ERROR;
  }

  __HAL_TIM_DISABLE(&htim3);
  __HAL_TIM_SET_PRESCALER(&htim3, prescaler);
  __HAL_TIM_SET_AUTORELOAD(&htim3, period - 1U);
  __HAL_TIM_SET_COUNTER(&htim3, 0U);
  if (HAL_TIM_GenerateEvent(&htim3, TIM_EVENTSOURCE_UPDATE) != HAL_OK)
  {
    return HAL_ERROR;
  }
  __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);

  return HAL_OK;
}

static HAL_StatusTypeDef WaveformEngine_ConfigDma(uint32_t request, uint32_t mode)
{
  (void)HAL_DMA_Abort(&hdma_spi1_tx);
  (void)HAL_DMA_DeInit(&hdma_spi1_tx);

  hdma_spi1_tx.Instance = DMA1_Stream3;
  hdma_spi1_tx.Init.Request = request;
  hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_spi1_tx.Init.Mode = mode;
  hdma_spi1_tx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

  if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
  {
    return HAL_ERROR;
  }

  __HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);
  return HAL_OK;
}

static void WaveformEngine_TurboHalfComplete(DMA_HandleTypeDef *hdma)
{
  (void)hdma;
  engineStatus.bufferIndex = 1U;
  engineStatus.sampleCount += totalSamplePoints / 2U;
}

static void WaveformEngine_TurboComplete(DMA_HandleTypeDef *hdma)
{
  (void)hdma;
  engineStatus.bufferIndex = 0U;
  engineStatus.sampleCount += (totalSamplePoints + 1U) / 2U;
}

static void WaveformEngine_TurboError(DMA_HandleTypeDef *hdma)
{
  (void)hdma;
  WaveformEngine_DmaError();
}

static HAL_StatusTypeDef WaveformEngine_StartTimDmaTurbo(void)
{
  if (activeConfig.channels != 1U)
  {
    return HAL_ERROR;
  }

  if (WaveformEngine_ConfigDma(DMA_REQUEST_TIM3_UP, DMA_CIRCULAR) != HAL_OK)
  {
    return HAL_ERROR;
  }

  hdma_spi1_tx.XferHalfCpltCallback = WaveformEngine_TurboHalfComplete;
  hdma_spi1_tx.XferCpltCallback = WaveformEngine_TurboComplete;
  hdma_spi1_tx.XferErrorCallback = WaveformEngine_TurboError;
  hdma_spi1_tx.XferAbortCallback = NULL;

  WaveformEngine_CleanDCache(frameTable, totalSamplePoints * sizeof(uint32_t));

  CLEAR_BIT(hspi1.Instance->CR1, SPI_CR1_CSTART);
  CLEAR_BIT(hspi1.Instance->CFG1, SPI_CFG1_TXDMAEN);
  MODIFY_REG(hspi1.Instance->CR2, SPI_CR2_TSIZE, 0UL);
  __HAL_SPI_ENABLE(&hspi1);

  if (HAL_DMA_Start_IT(&hdma_spi1_tx,
                       (uint32_t)frameTable,
                       (uint32_t)&hspi1.Instance->TXDR,
                       totalSamplePoints) != HAL_OK)
  {
    return HAL_ERROR;
  }

  DAC_SetLDACPin(GPIO_PIN_RESET);
  DAC_CS_LOW();
  SET_BIT(hspi1.Instance->CR1, SPI_CR1_CSTART);
  __HAL_TIM_ENABLE_DMA(&htim3, TIM_DMA_UPDATE);

  if (HAL_TIM_Base_Start(&htim3) != HAL_OK)
  {
    __HAL_TIM_DISABLE_DMA(&htim3, TIM_DMA_UPDATE);
    (void)HAL_DMA_Abort(&hdma_spi1_tx);
    DAC_CS_HIGH();
    DAC_SetLDACPin(GPIO_PIN_SET);
    CLEAR_BIT(hspi1.Instance->CR1, SPI_CR1_CSTART);
    return HAL_ERROR;
  }

  engineMode = WAVEFORM_ENGINE_MODE_TIM_DMA_TURBO;
  return HAL_OK;
}

static HAL_StatusTypeDef WaveformEngine_StartIrqDma(void)
{
  if (WaveformEngine_ConfigDma(DMA_REQUEST_SPI1_TX, DMA_NORMAL) != HAL_OK)
  {
    return HAL_ERROR;
  }

  engineMode = WAVEFORM_ENGINE_MODE_IRQ_DMA;
  return HAL_TIM_Base_Start_IT(&htim3);
}

static void WaveformEngine_StopHardware(void)
{
  (void)HAL_TIM_Base_Stop_IT(&htim3);
  (void)HAL_TIM_Base_Stop(&htim3);
  __HAL_TIM_DISABLE_DMA(&htim3, TIM_DMA_UPDATE);

  if (engineMode == WAVEFORM_ENGINE_MODE_TIM_DMA_TURBO)
  {
    (void)HAL_DMA_Abort(&hdma_spi1_tx);
    CLEAR_BIT(hspi1.Instance->CFG1, SPI_CFG1_TXDMAEN);
    CLEAR_BIT(hspi1.Instance->CR1, SPI_CR1_CSTART);
  }
  else
  {
    (void)HAL_SPI_DMAStop(&hspi1);
  }

  DAC_CS_HIGH();
  DAC_SetLDACPin(GPIO_PIN_SET);
  txBusy = 0U;

  (void)WaveformEngine_ConfigDma(DMA_REQUEST_SPI1_TX, DMA_NORMAL);
  engineMode = WAVEFORM_ENGINE_MODE_STOPPED;
}

void WaveformEngine_Init(void)
{
  memset(&engineStatus, 0, sizeof(engineStatus));
  memset(&activeConfig, 0, sizeof(activeConfig));
  totalSamplePoints = 0U;
  currentSamplePoint = 0U;
  txBusy = 0U;
  engineMode = WAVEFORM_ENGINE_MODE_STOPPED;
}

HAL_StatusTypeDef WaveformEngine_GenerateSineTable(uint16_t *table,
                                                   uint32_t points,
                                                   uint16_t amplitude,
                                                   uint16_t offset)
{
  if ((table == NULL) ||
      (points == 0U) ||
      (points > WAVEFORM_ENGINE_SINE_TABLE_SIZE))
  {
    return HAL_ERROR;
  }

  for (uint32_t i = 0U; i < points; i++)
  {
    double rad = 2.0 * 3.141592653589793 * (double)i / (double)points;
    double mv = (double)offset + (double)amplitude * sin(rad);

    if (mv < (double)VOLTAGE_SIM_MIN_MV)
    {
      mv = (double)VOLTAGE_SIM_MIN_MV;
    }
    if (mv > (double)VOLTAGE_SIM_MAX_MV)
    {
      mv = (double)VOLTAGE_SIM_MAX_MV;
    }

    table[i] = VoltageSim_MillivoltsToDacCode((uint16_t)mv);
  }

  return HAL_OK;
}

HAL_StatusTypeDef WaveformEngine_StartSine(WaveformEngine_Config *config)
{
  uint16_t sineCodes[WAVEFORM_ENGINE_SINE_TABLE_SIZE];

  if (config == NULL)
  {
    return HAL_ERROR;
  }

  if (config->cycleCount == 0U)
  {
    config->cycleCount = 1U;
  }

  if (WaveformEngine_IsValidConfig(config) == 0U)
  {
    return HAL_ERROR;
  }

  if (engineStatus.running != 0U)
  {
    WaveformEngine_Stop();
  }

  WaveformGen_Stop();

  if (WaveformEngine_GenerateSineTable(sineCodes,
                                       config->pointCount,
                                       config->amplitudeMv,
                                       config->offsetMv) != HAL_OK)
  {
    return HAL_ERROR;
  }

  for (uint32_t point = 0U; point < config->pointCount; point++)
  {
    for (uint8_t ch = 0U; ch < config->channels; ch++)
    {
      uint32_t frameIndex = (point * config->channels) + ch;
      frameTable[frameIndex] = WaveformEngine_BuildDacFrame(config->channelList[ch],
                                                            sineCodes[point]);
    }
  }

  if (WaveformEngine_SetSampleRate(config->sampleRate) != HAL_OK)
  {
    return HAL_ERROR;
  }

  memcpy(&activeConfig, config, sizeof(activeConfig));
  activeConfig.active = 1U;
  totalSamplePoints = activeConfig.pointCount;
  currentSamplePoint = 0U;
  txBusy = 0U;

  engineStatus.running = 1U;
  engineStatus.bufferIndex = 0U;
  engineStatus.sampleCount = 0U;
  engineStatus.errorCount = 0U;

  if (((activeConfig.channels == 1U) ? WaveformEngine_StartTimDmaTurbo() : WaveformEngine_StartIrqDma()) != HAL_OK)
  {
    WaveformEngine_StopHardware();
    activeConfig.active = 0U;
    engineStatus.running = 0U;
    return HAL_ERROR;
  }

  printf("[wave-high] started sample=%luHz points=%lu ch=%u mode=%s\r\n",
         (unsigned long)activeConfig.sampleRate,
         (unsigned long)activeConfig.pointCount,
         activeConfig.channels,
         (engineMode == WAVEFORM_ENGINE_MODE_TIM_DMA_TURBO) ? "tim-dma" : "irq-dma");

  return HAL_OK;
}

void WaveformEngine_Stop(void)
{
  if (engineStatus.running == 0U)
  {
    return;
  }

  WaveformEngine_StopHardware();
  activeConfig.active = 0U;
  engineStatus.running = 0U;

  printf("[wave-high] stopped samples=%lu errors=%lu\r\n",
         (unsigned long)engineStatus.sampleCount,
         (unsigned long)engineStatus.errorCount);
}

WaveformEngine_Status WaveformEngine_GetStatus(void)
{
  return engineStatus;
}

void WaveformEngine_TimerElapsed(void)
{
  uint32_t frameOffset;

  if ((engineStatus.running == 0U) || (engineMode != WAVEFORM_ENGINE_MODE_IRQ_DMA))
  {
    return;
  }

  if (txBusy != 0U)
  {
    engineStatus.errorCount++;
    return;
  }

  frameOffset = currentSamplePoint * activeConfig.channels;
  DAC_CS_LOW();

  /* In SPI_DATASIZE_24BIT mode, HAL_SPI_Transmit_DMA Size is a frame count.
     Each frame is stored as one uint32_t word in frameTable. */
  if (HAL_SPI_Transmit_DMA(&hspi1,
                           (uint8_t *)&frameTable[frameOffset],
                           (uint16_t)activeConfig.channels) != HAL_OK)
  {
    DAC_CS_HIGH();
    engineStatus.errorCount++;
    return;
  }

  txBusy = 1U;
  currentSamplePoint++;
  if (currentSamplePoint >= totalSamplePoints)
  {
    currentSamplePoint = 0U;
  }
}

void WaveformEngine_DmaHalfComplete(void)
{
  engineStatus.bufferIndex = 1U;
}

void WaveformEngine_DmaComplete(void)
{
  engineStatus.bufferIndex = 0U;
}

void WaveformEngine_DmaError(void)
{
  engineStatus.errorCount++;
  WaveformEngine_StopHardware();
  activeConfig.active = 0U;
  engineStatus.running = 0U;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance != SPI1)
  {
    return;
  }

  if ((engineStatus.running != 0U) && (engineMode == WAVEFORM_ENGINE_MODE_IRQ_DMA))
  {
    DAC_CS_HIGH();
    WaveformEngine_PulseLdacFast();
    txBusy = 0U;
    engineStatus.sampleCount++;
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1)
  {
    WaveformEngine_DmaError();
  }
}
