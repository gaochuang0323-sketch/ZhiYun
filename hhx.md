# BMS 故障模拟器项目分层架构与接口说明

## 总体架构,接口的里面的参数(具体一点),RTOS里面每个任务的周期,优先级的分配是怎么进行的也要说明

    ┌─────────────────────────────────────────────────────────────┐
    │ 上位机 / 串口调试工具 / TCP JSON 客户端 / USB-CAN 工具           │
    └───────────────┬───────────────────────┬─────────────────────┘
                    │                       │
              USART 文本/JSON          TCP JSON / CAN
                    │                       │
    ┌───────────────▼───────────────────────▼─────────────────────┐
    │ 通信与命令层                                                  │
    │ fault_console / tcp_json_server / fault_command             │
    └───────────────┬───────────────────────┬─────────────────────┘
                    │                       │
    ┌───────────────▼───────────────────────▼─────────────────────┐
    │ 应用业务层                                                    │
    │ voltage_sim / waveform_gen / dac_safety / bms_response_log  │
    └───────────────┬───────────────────────┬─────────────────────┘
                    │                       │
    ┌───────────────▼───────────────────────▼─────────────────────┐
    │ 器件驱动层                                                    │
    │ DAC81416 register driver                                    │
    └───────────────┬───────────────────────┬─────────────────────┘
                    │                       │
    ┌───────────────▼───────────────────────▼─────────────────────┐
    │ BSP 板级支持层                                                │
    │ bsp_dac81416 / bsp_can / bsp_eth / bsp_sram                 │
    └───────────────┬───────────────────────┬─────────────────────┘
                    │                       |
    ┌───────────────▼───────────────────────▼─────────────────────┐
    │ HAL / CubeMX 外设层                                          │
    │ SPI1 / GPIO / USART1 / FDCAN1 / ETH / FMC / FreeRTOS / LwIP │
    └─────────────────────────────────────────────────────────────┘

## 分层职责

| 层级               | 主要文件                                                                         | 职责                                                             |
| ---------------- | ---------------------------------------------------------------------------- | -------------------------------------------------------------- |
| HAL / CubeMX 外设层 | `Core/Src/*.c`、`LWIP/*`                                                      | 初始化 MCU 外设、时钟、GPIO、SPI、USART、FDCAN、ETH、FMC、FreeRTOS 和 LwIP     |
| BSP 板级支持层        | `BSP/Src/*`                                                                  | 封装板级硬件操作，例如 DAC CS/RESET/LDAC/ALMOUT、CAN 收发、以太网 PHY、外部 SRAM 自检 |
| 器件驱动层            | `Drivers/DAC81416/*`                                                         | 封装 DAC81416 寄存器读写、24-bit SPI 帧、通道输出、量程配置、内部基准控制                |
| 应用业务层            | `App/Src/voltage_sim.c`、`waveform_gen.c`、`dac_safety.c`、`bms_response_log.c` | 实现电芯电压模拟、故障注入、波形输出、DAC 安全保护、BMS 响应时间记录                         |
| 通信与命令层           | `fault_console.c`、`tcp_json_server.c`、`fault_command.c`                      | 将串口文本命令和 TCP JSON 命令统一解析到同一套业务接口                               |
| 调度入口层            | `main.c`、`freertos.c`                                                        | 系统启动、模块初始化、周期调度、后台任务循环                                         |

## 主要模块接口

### 命令执行接口

| 接口                           | 文件                          | 功能                         |
| ---------------------------- | --------------------------- | -------------------------- |
| `FaultCommand_ExecuteLine()` | `App/Inc/fault_command.h`   | 统一执行一行文本命令或 JSON 命令        |
| `FaultConsole_Init()`        | `App/Inc/fault_console.h`   | 初始化 USART1 命令接收            |
| `FaultConsole_Process()`     | `App/Inc/fault_console.h`   | 周期处理串口输入并转交命令层             |
| `TcpJsonServer_Start()`      | `App/Inc/tcp_json_server.h` | 启动 TCP JSON 控制服务，端口 `5000` |

说明：串口和 TCP 最终都调用 `FaultCommand_ExecuteLine()`，因此两种控制方式复用同一套业务逻辑。

### 电压模拟接口

