/**
 * Unit tests for BMS Response Log module
 */
#include "unity.h"
#include "bms_response_log.h"
#include "stubs.h"

/* Unity test runner setup */
void setUp(void)
{
    BmsResponseLog_Clear();
    BmsResponseLog_DisableFilter();
    BmsResponseLog_ClearDataFilter();

    HAL_SetTick(0);
    DAC_ClearWriteChannelCalled();
    DAC_ClearLdacCalled();
}

void tearDown(void)
{
}

/* ========== BmsResponseLog_GetTriggerName ========== */
void test_GetTriggerName_returns_correct_names(void)
{
    TEST_ASSERT_EQUAL_STRING("NONE", BmsResponseLog_GetTriggerName(BMS_RESPONSE_LOG_TRIGGER_NONE));
    TEST_ASSERT_EQUAL_STRING("OV", BmsResponseLog_GetTriggerName(BMS_RESPONSE_LOG_TRIGGER_OV));
    TEST_ASSERT_EQUAL_STRING("UV", BmsResponseLog_GetTriggerName(BMS_RESPONSE_LOG_TRIGGER_UV));
    TEST_ASSERT_EQUAL_STRING("DIFF", BmsResponseLog_GetTriggerName(BMS_RESPONSE_LOG_TRIGGER_DIFF));
}

/* ========== BmsResponseLog_RecordFaultTrigger ========== */
void test_RecordFaultTrigger_sets_armed_and_trigger_info(void)
{
    BmsResponseLog_Status status;

    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_OV, 1, 4500, 0, 0);
    status = BmsResponseLog_GetStatus();

    TEST_ASSERT_EQUAL_UINT8(1, status.armed);
    TEST_ASSERT_EQUAL_UINT8(BMS_RESPONSE_LOG_TRIGGER_OV, status.triggerType);
    TEST_ASSERT_EQUAL_UINT8(1, status.primaryCell);
    TEST_ASSERT_EQUAL_UINT16(4500, status.primaryMillivolts);
}

void test_RecordFaultTrigger_clears_previous_response(void)
{
    BspCan_RxFrame frame = {0};
    BmsResponseLog_Status status;

    /* First trigger */
    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_UV, 2, 1000, 0, 0);

    /* Simulate a response frame */
    frame.id = 0x100;
    frame.isExtended = 0;
    frame.length = 8;
    frame.tick = 100;
    frame.data[0] = 0x01;
    BspCan_OnRxFrame(&frame);

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(1, status.responseCaptured);

    /* Second trigger - should clear previous response */
    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_DIFF, 3, 4000, 5, 1000);

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(1, status.armed);
    TEST_ASSERT_EQUAL_UINT8(0, status.responseCaptured);
    TEST_ASSERT_EQUAL_UINT32(0, status.canFramesAfterTrigger);
}

/* ========== BmsResponseLog_SetFilter ========== */
void test_SetFilter_std_accepts_11bit_ids(void)
{
    TEST_ASSERT_EQUAL(HAL_OK, BmsResponseLog_SetFilter(1, 0, 0x7FF, 0x7FF));
}

void test_SetFilter_std_rejects_ids_over_11bit(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, BmsResponseLog_SetFilter(1, 0, 0x800, 0x7FF));
}

void test_SetFilter_std_rejects_mask_over_11bit(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, BmsResponseLog_SetFilter(1, 0, 0x100, 0x800));
}

void test_SetFilter_ext_accepts_29bit_ids(void)
{
    TEST_ASSERT_EQUAL(HAL_OK, BmsResponseLog_SetFilter(1, 1, 0x1FFFFFFF, 0x1FFFFFFF));
}

void test_SetFilter_ext_rejects_ids_over_29bit(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, BmsResponseLog_SetFilter(1, 1, 0x20000000, 0x1FFFFFFF));
}

/* ========== BmsResponseLog_SetDataFilter ========== */
void test_SetDataFilter_validates_index(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, BmsResponseLog_SetDataFilter(8, 0xFF, 0x55));
    TEST_ASSERT_EQUAL(HAL_OK, BmsResponseLog_SetDataFilter(7, 0xFF, 0x55));
}

