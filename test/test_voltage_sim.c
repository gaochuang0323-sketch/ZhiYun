/**
 * Unit tests for Voltage Simulator module
 */
#include "unity.h"
#include "voltage_sim.h"
#include "stubs.h"

void setUp(void)
{
    HAL_SetTick(0);
    DAC_SetWriteResult(HAL_OK);
    DAC_ClearWriteChannelCalled();
    DAC_ClearLdacCalled();
}

void tearDown(void)
{
}

/* ========== VoltageSim_MillivoltsToDacCode ========== */
void test_MillivoltsToDacCode_returns_0_for_0mv(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, VoltageSim_MillivoltsToDacCode(0));
}

void test_MillivoltsToDacCode_returns_0xFFFF_for_max_voltage(void)
{
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, VoltageSim_MillivoltsToDacCode(VOLTAGE_SIM_MAX_MV));
}

void test_MillivoltsToDacCode_returns_0xFFFF_for_above_max(void)
{
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, VoltageSim_MillivoltsToDacCode(VOLTAGE_SIM_MAX_MV + 1));
}

void test_MillivoltsToDacCode_returns_midpoint(void)
{
    /* 2500mV should map to approximately 0x7FFF (half of 0xFFFF) */
    uint16_t code = VoltageSim_MillivoltsToDacCode(VOLTAGE_SIM_MAX_MV / 2);
    TEST_ASSERT_UINT16_WITHIN(2, 0x7FFF, code);
}

void test_MillivoltsToDacCode_min_voltage(void)
{
    /* 500mV is MIN_MV, should map to ~0x1999 */
    uint16_t code = VoltageSim_MillivoltsToDacCode(VOLTAGE_SIM_MIN_MV);
    TEST_ASSERT(code > 0);
}

/* ========== VoltageSim_Init ========== */
void test_Init_sets_all_cells_to_default(void)
{
    HAL_StatusTypeDef status;
    uint8_t cell;

    status = VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(HAL_OK, status);

    for (cell = 1; cell <= VOLTAGE_SIM_CELL_COUNT; cell++)
    {
        TEST_ASSERT_EQUAL_UINT16(3200, VoltageSim_GetLastCellVoltageMv(cell));
    }
    /* LDAC should have been called to update all outputs */
    TEST_ASSERT_EQUAL_UINT8(1, DAC_GetLdacCalled());
}

/* ========== VoltageSim_SetCellVoltageMv ========== */
void test_SetCellVoltageMv_invalid_cell_returns_error(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_SetCellVoltageMv(0, 3200));
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_SetCellVoltageMv(14, 3200));
}

void test_SetCellVoltageMv_invalid_voltage_returns_error(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_SetCellVoltageMv(1, 100));  /* below MIN */
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_SetCellVoltageMv(1, 6000)); /* above MAX */
}

void test_SetCellVoltageMv_valid_call_succeeds(void)
{
    HAL_StatusTypeDef status;

    VoltageSim_Init(3200);
    status = VoltageSim_SetCellVoltageMv(1, 2500);

    TEST_ASSERT_EQUAL(HAL_OK, status);
    TEST_ASSERT_EQUAL_UINT16(2500, VoltageSim_GetLastCellVoltageMv(1));
}

void test_SetCellVoltageMv_clears_existing_fault(void)
{
    VoltageSim_Init(3200);
    VoltageSim_InjectCellOverVoltage(1, 4500);
    /* Fault should be active */
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_OVER_VOLTAGE, VoltageSim_GetCellFaultInfo(1).type);

    /* Setting voltage should clear the fault */
    VoltageSim_SetCellVoltageMv(1, 3200);
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_NONE, VoltageSim_GetCellFaultInfo(1).type);
}

/* ========== VoltageSim_SetAllCellsVoltageMv ========== */
void test_SetAllCellsVoltageMv_sets_all_cells(void)
{
    uint8_t cell;

    VoltageSim_SetAllCellsVoltageMv(3000);

    for (cell = 1; cell <= VOLTAGE_SIM_CELL_COUNT; cell++)
    {
        TEST_ASSERT_EQUAL_UINT16(3000, VoltageSim_GetLastCellVoltageMv(cell));
    }
}

void test_SetAllCellsVoltageMv_rejects_invalid_voltage(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_SetAllCellsVoltageMv(100));
}

void test_SetAllCellsVoltageMv_clears_all_faults(void)
{
    VoltageSim_Init(3200);
    VoltageSim_InjectCellOverVoltage(1, 4500);
    VoltageSim_InjectCellUnderVoltage(3, 1000);

    VoltageSim_SetAllCellsVoltageMv(3000);

    TEST_ASSERT_EQUAL(0, VoltageSim_GetActiveFaultCount());
}

