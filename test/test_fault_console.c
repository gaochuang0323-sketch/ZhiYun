<<<<<<< HEAD
#include "unity.h"
#include "fault_console.h"
#include "stubs.h"

void FaultCommand_ExecuteLine(const char *line, void (*write)(const char *, void *), void *context)
{
    (void)line;
    (void)write;
    (void)context;
=======
/**
 * Unit tests for Fault Console module
 *
 * Tests UART character reading and line accumulation.
 */
#include "unity.h"
#include "fault_console.h"
#include "stubs.h"
#include "fault_command.h"

/* UART receive stub state */
static uint8_t uartBuffer[256];
static uint16_t uartWriteIndex = 0;
static uint16_t uartReadIndex = 0;

/* Override HAL_UART_Receive to read from our test buffer */
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void)huart;
    (void)Size;
    (void)Timeout;

    if (uartReadIndex < uartWriteIndex) {
        *pData = uartBuffer[uartReadIndex++];
        return HAL_OK;
    }
    return HAL_ERROR;
}

static void uartPushData(const uint8_t *data, uint16_t length)
{
    uint16_t i;
    for (i = 0; i < length && uartWriteIndex < sizeof(uartBuffer); i++) {
        uartBuffer[uartWriteIndex++] = data[i];
    }
}

static void uartReset(void)
{
    uartWriteIndex = 0;
    uartReadIndex = 0;
}

/* Test output capture */
static char consoleOutput[1024];
static uint32_t consoleOutputLen;

static void testWrite(const char *text, void *context)
{
    (void)context;
    if (text) {
        uint32_t len = 0;
        while (text[len] != '\0' && (consoleOutputLen + len) < (sizeof(consoleOutput) - 1)) {
            consoleOutput[consoleOutputLen + len] = text[len];
            len++;
        }
        consoleOutputLen += len;
        consoleOutput[consoleOutputLen] = '\0';
    }
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
}

void setUp(void)
{
<<<<<<< HEAD
    UART_Reset();
=======
    uartReset();
    consoleOutput[0] = '\0';
    consoleOutputLen = 0;
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
    FaultConsole_Init();
}

void tearDown(void)
{
}

<<<<<<< HEAD
void test_Init_does_not_crash(void)
{
}

void test_Process_consumes_characters_and_line_ends_with_newline(void)
{
    const uint8_t input[] = "status\r";
    UART_PushData(input, sizeof(input) - 1);
    FaultConsole_Process();
=======
/* ========== FaultConsole_Init ========== */
void test_Init_prints_ready_message(void)
{
    /* Init already called in setUp */
    /* The Init function calls printf, which goes to stdout */
}

/* ========== FaultConsole_Process with line input ========== */
void test_Process_accumulates_characters_until_newline(void)
{
    const uint8_t input[] = "status\r";
    uartPushData(input, sizeof(input) - 1);

    /* Mock FaultCommand_ExecuteLine to verify it gets called with the right text */
    /* Our test write callback will capture the output */
    FaultConsole_Process();
    /* Process should have consumed the line */
    TEST_ASSERT_EQUAL_UINT32(uartReadIndex, uartWriteIndex);
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
}

void test_Process_handles_multiple_lines(void)
{
    const uint8_t input[] = "help\rstatus\r";
<<<<<<< HEAD
    UART_PushData(input, sizeof(input) - 1);
    FaultConsole_Process();
=======
    uartPushData(input, sizeof(input) - 1);

    FaultConsole_Process();
    /* Both lines should be consumed */
    TEST_ASSERT_EQUAL_UINT32(uartReadIndex, uartWriteIndex);
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
}

void test_Process_ignores_empty_lines(void)
{
    const uint8_t input[] = "\r\n";
<<<<<<< HEAD
    UART_PushData(input, sizeof(input) - 1);
    FaultConsole_Process();
=======
    uartPushData(input, sizeof(input) - 1);

    FaultConsole_Process();
    TEST_ASSERT_EQUAL_UINT32(uartReadIndex, uartWriteIndex);
    /* No output from empty commands */
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
}

void test_Process_line_too_long_resets(void)
{
<<<<<<< HEAD
=======
    /* Create a line longer than 160 characters */
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
    uint8_t longLine[200];
    uint16_t i;

    for (i = 0; i < 199; i++) {
        longLine[i] = 'A' + (i % 26);
    }
    longLine[199] = '\r';
<<<<<<< HEAD
    UART_PushData(longLine, 200);
    FaultConsole_Process();
}

void test_Process_handles_unix_newline(void)
{
    const uint8_t input[] = "status\n";
    UART_PushData(input, sizeof(input) - 1);
    FaultConsole_Process();
}

void test_Process_handles_carriage_return_without_line_feed(void)
{
    const uint8_t input[] = "set 1 2500\r";
    UART_PushData(input, sizeof(input) - 1);
    FaultConsole_Process();
}

void test_Process_accumulates_multiple_chunks(void)
{
    const uint8_t line1[] = "help\r";
    const uint8_t line2[] = "status\r";

    UART_PushData(line1, sizeof(line1) - 1);
    UART_PushData(line2, sizeof(line2) - 1);
    FaultConsole_Process();
}

void test_Process_no_data_does_nothing(void)
{
    FaultConsole_Process();
}
=======
    uartPushData(longLine, 200);

    FaultConsole_Process();
    /* Line should have been discarded */
    TEST_ASSERT_EQUAL_UINT32(uartReadIndex, uartWriteIndex);
}
>>>>>>> f4c64959d37a1ca5ac670db93692e329027a8e2d
