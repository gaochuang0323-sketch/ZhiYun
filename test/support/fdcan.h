/* Stub FDCAN header for unit testing */
#ifndef __FDCAN_H__
#define __FDCAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define FDCAN_DLC_BYTES_0  0U
#define FDCAN_DLC_BYTES_1  1U
#define FDCAN_DLC_BYTES_2  2U
#define FDCAN_DLC_BYTES_3  3U
#define FDCAN_DLC_BYTES_4  4U
#define FDCAN_DLC_BYTES_5  5U
#define FDCAN_DLC_BYTES_6  6U
#define FDCAN_DLC_BYTES_7  7U
#define FDCAN_DLC_BYTES_8  8U

typedef uint32_t FDCAN_HandleTypeDef;

typedef enum {
    FDCAN_STANDARD_ID = 0,
    FDCAN_EXTENDED_ID = 1
} FDCAN_IdType;

typedef enum {
    FDCAN_DATA_FRAME = 0,
    FDCAN_REMOTE_FRAME = 1
} FDCAN_TxFrameType;

typedef enum {
    FDCAN_ESI_ACTIVE = 0,
    FDCAN_ESI_PASSIVE = 1
} FDCAN_ErrorStateIndicator;

typedef enum {
    FDCAN_BRS_OFF = 0,
    FDCAN_BRS_ON = 1
} FDCAN_BitRateSwitch;

typedef enum {
    FDCAN_CLASSIC_CAN = 0,
    FDCAN_FD_CAN = 1
} FDCAN_FDFormat;

typedef enum {
    FDCAN_NO_TX_EVENTS = 0,
    FDCAN_STORE_TX_EVENTS = 1
} FDCAN_TxEventFifoControl;

typedef enum {
    FDCAN_FILTER_MASK = 0,
    FDCAN_FILTER_RANGE = 1,
    FDCAN_FILTER_DUAL = 2
} FDCAN_FilterTypeType;

typedef enum {
    FDCAN_FILTER_TO_RXFIFO0 = 0,
    FDCAN_FILTER_TO_RXFIFO1 = 1,
    FDCAN_FILTER_REJECT = 2,
    FDCAN_FILTER_HIGH_PRIO = 3
} FDCAN_FilterConfigType;

typedef enum {
    FDCAN_RX_FIFO0 = 0,
    FDCAN_RX_FIFO1 = 1
} FDCAN_RxFifoType;

typedef enum {
    FDCAN_ACCEPT_IN_RX_FIFO0 = 0,
    FDCAN_ACCEPT_IN_RX_FIFO1 = 1,
    FDCAN_REJECT_REMOTE = 2,
    FDCAN_REJECT = 3
} FDCAN_GlobalFilterType;

typedef struct {
    uint32_t IdType;
    uint32_t FilterIndex;
    uint32_t FilterType;
    uint32_t FilterConfig;
    uint32_t FilterID1;
    uint32_t FilterID2;
} FDCAN_FilterTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t TxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t TxEventFifoControl;
    uint32_t MessageMarker;
} FDCAN_TxHeaderTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t RxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t RxTimestamp;
    uint32_t FilterIndex;
} FDCAN_RxHeaderTypeDef;

extern FDCAN_HandleTypeDef hfdcan1;

HAL_StatusTypeDef HAL_FDCAN_ConfigFilter(FDCAN_HandleTypeDef *hfdcan, FDCAN_FilterTypeDef *sFilter);
HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *hfdcan, uint32_t NonMatchingStd, uint32_t NonMatchingExt, uint32_t RejectRemoteStd, uint32_t RejectRemoteExt);
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan, FDCAN_TxHeaderTypeDef *pTxHeader, uint8_t *pTxData);
uint32_t HAL_FDCAN_GetRxFifoFillLevel(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo);
HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo, FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData);

#ifdef __cplusplus
}
#endif

#endif /* __FDCAN_H__ */