/* ========== VoltageSim_InjectCellOverVoltage ========== */
void test_InjectCellOverVoltage_succeeds(void)
{
    VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(HAL_OK, VoltageSim_InjectCellOverVoltage(1, 4500));
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_OVER_VOLTAGE, VoltageSim_GetCellFaultInfo(1).type);
    TEST_ASSERT_EQUAL_UINT16(4500, VoltageSim_GetCellFaultInfo(1).targetMillivolts);
    TEST_ASSERT_EQUAL_UINT16(3200, VoltageSim_GetCellFaultInfo(1).restoreMillivolts);
}

void test_InjectCellOverVoltage_rejects_lower_target(void)
{
    VoltageSim_Init(3200);
    /* Target must be greater than restore voltage for OV */
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_InjectCellOverVoltage(1, 3000));
}

void test_InjectCellOverVoltage_rejects_same_target(void)
{
    VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_InjectCellOverVoltage(1, 3200));
}

/* ========== VoltageSim_InjectCellUnderVoltage ========== */
void test_InjectCellUnderVoltage_succeeds(void)
{
    VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(HAL_OK, VoltageSim_InjectCellUnderVoltage(1, 1000));
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_UNDER_VOLTAGE, VoltageSim_GetCellFaultInfo(1).type);
    TEST_ASSERT_EQUAL_UINT16(1000, VoltageSim_GetCellFaultInfo(1).targetMillivolts);
}

void test_InjectCellUnderVoltage_rejects_higher_target(void)
{
    VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_InjectCellUnderVoltage(1, 3500));
}

/* ========== VoltageSim_ClearCellFault ========== */
void test_ClearCellFault_clears_fault_and_restores(void)
{
    VoltageSim_Init(3200);
    VoltageSim_InjectCellOverVoltage(1, 4500);

    TEST_ASSERT_EQUAL(HAL_OK, VoltageSim_ClearCellFault(1));
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_NONE, VoltageSim_GetCellFaultInfo(1).type);
    TEST_ASSERT_EQUAL_UINT16(3200, VoltageSim_GetLastCellVoltageMv(1));
}

void test_ClearCellFault_invalid_cell_returns_error(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_ClearCellFault(0));
}

void test_ClearCellFault_no_fault_returns_ok(void)
{
    VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(HAL_OK, VoltageSim_ClearCellFault(1));
}

/* ========== VoltageSim_ClearAllFaults ========== */
void test_ClearAllFaults_clears_multiple_faults(void)
{
    VoltageSim_Init(3200);
    VoltageSim_InjectCellOverVoltage(1, 4500);
    VoltageSim_InjectCellUnderVoltage(5, 1000);

    TEST_ASSERT_EQUAL(2, VoltageSim_GetActiveFaultCount());
    TEST_ASSERT_EQUAL(HAL_OK, VoltageSim_ClearAllFaults());
    TEST_ASSERT_EQUAL(0, VoltageSim_GetActiveFaultCount());
}

/* ========== VoltageSim_GetActiveFaultCount ========== */
void test_GetActiveFaultCount_returns_zero_initially(void)
{
    VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(0, VoltageSim_GetActiveFaultCount());
}

void test_GetActiveFaultCount_counts_faults(void)
{
    VoltageSim_Init(3200);
    VoltageSim_InjectCellOverVoltage(1, 4500);
    VoltageSim_InjectCellUnderVoltage(5, 1000);
    TEST_ASSERT_EQUAL(2, VoltageSim_GetActiveFaultCount());
}

/* ========== VoltageSim_GetFaultTypeName ========== */
void test_GetFaultTypeName_returns_correct_names(void)
{
    TEST_ASSERT_EQUAL_STRING("NONE", VoltageSim_GetFaultTypeName(VOLTAGE_SIM_FAULT_NONE));
    TEST_ASSERT_EQUAL_STRING("OV", VoltageSim_GetFaultTypeName(VOLTAGE_SIM_FAULT_OVER_VOLTAGE));
    TEST_ASSERT_EQUAL_STRING("UV", VoltageSim_GetFaultTypeName(VOLTAGE_SIM_FAULT_UNDER_VOLTAGE));
}

/* ========== VoltageSim_InjectCellOverVoltageFor / timed faults ========== */
void test_InjectCellOverVoltageFor_sets_duration(void)
{
    VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(HAL_OK, VoltageSim_InjectCellOverVoltageFor(1, 4500, 500));
    TEST_ASSERT_EQUAL_UINT32(500, VoltageSim_GetCellFaultInfo(1).durationMs);
}

