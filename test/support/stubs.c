/* Stub implementations for HAL functions and DAC functions used in unit tests */

#include "stm32h7xx_hal.h"
#include "dac81416.h"
#include "fdcan.h"
#include "bsp_can.h"
<<<<<<< HEAD
#include "usart.h"

/* UART handle needed by fault_console.c */
UART_HandleTypeDef huart1;
=======
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d

/* HAL Tick stub */
static uint32_t mockTick = 0U;

uint32_t HAL_GetTick(void)
{
    return mockTick;
}

void HAL_SetTick(uint32_t tick)
{
    mockTick = tick;
}

void HAL_Delay(uint32_t Delay)
{
    (void)Delay;
}

/* GPIO stubs */
void HAL_GPIO_WritePin(uint16_t GPIO_Pin, uint16_t GPIO_Port, GPIO_PinState PinState)
{
    (void)GPIO_Pin;
    (void)GPIO_Port;
    (void)PinState;
}

GPIO_PinState HAL_GPIO_ReadPin(uint16_t GPIO_Pin, uint16_t GPIO_Port)
{
    (void)GPIO_Pin;
    (void)GPIO_Port;
    return GPIO_PIN_RESET;
}

<<<<<<< HEAD
/* UART test buffer */
#define UART_TEST_BUFFER_SIZE 256
static uint8_t uartTestBuffer[UART_TEST_BUFFER_SIZE];
static uint16_t uartTestWriteIndex = 0;
static uint16_t uartTestReadIndex = 0;
static HAL_StatusTypeDef uartReceiveResult = HAL_OK;

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void)huart;
    (void)Size;
    (void)Timeout;

    if (uartTestReadIndex < uartTestWriteIndex) {
        *pData = uartTestBuffer[uartTestReadIndex++];
        return uartReceiveResult;
    }
    return HAL_ERROR;
}

void UART_PushData(const uint8_t *data, uint16_t length)
{
    uint16_t i;
    for (i = 0; i < length && uartTestWriteIndex < UART_TEST_BUFFER_SIZE; i++) {
        uartTestBuffer[uartTestWriteIndex++] = data[i];
    }
}

void UART_Reset(void)
{
    uartTestWriteIndex = 0;
    uartTestReadIndex = 0;
}

void UART_SetReceiveResult(HAL_StatusTypeDef result)
{
    uartReceiveResult = result;
}

=======
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
/* DAC stubs */
static HAL_StatusTypeDef dacWriteResult = HAL_OK;
static uint8_t dacWriteChannelCalled = 0U;
static uint8_t dacLastChannel = 0U;
static uint16_t dacLastValue = 0U;
static uint8_t ldacCalled = 0U;

HAL_StatusTypeDef DAC_WriteChannel(uint8_t channel, uint16_t value)
{
    dacWriteChannelCalled = 1U;
    dacLastChannel = channel;
    dacLastValue = value;
    return dacWriteResult;
}

void DAC_SetWriteResult(HAL_StatusTypeDef result)
{
    dacWriteResult = result;
}

uint8_t DAC_GetWriteChannelCalled(void)
{
    return dacWriteChannelCalled;
}

void DAC_ClearWriteChannelCalled(void)
{
    dacWriteChannelCalled = 0U;
}

uint8_t DAC_GetLastChannel(void)
{
    return dacLastChannel;
}

uint16_t DAC_GetLastValue(void)
{
    return dacLastValue;
}

void DAC_LDAC_Update(void)
{
    ldacCalled = 1U;
}

uint8_t DAC_GetLdacCalled(void)
{
    return ldacCalled;
}

void DAC_ClearLdacCalled(void)
{
    ldacCalled = 0U;
}

/* Alarm pin stub */
static uint8_t alarmPinValue = 1U;

void DAC_SetAlarmPinValue(uint8_t value)
{
    alarmPinValue = value;
}

uint8_t DAC_ReadAlarmPin(void)
{
    return alarmPinValue;
}

