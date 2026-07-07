/**
 * Unit tests for DAC Safety module
 *
 * Tests focus on core functionality independent of timing dependencies.
 * Uses CMock mocks for waveform_gen and voltage_sim.
 */
#include "unity.h"
#include "dac_safety.h"
#include "stubs.h"

#include "mock_waveform_gen.h"
#include "mock_voltage_sim.h"

#define TEST_SAFE_MV 3000U

void setUp(void)
{
    DAC_SetAlarmPinValue(1);
    DAC_SetWriteResult(HAL_OK);
    DAC_ClearWriteChannelCalled();
    DAC_ClearLdacCalled();
    HAL_SetTick(100U);
}

void tearDown(void)
{
}

/* ========== DacSafety_Init ========== */
void test_Init_sets_safe_millivolts(void)
{
    DacSafety_Init(TEST_SAFE_MV);
    DacSafety_Status status = DacSafety_GetStatus();
    TEST_ASSERT_EQUAL_UINT16(TEST_SAFE_MV, status.safeMillivolts);
    TEST_ASSERT_EQUAL_UINT8(0, status.emergencyActive);
}

void test_Init_uses_default_for_invalid_voltage(void)
{
    DacSafety_Init(100);
    DacSafety_Status status = DacSafety_GetStatus();
    TEST_ASSERT_EQUAL_UINT16(DAC_SAFETY_DEFAULT_SAFE_MV, status.safeMillivolts);
}

void test_Init_reads_alarm_pin_initially(void)
{
    DAC_SetAlarmPinValue(0);
    DacSafety_Init(TEST_SAFE_MV);
    DacSafety_Status status = DacSafety_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(0, status.alarmPinState);
    TEST_ASSERT_EQUAL_UINT8(1, status.alarmActive);
}

/* ========== DacSafety_IsOutputAllowed ========== */
void test_IsOutputAllowed_returns_true_by_default(void)
{
    DacSafety_Init(TEST_SAFE_MV);
    TEST_ASSERT_EQUAL_UINT8(1, DacSafety_IsOutputAllowed());
}

void test_IsOutputAllowed_returns_false_after_emergency_stop(void)
{
    DacSafety_Init(TEST_SAFE_MV);
    WaveformGen_Stop_Expect();
    VoltageSim_ClearAllFaults_ExpectAndReturn(HAL_OK);
    VoltageSim_SetAllCellsVoltageMv_ExpectAndReturn(TEST_SAFE_MV, HAL_OK);

    DacSafety_EmergencyStop(TEST_SAFE_MV);
    TEST_ASSERT_EQUAL_UINT8(0, DacSafety_IsOutputAllowed());
}

void test_IsOutputAllowed_returns_true_after_release(void)
{
    DacSafety_Init(TEST_SAFE_MV);
    WaveformGen_Stop_Expect();
    VoltageSim_ClearAllFaults_ExpectAndReturn(HAL_OK);
    VoltageSim_SetAllCellsVoltageMv_ExpectAndReturn(TEST_SAFE_MV, HAL_OK);

    DacSafety_EmergencyStop(TEST_SAFE_MV);
    DacSafety_Release();
    TEST_ASSERT_EQUAL_UINT8(1, DacSafety_IsOutputAllowed());
}

/* ========== DacSafety_EmergencyStop ========== */
void test_EmergencyStop_sets_emergency_active(void)
{
    DacSafety_Init(TEST_SAFE_MV);

    WaveformGen_Stop_Expect();
    VoltageSim_ClearAllFaults_ExpectAndReturn(HAL_OK);
    VoltageSim_SetAllCellsVoltageMv_ExpectAndReturn(2000, HAL_OK);

    HAL_StatusTypeDef status = DacSafety_EmergencyStop(2000);
    TEST_ASSERT_EQUAL(HAL_OK, status);

    DacSafety_Status s = DacSafety_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(1, s.emergencyActive);
    TEST_ASSERT_EQUAL_UINT16(2000, s.safeMillivolts);
}

void test_EmergencyStop_rejects_invalid_voltage(void)
{
    DacSafety_Init(TEST_SAFE_MV);
    TEST_ASSERT_EQUAL(HAL_ERROR, DacSafety_EmergencyStop(100));
}

void test_EmergencyStop_returns_error_on_voltage_failure(void)
{
    DacSafety_Init(TEST_SAFE_MV);

    WaveformGen_Stop_Expect();
    VoltageSim_ClearAllFaults_ExpectAndReturn(HAL_OK);
    VoltageSim_SetAllCellsVoltageMv_ExpectAndReturn(2000, HAL_ERROR);

    HAL_StatusTypeDef status = DacSafety_EmergencyStop(2000);
    TEST_ASSERT_EQUAL(HAL_ERROR, status);
}

/* ========== DacSafety_Process (basic) ========== */
void test_Process_skips_if_not_enough_time_elapsed(void)
{
    DacSafety_Init(TEST_SAFE_MV);
    DacSafety_Process(10U);
    DacSafety_Status s = DacSafety_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(0, s.alarmActive);
}

void test_Process_detects_alarm_and_does_not_hang(void)
{
    DacSafety_Init(TEST_SAFE_MV);

    DAC_SetAlarmPinValue(0);

    /* Set tick far enough from Init time to trigger poll */
    HAL_SetTick(200U);

    WaveformGen_Stop_Expect();
    VoltageSim_ClearAllFaults_ExpectAndReturn(HAL_OK);
    VoltageSim_SetAllCellsVoltageMv_ExpectAndReturn(TEST_SAFE_MV, HAL_OK);

    DacSafety_Process(200U);

    DacSafety_Status s = DacSafety_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(0, s.alarmPinState);
    TEST_ASSERT_EQUAL_UINT8(1, s.alarmActive);
    TEST_ASSERT_EQUAL_UINT8(1, s.alarmLatched);
    TEST_ASSERT_EQUAL_UINT32(1, s.alarmCount);
    TEST_ASSERT_EQUAL_UINT8(0, DacSafety_IsOutputAllowed());
}

/* ========== DacSafety_Release ========== */
void test_Release_clears_emergency_and_alarm_latch(void)
{
    DacSafety_Init(TEST_SAFE_MV);

    WaveformGen_Stop_Expect();
    VoltageSim_ClearAllFaults_ExpectAndReturn(HAL_OK);
    VoltageSim_SetAllCellsVoltageMv_ExpectAndReturn(TEST_SAFE_MV, HAL_OK);
    DacSafety_EmergencyStop(TEST_SAFE_MV);

    DacSafety_Release();

    DacSafety_Status s = DacSafety_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(0, s.emergencyActive);
    TEST_ASSERT_EQUAL_UINT8(0, s.alarmLatched);
}

/* ========== DacSafety_GetStatus ========== */
void test_GetStatus_returns_latest_state(void)
{
    /* Init with a fresh start */
    DacSafety_Init(TEST_SAFE_MV);

    DacSafety_Status s = DacSafety_GetStatus();
    TEST_ASSERT_EQUAL_UINT16(TEST_SAFE_MV, s.safeMillivolts);
    TEST_ASSERT_EQUAL_UINT8(0, s.emergencyActive);
}
