#include "fault_console.h"

#include <stdio.h>

#include "fault_command.h"
#include "usart.h"

#define FAULT_CONSOLE_LINE_SIZE 160U
#define FAULT_CONSOLE_RX_BUDGET 32U

static char consoleLine[FAULT_CONSOLE_LINE_SIZE];
static uint16_t consoleLineLength;

static void FaultConsole_Write(const char *text, void *context)
{
  (void)context;
  printf("%s", text);
}

void FaultConsole_Init(void)
{
  consoleLineLength = 0U;
  printf("[cmd] console ready, type 'help' or JSON {\"cmd\":\"help\"}\r\n");
}

void FaultConsole_Process(void)
{
  uint8_t data;

  for (uint32_t budget = 0U; budget < FAULT_CONSOLE_RX_BUDGET; budget++)
  {
    if (HAL_UART_Receive(&huart1, &data, 1U, 0U) != HAL_OK)
    {
      break;
    }

    if ((data == '\r') || (data == '\n'))
    {
      if (consoleLineLength > 0U)
      {
        consoleLine[consoleLineLength] = '\0';
        FaultCommand_ExecuteLine(consoleLine, FaultConsole_Write, NULL);
        consoleLineLength = 0U;
      }
      continue;
    }

    if (consoleLineLength < (FAULT_CONSOLE_LINE_SIZE - 1U))
    {
      consoleLine[consoleLineLength++] = (char)data;
    }
    else
    {
      consoleLineLength = 0U;
      printf("[cmd] line too long\r\n");
    }
  }
}
