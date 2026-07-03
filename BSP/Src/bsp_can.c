#include "bsp_can.h"

#include <stdio.h>
#include <string.h>

#include "fdcan.h"

static BspCan_Status canStatus;

__weak void BspCan_OnRxFrame(const BspCan_RxFrame *frame)
{
  (void)frame;
}

static uint32_t BspCan_LengthToDlc(uint8_t length)
{
  switch (length)
  {
    case 0U: return FDCAN_DLC_BYTES_0;
    case 1U: return FDCAN_DLC_BYTES_1;
    case 2U: return FDCAN_DLC_BYTES_2;
    case 3U: return FDCAN_DLC_BYTES_3;
    case 4U: return FDCAN_DLC_BYTES_4;
    case 5U: return FDCAN_DLC_BYTES_5;
    case 6U: return FDCAN_DLC_BYTES_6;
    case 7U: return FDCAN_DLC_BYTES_7;
    case 8U:
    default:
      return FDCAN_DLC_BYTES_8;
  }
}

static uint8_t BspCan_DlcToLength(uint32_t dlc)
{
  switch (dlc)
  {
    case FDCAN_DLC_BYTES_0: return 0U;
    case FDCAN_DLC_BYTES_1: return 1U;
    case FDCAN_DLC_BYTES_2: return 2U;
    case FDCAN_DLC_BYTES_3: return 3U;
    case FDCAN_DLC_BYTES_4: return 4U;
    case FDCAN_DLC_BYTES_5: return 5U;
    case FDCAN_DLC_BYTES_6: return 6U;
    case FDCAN_DLC_BYTES_7: return 7U;
    case FDCAN_DLC_BYTES_8:
    default:
      return 8U;
  }
}

static HAL_StatusTypeDef BspCan_ConfigAcceptAllFilters(void)
{
  FDCAN_FilterTypeDef filter = {0};

  filter.IdType = FDCAN_STANDARD_ID;
  filter.FilterIndex = 0U;
  filter.FilterType = FDCAN_FILTER_MASK;
  filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  filter.FilterID1 = 0x000U;
  filter.FilterID2 = 0x000U;
  if (HAL_FDCAN_ConfigFilter(&hfdcan1, &filter) != HAL_OK)
  {
    return HAL_ERROR;
  }

  filter.IdType = FDCAN_EXTENDED_ID;
  filter.FilterIndex = 0U;
  filter.FilterID1 = 0x00000000U;
  filter.FilterID2 = 0x00000000U;
  if (HAL_FDCAN_ConfigFilter(&hfdcan1, &filter) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
                                   FDCAN_ACCEPT_IN_RX_FIFO0,
                                   FDCAN_ACCEPT_IN_RX_FIFO0,
                                   FDCAN_REJECT_REMOTE,
                                   FDCAN_REJECT_REMOTE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef BspCan_Init(void)
{
  memset(&canStatus, 0, sizeof(canStatus));

  if (BspCan_ConfigAcceptAllFilters() != HAL_OK)
  {
    canStatus.errorCount++;
    return HAL_ERROR;
  }

  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
  {
    canStatus.errorCount++;
    return HAL_ERROR;
  }

  canStatus.started = 1U;
  printf("[can] FDCAN1 started, classic CAN 500kbps, accept all frames\r\n");
  return HAL_OK;
}

HAL_StatusTypeDef BspCan_SendClassic(uint32_t id,
                                     uint8_t isExtended,
                                     const uint8_t *data,
                                     uint8_t length)
{
  FDCAN_TxHeaderTypeDef txHeader = {0};
  uint8_t txData[BSP_CAN_MAX_DATA_LEN] = {0};

  if ((canStatus.started == 0U) ||
      (length > BSP_CAN_MAX_DATA_LEN) ||
      ((length > 0U) && (data == NULL)))
  {
    canStatus.errorCount++;
    return HAL_ERROR;
  }

  if (((isExtended == 0U) && (id > 0x7FFU)) ||
      ((isExtended != 0U) && (id > 0x1FFFFFFFU)))
  {
    canStatus.errorCount++;
    return HAL_ERROR;
  }

  if (length > 0U)
  {
    memcpy(txData, data, length);
  }

  txHeader.Identifier = id;
  txHeader.IdType = (isExtended == 0U) ? FDCAN_STANDARD_ID : FDCAN_EXTENDED_ID;
  txHeader.TxFrameType = FDCAN_DATA_FRAME;
  txHeader.DataLength = BspCan_LengthToDlc(length);
  txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  txHeader.BitRateSwitch = FDCAN_BRS_OFF;
  txHeader.FDFormat = FDCAN_CLASSIC_CAN;
  txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  txHeader.MessageMarker = 0U;

  if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader, txData) != HAL_OK)
  {
    canStatus.errorCount++;
    return HAL_ERROR;
  }

  canStatus.txCount++;
  return HAL_OK;
}

void BspCan_Process(void)
{
  while ((canStatus.started != 0U) &&
         (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0U))
  {
    FDCAN_RxHeaderTypeDef rxHeader = {0};
    uint8_t rxData[BSP_CAN_MAX_DATA_LEN] = {0};
    BspCan_RxFrame frame = {0};
    uint8_t length;

    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK)
    {
      canStatus.errorCount++;
      return;
    }

    length = BspCan_DlcToLength(rxHeader.DataLength);
    canStatus.rxCount++;
    canStatus.lastRxId = rxHeader.Identifier;
    canStatus.lastRxTick = HAL_GetTick();
    canStatus.lastRxIsExtended = (rxHeader.IdType == FDCAN_EXTENDED_ID) ? 1U : 0U;
    canStatus.lastRxLength = length;
    memcpy(canStatus.lastRxData, rxData, length);

    frame.id = rxHeader.Identifier;
    frame.tick = canStatus.lastRxTick;
    frame.isExtended = canStatus.lastRxIsExtended;
    frame.length = length;
    memcpy(frame.data, rxData, length);
    BspCan_OnRxFrame(&frame);

    printf("[can] rx %s id=0x%lX len=%u data=",
           (rxHeader.IdType == FDCAN_EXTENDED_ID) ? "ext" : "std",
           (unsigned long)rxHeader.Identifier,
           length);
    for (uint8_t index = 0U; index < length; index++)
    {
      printf("%02X%s", rxData[index], (index + 1U == length) ? "" : " ");
    }
    printf("\r\n");
  }
}

BspCan_Status BspCan_GetStatus(void)
{
  return canStatus;
}
