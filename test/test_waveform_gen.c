/**
 * Unit tests for Waveform Generator module
 */
#include "unity.h"
#include "waveform_gen.h"
#include "stubs.h"
#include "voltage_sim.h"

void setUp(void)
{
    WaveformGen_Stop();
    HAL_SetTick(0);
    DAC_SetWriteResult(HAL_OK);
    DAC_ClearWriteChannelCalled();
    DAC_ClearLdacCalled();
    VoltageSim_Init(3200);
}

void tearDown(void)
{
}

/* ========== WaveformGen_GetTypeName ========== */
void test_GetTypeName_returns_correct_names(void)
{
    TEST_ASSERT_EQUAL_STRING("none", WaveformGen_GetTypeName(WAVEFORM_GEN_NONE));
    TEST_ASSERT_EQUAL_STRING("square", WaveformGen_GetTypeName(WAVEFORM_GEN_SQUARE));
    TEST_ASSERT_EQUAL_STRING("sine", WaveformGen_GetTypeName(WAVEFORM_GEN_SINE));
    TEST_ASSERT_EQUAL_STRING("sine_all", WaveformGen_GetTypeName(WAVEFORM_GEN_SINE_ALL));
}

/* ========== WaveformGen_StartSquare ========== */
void test_StartSquare_valid_params_succeeds(void)
{
    HAL_StatusTypeDef status;
    WaveformGen_Status ws;

    status = WaveformGen_StartSquare(1, 1000, 4000, 100);
    TEST_ASSERT_EQUAL(HAL_OK, status);

    ws = WaveformGen_GetStatus();
    TEST_ASSERT_EQUAL_UINT8(1, ws.active);
    TEST_ASSERT_EQUAL(WAVEFORM_GEN_SQUARE, ws.type);
    TEST_ASSERT_EQUAL_UINT8(1, ws.cell);
    TEST_ASSERT_EQUAL_UINT16(1000, ws.lowMillivolts);
    TEST_ASSERT_EQUAL_UINT16(4000, ws.highMillivolts);
    TEST_ASSERT_EQUAL_UINT32(100, ws.periodMs);
}

void test_StartSquare_invalid_cell_returns_error(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, WaveformGen_StartSquare(0, 1000, 4000, 100));
    TEST_ASSERT_EQUAL(HAL_ERROR, WaveformGen_StartSquare(14, 1000, 4000, 100));
}

void test_StartSquare_low_equals_high_returns_error(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, WaveformGen_StartSquare(1, 3000, 3000, 100));
}

void test_StartSquare_low_above_high_returns_error(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, WaveformGen_StartSquare(1, 4000, 3000, 100));
}

void test_StartSquare_below_min_period_returns_error(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, WaveformGen_StartSquare(1, 1000, 4000, 1));
}

void test_StartSquare_invalid_voltage_returns_error(void)
{
    /* Below MIN_MV */
    TEST_ASSERT_EQUAL(HAL_ERROR, WaveformGen_StartSquare(1, 100, 4000, 100));
    /* Above MAX_MV */
    TEST_ASSERT_EQUAL(HAL_ERROR, WaveformGen_StartSquare(1, 1000, 6000, 100));
}

/* ========== WaveformGen_StartSine ========== */
void test_StartSine_valid_params_succeeds(void)
{
    HAL_StatusTypeDef status;
    /* offset=3000, amplitude=1500, max=3000+1500=4500 <= MAX_MV(5000) */
    /* min=3000-1500=1500 >= MIN_MV(500) */
    status = WaveformGen_StartSine(1, 3000, 1500, 100);
    TEST_ASSERT_EQUAL(HAL_OK, status);
    TEST_ASSERT_EQUAL(WAVEFORM_GEN_SINE, WaveformGen_GetStatus().type);
}

void test_StartSine_zero_amplitude_returns_error(void)
{
    TEST_ASSERT_EQUAL(HAL_ERROR, WaveformGen_StartSine(1, 3200, 0, 100));
}

void test_StartSine_out_of_range_returns_error(void)
{
    /* amplitude 2500 + offset 3000 = 5500 > MAX_MV(5000) */
    TEST_ASSERT_EQUAL(HAL_ERROR, WaveformGen_StartSine(1, 3000, 2500, 100));
}

