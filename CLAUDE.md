# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Firmware for a portable energy-storage BMS fault-simulation device (便携式储能电站 BMS 故障模拟装置). It drives a DAC81416 to emulate 13 battery-cell voltages (500–5000mV, default 3200mV), injects over/under-voltage and cell-difference faults, generates waveforms, and measures BMS CAN response latency. Hardware: STM32H743 (Cortex-M7) + FreeRTOS + LwIP.

## Build & Flash

Firmware build uses a GCC arm-none-eabi Makefile. On Windows the make binary is `mingw32-make`.

```bash
mingw32-make -j4          # build -> build/project_01.{elf,hex,bin}
mingw32-make flash        # STM32_Programmer_CLI -c port=SWD -w build/project_01.hex -v -rst
mingw32-make clean
```

Requires arm-none-eabi-gcc on PATH (or pass `GCC_PATH=...`) and STM32CubeProgrammer for `flash` (override with `PROGRAMMER=`). The project is a CubeMX-generated layout: `.ioc`/`.mxproject` regenerate `Core/`, `Drivers/`, `Middlewares/`, `LWIP/` — do not hand-edit those outside `USER CODE BEGIN/END` guards, and add new firmware `.c` files to `C_SOURCES` in the Makefile manually (CubeMX won't know about `App/` and `BSP/`).

## Host Unit Tests

Tests run on the host (not the target) via Ceedling + Unity + CMock, configured in `project.yml`. They compile `App/Src/**` and `BSP/Src/**` against hand-written stubs in `test/support/` (which replace the HAL, CMSIS-RTOS, and hardware headers). Test files are `test/test_*.c`.

```powershell
# From repo root (PowerShell). Clear stale generated runners first — they cause
# "multiple definition of resetTest" / "undefined reference to setUp".
Remove-Item -Force 'test\*_runner*.c' -ErrorAction SilentlyContinue
if (Test-Path 'test\build') { Remove-Item -Recurse -Force 'test\build' }
& 'C:\Ruby40-x64\bin\ruby.exe' -S ceedling gcov:all   # all tests + HTML coverage
```

- Single test file: `ceedling test:test_voltage_sim` (or `gcov:test_voltage_sim` for coverage).
- Coverage report: `test/build/artifacts/gcov/gcovr/GcovCoverageResults.html`.
- Disabled tests are renamed off the `test_` prefix so Ceedling skips them: `_pending_fault_con.c` (fault_console includes `fault_command.c` directly, so CMock can't isolate it) and `_hang_bsp_can.c` (leaves a background process). If a `build` delete fails, `taskkill /f /im test_bsp_can.exe` first.

## Architecture

Layered, with the firmware-specific code isolated from CubeMX output so it stays testable:

- `App/` — application logic, hardware-independent, unit-tested. This is where most feature work happens.
- `BSP/` — board support: thin wrappers over HAL for DAC (SPI), CAN, Ethernet PHY, external SRAM (FMC).
- `Core/`, `Drivers/`, `Middlewares/`, `LWIP/` — CubeMX/vendor generated (STM32 HAL, FreeRTOS, LwIP). Edit only inside `USER CODE` blocks.

### Runtime model

There is effectively **one FreeRTOS task doing the application work**: `StartDefaultTask` in `Core/Src/freertos.c`. It runs a 1ms cooperative super-loop that calls, each tick:
`VoltageSim_Process` → `WaveformGen_Process` → `DacSafety_Process` → `FaultConsole_Process` → `BspCan_Process`, then `osDelay(1)`.

Consequences to keep in mind:
- All `*_Process()` functions are polled every ~1ms and must be non-blocking. Fault ramps, waveform stepping (low-freq path), timeouts, and CAN RX draining are all driven by comparing `HAL_GetTick()` against stored timestamps — not by blocking or timers.
- LwIP/TCP runs in its own thread(s) via the LwIP CMSIS-OS port; the TCP JSON server is started from `StartDefaultTask`.
- Shared resources are mutex-protected: `printfMutex` (serial output, created in `MX_FREERTOS_Init`) and the SPI/DAC mutex in `bsp_dac81416.c`. Hold them briefly.

### Command dispatch

Serial (`fault_console.c`, USART1 @115200) and TCP (`tcp_json_server.c`, static IP 192.168.137.202:5000) are two front-ends that funnel into one parser: `fault_command.c`. It accepts both a text grammar and line-delimited JSON, and writes replies through a `FaultCommand_WriteFn` callback (so the same command logic serves both transports). When adding a command, add it in `fault_command.c` (text branch, JSON branch, and the help text) — not in the transport files.

### Feature modules (`App/Src`)

- `voltage_sim.c` — the core model: per-cell target voltage, fault state machines (OV/UV/diff with slew-rate ramps and auto-restore), per-channel calibration offset (RAM only, lost on power-off), mV↔DAC-code conversion.
- `waveform_gen.c` — low-frequency square/sine, stepped in the 1ms super-loop.
- `waveform_engine.c` — high-frequency `wave high` path using TIM3 + SPI DMA (see README "High-Speed Waveform Engine Note"): single-channel uses TIM3-update DMA streaming 24-bit frames straight to `SPI1->TXDR`; multi-channel uses the TIM3-interrupt + SPI-DMA-burst path.
- `dac_safety.c` — emergency-stop latch and `nALMOUT` monitoring; can override all outputs.
- `bms_response_log.c` — records fault-injection time (T1), matches the BMS's CAN response frame (T2) via configurable ID/mask + data-byte filters, computes ΔT.

Voltage/waveform/safety modules all ultimately write through the DAC BSP; safety state gates the others.

## Conventions

- C style: `ModuleName_FunctionName` (PascalCase module prefix), most functions return `HAL_StatusTypeDef`. Match the existing naming and the doc-comment style in headers.
- Keep application logic in `App/` free of direct HAL calls where practical — go through `BSP/` — so it remains host-testable against the stubs.
- Voltage inputs are validated against `VOLTAGE_SIM_MIN_MV`/`MAX_MV` (500/5000); preserve these clamps.

## Reference Docs

Extensive Chinese docs exist. Most useful: `README.md` (full command reference + usage tutorials), `项目分层架构与接口说明.md` (architecture + per-module interfaces), `流程图.md` (Mermaid diagrams), `测试使用说明.md` (test setup), `docs/通信协议.md` (CAN protocol).