| 接口                                                            | 功能                   |
| ------------------------------------------------------------- | -------------------- |
| `VoltageSim_Init()`                                           | 初始化 13 路电芯默认电压       |
| `VoltageSim_SetCellVoltageMv()`                               | 设置单节电芯电压             |
| `VoltageSim_SetAllCellsVoltageMv()`                           | 设置全部电芯电压             |
| `VoltageSim_InjectCellOverVoltageRamp()`                      | 单体过压故障，支持持续时间和斜率     |
| `VoltageSim_InjectCellUnderVoltageRamp()`                     | 单体欠压故障，支持持续时间和斜率     |
| `VoltageSim_InjectVoltageDifference()`                        | 压差过大故障               |
| `VoltageSim_ClearCellFault()` / `VoltageSim_ClearAllFaults()` | 清除故障                 |
| `VoltageSim_Process()`                                        | FreeRTOS 周期调用，推进故障时序 |
| `VoltageSim_SetCellCalibrationOffsetMv()`                     | 每通道 mV 级校准补偿         |
| `VoltageSim_MillivoltsToDacCode()`                            | mV 到 DAC 码值转换        |

当前范围：`C01~C13`，电压范围 `500mV~5000mV`，默认正常电压 `3200mV`。

### 波形输出接口

| 接口                           | 功能                    |
| ---------------------------- | --------------------- |
| `WaveformGen_StartSquare()`  | 单通道方波输出               |
| `WaveformGen_StartSine()`    | 单通道正弦波输出              |
| `WaveformGen_StartSineAll()` | 13 路同相同步正弦波输出         |
| `WaveformGen_Stop()`         | 停止波形                  |
| `WaveformGen_Process()`      | FreeRTOS 周期调用，推进波形采样点 |
| `WaveformGen_GetStatus()`    | 查询当前波形状态              |

说明：当前波形发生器为 FreeRTOS 1ms 低频验证版本，后续高精度版本应升级为 `TIM + SPI DMA + LDAC`。

### DAC 安全保护接口

| 接口                            | 功能                                   |
| ----------------------------- | ------------------------------------ |
| `DacSafety_Init()`            | 初始化安全电压，默认 `3200mV`                  |
| `DacSafety_Process()`         | 周期读取 `nALMOUT` 报警引脚(**==还可以进行优化==**) |
| `DacSafety_EmergencyStop()`   | 急停，停止波形、清除故障、输出安全电压                  |
| `DacSafety_Release()`         | 释放急停锁存                               |
| `DacSafety_IsOutputAllowed()` | 判断当前是否允许修改 DAC 输出                    |
| `DacSafety_GetStatus()`       | 查询安全状态、报警计数和最近报警时间                   |

安全策略：急停锁存期间，`normal`、`set`、`ov`、`uv`、`diff`、`wave`、`cal`、`clear` 等会改变输出的命令会被拒绝。

### BMS 响应时间记录接口

| 接口                                    | 功能                         |
| ------------------------------------- | -------------------------- |
| `BmsResponseLog_RecordFaultTrigger()` | 故障注入成功时记录 T1               |
| `BspCan_OnRxFrame()`                  | CAN 收到帧后回调到响应记录模块          |
| `BmsResponseLog_SetFilter()`          | 配置目标 CAN ID/mask 和标准/扩展帧类型 |
| `BmsResponseLog_SetDataFilter()`      | 配置数据字节 bit/mask 过滤         |
| `BmsResponseLog_ClearDataFilter()`    | 清除数据过滤                     |
| `BmsResponseLog_Clear()`              | 清除当前响应记录                   |
| `BmsResponseLog_GetStatus()`          | 查询 T1/T2、响应延时、响应帧数据和过滤命中状态 |

当前能力：支持从“故障后第一帧 CAN”升级为“目标 CAN 告警帧”响应时间统计。拿到 BMS 的 DBC 或告警 bit 定义后，可以直接配置目标 ID 和数据掩码。

### DAC81416 器件驱动接口

| 接口                             | 功能                                    |
| ------------------------------ | ------------------------------------- |
| `DAC81416_Init()`              | 初始化 DAC，配置内部 2.5V 基准和 0~5V 量程         |
| `DAC_WriteRegister()`          | 写 DAC81416 寄存器                        |
| `DAC_ReadRegister()`           | 读 DAC81416 寄存器，read + NOP 两个 24-bit 帧 |
| `DAC_WriteChannel()`           | 写单个 DAC 通道                            |
| `DAC_WriteAllChannels()`       | 写全部 16 路 DAC 通道并触发 LDAC               |
| `DAC_EnableInternalRef()`      | 使能/关闭内部参考电压                           |
| `DAC_SetAllChannelsRange()`    | 配置全部通道量程                              |
| `DAC_ConfigAllChannels0To5V()` | 统一配置 0~5V 输出范围                        |
| `DAC_PowerDownChannels()`      | 配置通道上下电                               |
| `DAC_TriggerLDAC()`            | 软件触发 DAC 更新                           |