/* ========== WaveformGen_StartSineAll ========== */
void test_StartSineAll_valid_params_succeeds(void)
{
    HAL_StatusTypeDef status;

    status = WaveformGen_StartSineAll(3200, 1000, 200);
    TEST_ASSERT_EQUAL(HAL_OK, status);
    TEST_ASSERT_EQUAL(WAVEFORM_GEN_SINE_ALL, WaveformGen_GetStatus().type);
    TEST_ASSERT_EQUAL_UINT8(0, WaveformGen_GetStatus().cell);
}

/* ========== WaveformGen_Stop ========== */
void test_Stop_deactivates_waveform(void)
{
    WaveformGen_StartSquare(1, 1000, 4000, 100);
    WaveformGen_Stop();
    TEST_ASSERT_EQUAL_UINT8(0, WaveformGen_GetStatus().active);
    TEST_ASSERT_EQUAL(WAVEFORM_GEN_NONE, WaveformGen_GetStatus().type);
}

/* ========== WaveformGen_Process - Square ========== */
void test_Process_square_outputs_low_then_high(void)
{
    uint32_t startTick = 1000;

    HAL_SetTick(startTick);
    WaveformGen_StartSquare(1, 1000, 4000, 100);

    /* At t=startTick, phase = (1000-1000)%100 = 0 => low */
    HAL_SetTick(startTick);
    WaveformGen_Process(startTick);
    TEST_ASSERT_EQUAL_UINT16(1000, WaveformGen_GetStatus().lastMillivolts);

    /* At t=startTick+50, phase = 50, 50 >= 50 => high */
    HAL_SetTick(startTick + 50);
    WaveformGen_Process(startTick + 50);
    TEST_ASSERT_EQUAL_UINT16(4000, WaveformGen_GetStatus().lastMillivolts);

    /* At t=startTick+100, phase = (1100-1000)%100 = 0 => low */
    HAL_SetTick(startTick + 100);
    WaveformGen_Process(startTick + 100);
    TEST_ASSERT_EQUAL_UINT16(1000, WaveformGen_GetStatus().lastMillivolts);
}

void test_Process_skips_if_not_active(void)
{
    WaveformGen_Process(100);
    /* No crash, no output */
}

void test_Process_inactive_returns_ok(void)
{
    /* active=0, type=NONE by default */
    TEST_ASSERT_EQUAL(HAL_OK, WaveformGen_Process(100));
}

/* ========== WaveformGen_Process - Sine ========== */
void test_Process_sine_outputs_varying_voltage(void)
{
    uint16_t v0, v1, v2;
    uint32_t startTick = 1000;

    /* offset=3000, amplitude=1500, period=64ms */
    HAL_SetTick(startTick);
    WaveformGen_StartSine(2, 3000, 1500, 64);

    /* At t=startTick, phase = 0, sine(0)=0 => output = 3000 */
    HAL_SetTick(startTick);
    WaveformGen_Process(startTick);
    v0 = WaveformGen_GetStatus().lastMillivolts;
    TEST_ASSERT_EQUAL_UINT16(3000, v0);

    /* At t=startTick+1, output should have changed */
    HAL_SetTick(startTick + 1);
    WaveformGen_Process(startTick + 1);
    v1 = WaveformGen_GetStatus().lastMillivolts;

    /* At t=startTick+16 (quarter period), output should be near max */
    HAL_SetTick(startTick + 16);
    WaveformGen_Process(startTick + 16);
    v2 = WaveformGen_GetStatus().lastMillivolts;

    TEST_ASSERT(v2 > v0); /* increasing from start */
}

/* ========== WaveformGen_Process - Sine All ========== */
void test_Process_sine_all_updates_all_cells(void)
{
    uint8_t cell;
    uint32_t startTick = 1000;

    /* offset=3000, amplitude=1000, max=4000 <= MAX_MV(5000) */
    HAL_SetTick(startTick);
    WaveformGen_StartSineAll(3000, 1000, 100);
    HAL_SetTick(startTick + 50);
    WaveformGen_Process(startTick + 50);

    /* All cells should have the same voltage */
    for (cell = 1; cell <= VOLTAGE_SIM_CELL_COUNT; cell++)
    {
        TEST_ASSERT_EQUAL_UINT16(WaveformGen_GetStatus().lastMillivolts,
                                 VoltageSim_GetLastCellVoltageMv(cell));
    }
}
