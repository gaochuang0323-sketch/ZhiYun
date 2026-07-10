#include "unity.h"
#include <string.h>
#include "fault_command.h"
#include "stubs.h"

<<<<<<< HEAD
/* CMock mocks for all modules fault_command.c depends on */
/* waveform_engine.h: uses hand-written stub (test/support/waveform_engine.c)
   instead of CMock, to avoid conflicts with other test files */
#include "mock_voltage_sim.h"
#include "mock_bsp_can.h"
#include "mock_bsp_sram.h"
#include "mock_dac_safety.h"
#include "mock_bms_response_log.h"
#include "mock_waveform_gen.h"

=======
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
static char testOutput[4096];
static uint32_t testOutputLen;

static void testWrite(const char *text, void *context)
{
    (void)context;
    if (text) {
        uint32_t len = 0;
        while (text[len] != '\0' && (testOutputLen + len) < (sizeof(testOutput) - 1)) {
            testOutput[testOutputLen + len] = text[len];
            len++;
        }
        testOutputLen += len;
        testOutput[testOutputLen] = '\0';
    }
}

<<<<<<< HEAD
void setUp(void)
{
    testOutput[0] = '\0';
    testOutputLen = 0;
}

void tearDown(void) {}

/* ========== Basic commands ========== */
void test_help_output(void)
{
    FaultCommand_ExecuteLine("help", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "[cmd] help"));
=======
void setUp(void) { testOutput[0] = '\0'; testOutputLen = 0; }
void tearDown(void) {}

void test_help_writes_output(void)
{
    FaultCommand_ExecuteLine("help", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
}

void test_empty_line_does_nothing(void)
{
    FaultCommand_ExecuteLine("", testWrite, NULL);
    TEST_ASSERT_EQUAL_UINT32(0, testOutputLen);
}

<<<<<<< HEAD
=======
void test_null_does_nothing(void)
{
    FaultCommand_ExecuteLine(NULL, testWrite, NULL);
    TEST_ASSERT_EQUAL_UINT32(0, testOutputLen);
}

>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
void test_status_output(void)
{
    FaultCommand_ExecuteLine("status", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_normal_output(void)
{
    FaultCommand_ExecuteLine("normal", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_set_output(void)
{
    FaultCommand_ExecuteLine("set 1 2500", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_ov_output(void)
{
    FaultCommand_ExecuteLine("ov 1 4500", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_uv_output(void)
{
    FaultCommand_ExecuteLine("uv 2 1000", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_wave_stop_output(void)
{
    FaultCommand_ExecuteLine("wave stop", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_wave_square_output(void)
{
    FaultCommand_ExecuteLine("wave square 1 1000 4000 100", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

<<<<<<< HEAD
=======
void test_can_send_output(void)
{
    FaultCommand_ExecuteLine("can send", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_log_status_output(void)
{
    FaultCommand_ExecuteLine("log status", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
void test_log_clear_output(void)
{
    FaultCommand_ExecuteLine("log clear", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_log_filter_output(void)
{
    FaultCommand_ExecuteLine("log filter std 100 7FF", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

<<<<<<< HEAD
void test_clear_output(void)
{
    FaultCommand_ExecuteLine("clear", testWrite, NULL);
=======
void test_log_filter_off_output(void)
{
    FaultCommand_ExecuteLine("log filter off", testWrite, NULL);
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
    TEST_ASSERT(testOutputLen > 0);
}

void test_safe_status_output(void)
{
    FaultCommand_ExecuteLine("safe status", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

<<<<<<< HEAD
=======
void test_clear_output(void)
{
    FaultCommand_ExecuteLine("clear", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_sram_status_output(void)
{
    FaultCommand_ExecuteLine("sram status", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_sram_test_output(void)
{
    FaultCommand_ExecuteLine("sram test", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
void test_emergency_stop_output(void)
{
    FaultCommand_ExecuteLine("emergency stop", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_json_help_output(void)
{
    FaultCommand_ExecuteLine("{\"cmd\":\"help\"}", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_json_unknown_output(void)
{
    FaultCommand_ExecuteLine("{\"cmd\":\"unknown\"}", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

<<<<<<< HEAD
/* ========== Enhanced: verify specific output content ========== */
void test_help_contains_specific_commands(void)
{
    testOutputLen = 0;
    FaultCommand_ExecuteLine("help", testWrite, NULL);
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "help"));
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "status"));
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "normal"));
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "wave"));
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "cal"));
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "can"));
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "safe"));
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "sram"));
=======
void test_help_contains_help_text(void)
{
    testOutputLen = 0;
    FaultCommand_ExecuteLine("help", testWrite, NULL);
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "[cmd] help"));
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
}

void test_json_unknown_contains_error(void)
{
    testOutputLen = 0;
    FaultCommand_ExecuteLine("{\"cmd\":\"unknown\"}", testWrite, NULL);
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "unknown_cmd"));
<<<<<<< HEAD
}

void test_normal_output_contains_mv(void)
{
    testOutputLen = 0;
    FaultCommand_ExecuteLine("normal", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_log_clear_contains_log(void)
{
    testOutputLen = 0;
    FaultCommand_ExecuteLine("log clear", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_log_filter_off_contains_disabled(void)
{
    testOutputLen = 0;
    FaultCommand_ExecuteLine("log filter off", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
=======
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
}