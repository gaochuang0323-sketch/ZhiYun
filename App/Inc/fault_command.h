#ifndef FAULT_COMMAND_H
#define FAULT_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef void (*FaultCommand_WriteFn)(const char *text, void *context);

void FaultCommand_ExecuteLine(const char *line, FaultCommand_WriteFn write, void *context);

#ifdef __cplusplus
}
#endif

#endif /* FAULT_COMMAND_H */
