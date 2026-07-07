/**
 * Unit tests for BSP CAN module
 */
#include "unity.h"
#include "bsp_can.h"
#include "stubs.h"

void setUp(void)
{
    FDCAN_Reset();
    /* Init resets the static state in bsp_can.c */
    BspCan_Init();
}

void tearDown(void)
{
}

/* ========== BspCan_Init ========== */
void test_Init_sets_started_flag(void)
{
    BspCan_Status status = BspCan_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(1, status.started);
}

void test_Init_returns_error_on_filter_fail(void)
{
    FDCAN_SetConfigFilterResult(HAL_ERROR);
    TEST_ASSERT_EQUAL(HAL_ERROR, BspCan_Init());
    TEST_ASSERT_EQUAL_UINT32(1, BspCan_GetStatus().errorCount);
}

void test_Init_returns_error_on_start_fail(void)
{
    FDCAN_SetStartResult(HAL_ERROR);
    TEST_ASSERT_EQUAL(HAL_ERROR, BspCan_Init());
    TEST_ASSERT_EQUAL_UINT32(1, BspCan_GetStatus().errorCount);
}

/* ========== BspCan_SendClassic ========== */
void test_SendClassic_valid_std_id_succeeds(void)
{
    uint8_t data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    HAL_StatusTypeDef status = BspCan_SendClassic(0x100, 0, data, 8);
    TEST_ASSERT_EQUAL(HAL_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1, BspCan_GetStatus().txCount);
}

void test_SendClassic_valid_ext_id_succeeds(void)
{
    uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    HAL_StatusTypeDef status = BspCan_SendClassic(0x1FFFFFFF, 1, data, 4);
    TEST_ASSERT_EQUAL(HAL_OK, status);
}

void test_SendClassic_std_id_over_11bit_returns_error(void)
{
    uint8_t data[1] = {0};
    HAL_StatusTypeDef status = BspCan_SendClassic(0x800, 0, data, 1);
    TEST_ASSERT_EQUAL(HAL_ERROR, status);
    TEST_ASSERT_EQUAL_UINT32(1, BspCan_GetStatus().errorCount);
}

void test_SendClassic_ext_id_over_29bit_returns_error(void)
{
    uint8_t data[1] = {0};
    HAL_StatusTypeDef status = BspCan_SendClassic(0x20000000, 1, data, 1);
    TEST_ASSERT_EQUAL(HAL_ERROR, status);
}

void test_SendClassic_length_over_8_returns_error(void)
{
    uint8_t data[9] = {0};
    HAL_StatusTypeDef status = BspCan_SendClassic(0x100, 0, data, 9);
    TEST_ASSERT_EQUAL(HAL_ERROR, status);
}

void test_SendClassic_null_data_with_length_returns_error(void)
{
    HAL_StatusTypeDef status = BspCan_SendClassic(0x100, 0, NULL, 5);
    TEST_ASSERT_EQUAL(HAL_ERROR, status);
}

void test_SendClassic_null_data_with_zero_length_succeeds(void)
{
    HAL_StatusTypeDef status = BspCan_SendClassic(0x100, 0, NULL, 0);
    TEST_ASSERT_EQUAL(HAL_OK, status);
}

void test_SendClassic_returns_error_on_tx_fifo_full(void)
{
    FDCAN_SetAddMessageResult(HAL_ERROR);
    uint8_t data[1] = {0};
    HAL_StatusTypeDef status = BspCan_SendClassic(0x100, 0, data, 1);
    TEST_ASSERT_EQUAL(HAL_ERROR, status);
    TEST_ASSERT_EQUAL_UINT32(1, BspCan_GetStatus().errorCount);
}

/* ========== BspCan_Process (RX) ========== */
void test_Process_drains_rx_fifo(void)
{
    uint8_t rxData[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    FDCAN_SetRxFifoFillLevel(2);
    FDCAN_SetRxData(0x200, 0, FDCAN_DLC_BYTES_8, rxData);

    BspCan_Process();

    BspCan_Status status = BspCan_GetStatus();
    TEST_ASSERT_EQUAL_UINT32(2, status.rxCount);
    TEST_ASSERT_EQUAL_UINT32(0x200, status.lastRxId);
    TEST_ASSERT_EQUAL_UINT8(8, status.lastRxLength);
    TEST_ASSERT_EQUAL_UINT8(0x11, status.lastRxData[0]);
}

void test_Process_handles_rx_error_gracefully(void)
{
    FDCAN_SetRxFifoFillLevel(1);
    FDCAN_SetGetRxMessageResult(HAL_ERROR);

    BspCan_Process();

    TEST_ASSERT_EQUAL_UINT32(1, BspCan_GetStatus().errorCount);
}

void test_Process_empty_fifo_does_nothing(void)
{
    FDCAN_SetRxFifoFillLevel(0);
    uint32_t rxCountBefore = BspCan_GetStatus().rxCount;

    BspCan_Process();

    TEST_ASSERT_EQUAL_UINT32(rxCountBefore, BspCan_GetStatus().rxCount);
}

/* ========== BspCan_GetStatus ========== */
void test_GetStatus_returns_counts(void)
{
    BspCan_Status status = BspCan_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(1, status.started);
    TEST_ASSERT_EQUAL_UINT32(0, status.txCount);
    TEST_ASSERT_EQUAL_UINT32(0, status.rxCount);
}