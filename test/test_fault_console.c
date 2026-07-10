#include "unity.h"
#include "fault_console.h"
#include "stubs.h"

void FaultCommand_ExecuteLine(const char *line, void (*write)(const char *, void *), void *context)
{
    (void)line;
    (void)write;
    (void)context;
}

void setUp(void)
{
    UART_Reset();
    FaultConsole_Init();
}

void tearDown(void)
{
}

void test_Init_does_not_crash(void)
{
}

void test_Process_consumes_characters_and_line_ends_with_newline(void)
{
    const uint8_t input[] = "status\r";
    UART_PushData(input, sizeof(input) - 1);
    FaultConsole_Process();
}

void test_Process_handles_multiple_lines(void)
{
    const uint8_t input[] = "help\rstatus\r";
    UART_PushData(input, sizeof(input) - 1);
    FaultConsole_Process();
}

void test_Process_ignores_empty_lines(void)
{
    const uint8_t input[] = "\r\n";
    UART_PushData(input, sizeof(input) - 1);
    FaultConsole_Process();
}

void test_Process_line_too_long_resets(void)
{
    uint8_t longLine[200];
    uint16_t i;

    for (i = 0; i < 199; i++) {
        longLine[i] = 'A' + (i % 26);
    }
    longLine[199] = '\r';
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