/* ========== FDCAN stubs ========== */
<<<<<<< HEAD
FDCAN_HandleTypeDef hfdcan1 = {0};
=======
FDCAN_HandleTypeDef hfdcan1 = 0;
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
static HAL_StatusTypeDef fdcanConfigFilterResult = HAL_OK;
static HAL_StatusTypeDef fdcanConfigGlobalResult = HAL_OK;
static HAL_StatusTypeDef fdcanStartResult = HAL_OK;
static HAL_StatusTypeDef fdcanAddMessageResult = HAL_OK;
static uint32_t fdcanRxFifoFillLevel = 0U;
static HAL_StatusTypeDef fdcanGetRxMessageResult = HAL_OK;
static uint32_t fdcanMsgIdToReturn = 0U;
static uint32_t fdcanMsgIdTypeToReturn = 0U;
static uint32_t fdcanMsgDlcToReturn = FDCAN_DLC_BYTES_8;
static uint8_t fdcanMsgDataToReturn[8] = {0};

HAL_StatusTypeDef HAL_FDCAN_ConfigFilter(FDCAN_HandleTypeDef *hfdcan, FDCAN_FilterTypeDef *sFilter)
{
    (void)hfdcan;
    (void)sFilter;
    return fdcanConfigFilterResult;
}

HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *hfdcan, uint32_t NonMatchingStd, uint32_t NonMatchingExt, uint32_t RejectRemoteStd, uint32_t RejectRemoteExt)
{
    (void)hfdcan;
    (void)NonMatchingStd; (void)NonMatchingExt; (void)RejectRemoteStd; (void)RejectRemoteExt;
    return fdcanConfigGlobalResult;
}

HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan)
{
    (void)hfdcan;
    return fdcanStartResult;
}

HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan, FDCAN_TxHeaderTypeDef *pTxHeader, uint8_t *pTxData)
{
    (void)hfdcan;
    (void)pTxHeader;
    (void)pTxData;
    return fdcanAddMessageResult;
}

uint32_t HAL_FDCAN_GetRxFifoFillLevel(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo)
{
    (void)hfdcan;
    (void)RxFifo;
    return fdcanRxFifoFillLevel;
}

HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo, FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData)
{
    (void)hfdcan;
    (void)RxFifo;
    uint8_t i;

<<<<<<< HEAD
    if (fdcanRxFifoFillLevel > 0U)
    {
        fdcanRxFifoFillLevel--;
    }

=======
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
    pRxHeader->Identifier = fdcanMsgIdToReturn;
    pRxHeader->IdType = fdcanMsgIdTypeToReturn;
    pRxHeader->DataLength = fdcanMsgDlcToReturn;
    for (i = 0; i < 8; i++) {
        pRxData[i] = fdcanMsgDataToReturn[i];
    }
    return fdcanGetRxMessageResult;
}

/* FDCAN test helpers */
void FDCAN_SetConfigFilterResult(HAL_StatusTypeDef result) { fdcanConfigFilterResult = result; }
void FDCAN_SetConfigGlobalResult(HAL_StatusTypeDef result) { fdcanConfigGlobalResult = result; }
void FDCAN_SetStartResult(HAL_StatusTypeDef result) { fdcanStartResult = result; }
void FDCAN_SetAddMessageResult(HAL_StatusTypeDef result) { fdcanAddMessageResult = result; }
void FDCAN_SetRxFifoFillLevel(uint32_t level) { fdcanRxFifoFillLevel = level; }
void FDCAN_SetGetRxMessageResult(HAL_StatusTypeDef result) { fdcanGetRxMessageResult = result; }
void FDCAN_SetRxData(uint32_t id, uint32_t idType, uint32_t dlc, const uint8_t *data)
{
    fdcanMsgIdToReturn = id;
    fdcanMsgIdTypeToReturn = idType;
    fdcanMsgDlcToReturn = dlc;
    if (data) {
        for (int i = 0; i < 8; i++) fdcanMsgDataToReturn[i] = data[i];
    }
}
void FDCAN_Reset(void)
{
    fdcanConfigFilterResult = HAL_OK;
    fdcanConfigGlobalResult = HAL_OK;
    fdcanStartResult = HAL_OK;
    fdcanAddMessageResult = HAL_OK;
    fdcanRxFifoFillLevel = 0U;
    fdcanGetRxMessageResult = HAL_OK;
    fdcanMsgIdToReturn = 0U;
    fdcanMsgIdTypeToReturn = 0U;
    fdcanMsgDlcToReturn = FDCAN_DLC_BYTES_8;
    for (int i = 0; i < 8; i++) fdcanMsgDataToReturn[i] = 0;
<<<<<<< HEAD
}
=======
}
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