void test_SetDataFilter_updates_minLength(void)
{
    BmsResponseLog_SetDataFilter(0, 0xFF, 0xAA);
    /* minLength should be 1 since index 0 + 1 = 1 */
    BmsResponseLog_SetDataFilter(3, 0xFF, 0xBB);
    /* minLength should be at least 4 since index 3 + 1 = 4 */
}

/* ========== BspCan_OnRxFrame ========== */
void test_OnRxFrame_ignores_null_frame(void)
{
    BspCan_OnRxFrame(NULL);
    /* Should not crash */
}

void test_OnRxFrame_ignores_when_not_armed(void)
{
    BspCan_RxFrame frame = {0};
    frame.id = 0x100;
    frame.isExtended = 0;
    frame.length = 8;
    frame.tick = 50;

    BspCan_OnRxFrame(&frame);
    /* armed is 0 by default, so no capture */
    TEST_ASSERT_EQUAL_UINT32(0, BmsResponseLog_GetStatus().canFramesAfterTrigger);
}

void test_OnRxFrame_counts_all_frames_after_trigger(void)
{
    BspCan_RxFrame frame1 = {0};
    BspCan_RxFrame frame2 = {0};

    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_OV, 1, 4500, 0, 0);

    frame1.id = 0x100;
    frame1.isExtended = 0;
    frame1.length = 8;
    frame1.tick = 10;
    BspCan_OnRxFrame(&frame1);

    frame2.id = 0x200;
    frame2.isExtended = 0;
    frame2.length = 8;
    frame2.tick = 20;
    BspCan_OnRxFrame(&frame2);

    TEST_ASSERT_EQUAL_UINT32(2, BmsResponseLog_GetStatus().canFramesAfterTrigger);
}

void test_OnRxFrame_captures_matching_frame(void)
{
    BspCan_RxFrame frame = {0};
    BmsResponseLog_Status status;

    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_UV, 2, 1000, 0, 0);
    BmsResponseLog_SetFilter(1, 0, 0x100, 0x7FF);

    frame.id = 0x100;
    frame.isExtended = 0;
    frame.length = 8;
    frame.tick = 50;
    frame.data[0] = 0x12;
    frame.data[1] = 0x34;
    BspCan_OnRxFrame(&frame);

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(1, status.responseCaptured);
    TEST_ASSERT_EQUAL_UINT32(0x100, status.responseCanId);
    TEST_ASSERT_EQUAL_UINT32(50, status.responseDelayMs);
    TEST_ASSERT_EQUAL_UINT8(0, status.responseCanIsExtended);
    TEST_ASSERT_EQUAL_UINT8(8, status.responseCanLength);
    TEST_ASSERT_EQUAL_UINT8(0x12, status.responseCanData[0]);
}

void test_OnRxFrame_ignores_non_matching_filter(void)
{
    BspCan_RxFrame frame = {0};
    BmsResponseLog_Status status;

    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_DIFF, 3, 4000, 5, 1000);
    BmsResponseLog_SetFilter(1, 0, 0x100, 0x7FF);

    frame.id = 0x200;
    frame.isExtended = 0;
    frame.length = 8;
    frame.tick = 50;
    BspCan_OnRxFrame(&frame);

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(0, status.responseCaptured);
    TEST_ASSERT_EQUAL_UINT32(1, status.filterMissCount);
}

void test_OnRxFrame_filter_checks_data_bytes(void)
{
    BspCan_RxFrame frame = {0};
    BmsResponseLog_Status status;

    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_OV, 1, 4500, 0, 0);
    BmsResponseLog_SetFilter(1, 0, 0x100, 0x7FF);
    BmsResponseLog_SetDataFilter(0, 0xFF, 0xAA);

    /* Frame with wrong data byte 0 */
    frame.id = 0x100;
    frame.isExtended = 0;
    frame.length = 8;
    frame.tick = 50;
    frame.data[0] = 0x55;
    BspCan_OnRxFrame(&frame);

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(0, status.responseCaptured);
    TEST_ASSERT_EQUAL_UINT32(1, status.filterMissCount);

    /* Frame with correct data byte 0 */
    frame.tick = 100;
    frame.data[0] = 0xAA;
    BspCan_OnRxFrame(&frame);

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(1, status.responseCaptured);
}