/* ========== VoltageSim_Process with timed fault ========== */
void test_Process_restores_after_duration_expires(void)
{
    VoltageSim_Init(3200);

    /* Inject OV fault with 100ms duration */
    HAL_SetTick(0);
    VoltageSim_InjectCellOverVoltageFor(1, 4500, 100);

    /* Process at t=0 - should still be at target */
    HAL_SetTick(0);
    VoltageSim_Process(0);
    TEST_ASSERT_EQUAL_UINT16(4500, VoltageSim_GetLastCellVoltageMv(1));

    /* Process at t=50 - still within duration */
    HAL_SetTick(50);
    VoltageSim_Process(50);
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_OVER_VOLTAGE, VoltageSim_GetCellFaultInfo(1).type);

    /* Process at t=150 - after duration, faultRestoring flag set */
    HAL_SetTick(150);
    VoltageSim_Process(150);
    /* Fault should still be active because restore write happens next cycle */
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_OVER_VOLTAGE, VoltageSim_GetCellFaultInfo(1).type);

    /* Process at t=151 - should write restore voltage and clear fault */
    HAL_SetTick(151);
    VoltageSim_Process(151);
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_NONE, VoltageSim_GetCellFaultInfo(1).type);
    TEST_ASSERT_EQUAL_UINT16(3200, VoltageSim_GetLastCellVoltageMv(1));
}

/* ========== VoltageSim_Calibration ========== */
void test_SetCellCalibrationOffset_valid(void)
{
    VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(HAL_OK, VoltageSim_SetCellCalibrationOffsetMv(1, 50));
    TEST_ASSERT_EQUAL(50, VoltageSim_GetCellCalibrationOffsetMv(1));
}

void test_SetCellCalibrationOffset_exceeds_limit(void)
{
    VoltageSim_Init(3200);
    TEST_ASSERT_EQUAL(HAL_ERROR, VoltageSim_SetCellCalibrationOffsetMv(1, 600));
}

void test_ClearCellCalibrationOffset_clears(void)
{
    VoltageSim_Init(3200);
    VoltageSim_SetCellCalibrationOffsetMv(1, 50);
    TEST_ASSERT_EQUAL(HAL_OK, VoltageSim_ClearCellCalibrationOffset(1));
    TEST_ASSERT_EQUAL(0, VoltageSim_GetCellCalibrationOffsetMv(1));
}

void test_ClearAllCalibrationOffsets_clears_all(void)
{
    VoltageSim_Init(3200);
    VoltageSim_SetCellCalibrationOffsetMv(1, 50);
    VoltageSim_SetCellCalibrationOffsetMv(2, -30);
    VoltageSim_ClearAllCalibrationOffsets();
    TEST_ASSERT_EQUAL(0, VoltageSim_GetCellCalibrationOffsetMv(1));
    TEST_ASSERT_EQUAL(0, VoltageSim_GetCellCalibrationOffsetMv(2));
}

void test_GetCellCalibrationOffset_invalid_cell(void)
{
    TEST_ASSERT_EQUAL(0, VoltageSim_GetCellCalibrationOffsetMv(0));
    TEST_ASSERT_EQUAL(0, VoltageSim_GetCellCalibrationOffsetMv(14));
}

/* ========== VoltageSim_GetLastCellVoltageMv ========== */
void test_GetLastCellVoltage_invalid_cell(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, VoltageSim_GetLastCellVoltageMv(0));
}

/* ========== VoltageSim_GetCellFaultInfo ========== */
void test_GetCellFaultInfo_invalid_cell_returns_empty(void)
{
    VoltageSim_CellFaultInfo info = VoltageSim_GetCellFaultInfo(0);
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_NONE, info.type);
}

void test_GetCellFaultInfo_valid_cell_returns_fault(void)
{
    VoltageSim_Init(3200);
    VoltageSim_InjectCellOverVoltage(1, 4500);
    VoltageSim_CellFaultInfo info = VoltageSim_GetCellFaultInfo(1);
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_OVER_VOLTAGE, info.type);
}

/* ========== VoltageSim_InjectVoltageDifference ========== */
void test_InjectVoltageDifference_sets_both_cells(void)
{
    VoltageSim_Init(3200);
    HAL_StatusTypeDef status = VoltageSim_InjectVoltageDifference(1, 4500, 2, 1000, 0, 0);
    TEST_ASSERT_EQUAL(HAL_OK, status);
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_OVER_VOLTAGE, VoltageSim_GetCellFaultInfo(1).type);
    TEST_ASSERT_EQUAL(VOLTAGE_SIM_FAULT_UNDER_VOLTAGE, VoltageSim_GetCellFaultInfo(2).type);
}