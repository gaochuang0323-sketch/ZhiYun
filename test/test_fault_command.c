#include "unity.h"
#include <string.h>
#include "fault_command.h"
#include "stubs.h"

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

void setUp(void) { testOutput[0] = '\0'; testOutputLen = 0; }
void tearDown(void) {}

void test_help_writes_output(void)
{
    FaultCommand_ExecuteLine("help", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_empty_line_does_nothing(void)
{
    FaultCommand_ExecuteLine("", testWrite, NULL);
    TEST_ASSERT_EQUAL_UINT32(0, testOutputLen);
}

void test_null_does_nothing(void)
{
    FaultCommand_ExecuteLine(NULL, testWrite, NULL);
    TEST_ASSERT_EQUAL_UINT32(0, testOutputLen);
}

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

void test_log_filter_off_output(void)
{
    FaultCommand_ExecuteLine("log filter off", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

void test_safe_status_output(void)
{
    FaultCommand_ExecuteLine("safe status", testWrite, NULL);
    TEST_ASSERT(testOutputLen > 0);
}

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

void test_help_contains_help_text(void)
{
    testOutputLen = 0;
    FaultCommand_ExecuteLine("help", testWrite, NULL);
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "[cmd] help"));
}

void test_json_unknown_contains_error(void)
{
    testOutputLen = 0;
    FaultCommand_ExecuteLine("{\"cmd\":\"unknown\"}", testWrite, NULL);
    TEST_ASSERT_NOT_NULL(strstr(testOutput, "unknown_cmd"));
}