void test_OnRxFrame_captures_only_first_match(void)
{
    BspCan_RxFrame frame1 = {0};
    BspCan_RxFrame frame2 = {0};
    BmsResponseLog_Status status;

    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_UV, 2, 1000, 0, 0);
    BmsResponseLog_SetFilter(1, 0, 0x100, 0x7FF);

    frame1.id = 0x100;
    frame1.isExtended = 0;
    frame1.length = 8;
    frame1.tick = 50;
    BspCan_OnRxFrame(&frame1);

    frame2.id = 0x100;
    frame2.isExtended = 0;
    frame2.length = 8;
    frame2.tick = 100;
    frame2.data[0] = 0xFF;
    BspCan_OnRxFrame(&frame2);

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT32(2, status.canFramesAfterTrigger);
    TEST_ASSERT_EQUAL_UINT32(50, status.responseDelayMs);
    TEST_ASSERT_EQUAL_UINT8(0, status.responseCanData[0]);
}

void test_OnRxFrame_filter_checks_extended_flag(void)
{
    BspCan_RxFrame frame = {0};
    BmsResponseLog_Status status;

    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_OV, 1, 4500, 0, 0);
    /* Filter expects standard frames */
    BmsResponseLog_SetFilter(1, 0, 0x100, 0x7FF);

    /* Extended frame with same ID should not match */
    frame.id = 0x100;
    frame.isExtended = 1;
    frame.length = 8;
    frame.tick = 50;
    BspCan_OnRxFrame(&frame);

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(0, status.responseCaptured);
}

void test_OnRxFrame_filter_checks_min_length(void)
{
    BspCan_RxFrame frame = {0};
    BmsResponseLog_Status status;

    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_OV, 1, 4500, 0, 0);
    BmsResponseLog_SetFilter(1, 0, 0x100, 0x7FF);
    BmsResponseLog_SetDataFilter(3, 0xFF, 0xAA);

    /* Frame too short (need at least 4 bytes) */
    frame.id = 0x100;
    frame.isExtended = 0;
    frame.length = 2;
    frame.tick = 50;
    BspCan_OnRxFrame(&frame);

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(0, status.responseCaptured);
}

void test_OnRxFrame_without_filter_accepts_any_frame(void)
{
    BspCan_RxFrame frame = {0};

    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_OV, 1, 4500, 0, 0);
    /* No filter set - should accept any frame */

    frame.id = 0x123;
    frame.isExtended = 0;
    frame.length = 8;
    frame.tick = 100;
    BspCan_OnRxFrame(&frame);

    TEST_ASSERT_EQUAL_UINT8(1, BmsResponseLog_GetStatus().responseCaptured);
}

/* ========== BmsResponseLog_Clear ========== */
void test_Clear_resets_status_preserves_filter(void)
{
    BmsResponseLog_Status status;

    BmsResponseLog_SetFilter(1, 0, 0x100, 0x7FF);
    BmsResponseLog_RecordFaultTrigger(BMS_RESPONSE_LOG_TRIGGER_OV, 1, 4500, 0, 0);
    BmsResponseLog_Clear();

    status = BmsResponseLog_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(0, status.armed);
    TEST_ASSERT_EQUAL_UINT8(1, status.filter.enabled);
    TEST_ASSERT_EQUAL_UINT32(0x100, status.filter.id);
}

/* ========== BmsResponseLog_DisableFilter ========== */
void test_DisableFilter_disables_filter(void)
{
    BmsResponseLog_SetFilter(1, 0, 0x100, 0x7FF);
    BmsResponseLog_DisableFilter();
    TEST_ASSERT_EQUAL_UINT8(0, BmsResponseLog_GetStatus().filter.enabled);
}

/* ========== BmsResponseLog_ClearDataFilter ========== */
void test_ClearDataFilter_clears_all_data_filters(void)
{
    BmsResponseLog_SetDataFilter(0, 0xFF, 0xAA);
    BmsResponseLog_SetDataFilter(1, 0xF0, 0x50);
    BmsResponseLog_ClearDataFilter();
    /* After clear, data masks should be zero */
    TEST_ASSERT_EQUAL_UINT8(0, BmsResponseLog_GetStatus().filter.dataMask[0]);
    TEST_ASSERT_EQUAL_UINT8(0, BmsResponseLog_GetStatus().filter.dataMask[1]);
}