SPI 特性：当前 SPI1 使用 STM32H7 原生 `24-bit` DataSize，每个命令严格 24 个 SCLK；读寄存器使用 `0xC0 | reg`，连续发送两个 24-bit 帧，共 48 个 SCLK。

### DAC BSP 接口

| 接口                                  | 功能                                     |
| ----------------------------------- | -------------------------------------- |
| `BSP_DAC81416_Init()`               | 初始化 DAC 板级 GPIO 状态                     |
| `DAC_SPI_TransmitFrames24()`        | 发送一个或多个 24-bit DAC SPI 帧               |
| `DAC_SPI_TransmitReceiveFrames24()` | 收发一个或多个 24-bit DAC SPI 帧，CS 在整个调用期间保持低 |
| `DAC_ResetHardware()`               | DAC 硬件复位，低电平保持 10ms，释放后等待 10ms         |
| `DAC_LDAC_Update()`                 | LDAC 低脉冲 1ms，更新 DAC 输出                 |
| `DAC_ReadAlarmPin()`                | 读取 `nALMOUT` 报警引脚                      |

### CAN BSP 接口

| 接口                     | 功能                                        |
| ---------------------- | ----------------------------------------- |
| `BspCan_Init()`        | 启动 FDCAN1，配置 Classic CAN 500kbps，接收标准/扩展帧 |
| `BspCan_SendClassic()` | 发送标准帧或扩展帧                                 |
| `BspCan_Process()`     | 周期读取 Rx FIFO，打印日志并触发接收回调                  |
| `BspCan_GetStatus()`   | 查询 CAN 发送/接收/错误计数和最后一帧                    |
| `BspCan_OnRxFrame()`   | 弱回调接口，当前由 BMS 响应记录模块实现                    |

### SRAM / FMC 接口

| 接口                        | 功能                              |
| ------------------------- | ------------------------------- |
| `BspSram_Init()`          | 初始化 SRAM 状态记录                   |
| `BspSram_RunSelfTest()`   | 外部 SRAM 写读自检，默认测试 64KB，可扩展到 1MB |
| `BspSram_GetStatus()`     | 查询 SRAM 基地址、容量、自检结果和失败地址        |
| `BspSram_GetResultName()` | 将 HAL 状态转为字符串                   |

当前 SRAM 基地址 `0x60000000`，容量 `1MB`，16bit 总线。当前仅实现基础自检，尚未作为日志缓存或文件系统使用。

### 以太网 / TCP 接口

| 接口                      | 文件                          | 功能                    |
| ----------------------- | --------------------------- | --------------------- |
| `MX_LWIP_Init()`        | `LWIP/App/lwip.c`           | 初始化 LwIP 网络协议栈        |
| `TcpJsonServer_Start()` | `App/Src/tcp_json_server.c` | 启动 TCP JSON server    |
| `BSP_ETH_PHY_Reset()`   | `BSP/Src/bsp_eth.c`         | LAN8720A PHY 软件管理接口支持 |

当前默认 IP：`192.168.137.202`，TCP JSON 端口：`5000`。

## 关键源码索引

| 模块              | 文件                                |
| --------------- | --------------------------------- |
| 系统启动            | `Core/Src/main.c`                 |
| FreeRTOS 主循环    | `Core/Src/freertos.c`             |
| SPI1 配置         | `Core/Src/spi.c`                  |
| FDCAN 配置        | `Core/Src/fdcan.c`                |
| FMC/SRAM 配置     | `Core/Src/fmc.c`                  |
| USART 配置        | `Core/Src/usart.c`                |
| LwIP 初始化        | `LWIP/App/lwip.c`                 |
| 以太网底层           | `LWIP/Target/ethernetif.c`        |
| 命令执行            | `App/Src/fault_command.c`         |
| 串口控制台           | `App/Src/fault_console.c`         |
| TCP JSON server | `App/Src/tcp_json_server.c`       |
| 电压模拟            | `App/Src/voltage_sim.c`           |
| 波形发生器           | `App/Src/waveform_gen.c`          |
| DAC 安全保护        | `App/Src/dac_safety.c`            |
| BMS 响应记录        | `App/Src/bms_response_log.c`      |
| DAC81416 驱动     | `Drivers/DAC81416/Src/dac81416.c` |
| DAC BSP         | `BSP/Src/bsp_dac81416.c`          |
| CAN BSP         | `BSP/Src/bsp_can.c`               |
| ETH BSP         | `BSP/Src/bsp_eth.c`               |
| SRAM BSP        | `BSP/Src/bsp_sram.c`              |
