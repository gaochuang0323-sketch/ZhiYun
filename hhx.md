# BMS 故障模拟器 — 项目架构说明

## 一、项目概述

本系统是一台 BMS 故障模拟器，代替真实电池模组向 BMS 注入故障信号，并记录 BMS 的响应时间。

| 项目 | 说明 |
|------|------|
| 主控芯片 | STM32H743 + FreeRTOS 实时系统 |
| 电压输出 | DAC81416 芯片，13 路独立可调（0.5V~5V） |
| 通信方式 | 串口（USART1）+ 以太网（TCP JSON）|
| CAN 接口 | FDCAN1，500kbps，用于与 BMS 通信 |
| 外部 SRAM | 1MB，用于扩展存储 |

---

## 二、软件分层架构

整个软件分为 5 层，从下到上依次是：

```text
┌────────────────────────────────────────────┐
│ 控制入口层 — 串口 / TCP / CAN 接收命令       │
├────────────────────────────────────────────┤
│ 命令解析层 — 把文本/JSON 翻译成业务动作       │
├────────────────────────────────────────────┤
│ 应用业务层 — 电压模拟/波形/安全/响应记录      │
├────────────────────────────────────────────┤
│ 硬件驱动层 — DAC/CAN/以太网/SRAM 操作封装    │
├────────────────────────────────────────────┤
│ 芯片外设层 — SPI/GPIO/串口/CAN/以太网初始化  │
└────────────────────────────────────────────┘
```

---

## 三、FreeRTOS 任务分布

系统有 **4 个任务** 同时工作：

| 任务名 | 优先级 | 干什么 |
|--------|--------|--------|
| **defaultTask** | 中等 | 每 1ms 跑一次，处理电压模拟、波形、安全检测、串口和 CAN 输入 |
| **EthIf** | 最高 | 以太网收到数据包立刻处理，防止丢包 |
| **TcpJson** | 最低 | 监听端口 5000，处理 TCP JSON 命令 |
| **ethernet_link_thread** | 中等 | 每 100ms 检查网线是否插好 |

**为什么业务不分独立任务？** 电压、波形、安全都要操作 DAC 芯片，分三个任务会冲突（同时写 DAC 打架）。放在同一个 1ms 循环里按顺序执行，既简单又安全。

---

## 四、所有模块的接口函数一览

按模块列出每个函数的名字和作用，领导只需要知道"这个函数是干什么的"即可。

### 4.1 命令执行入口

| 函数 | 作用 |
|------|------|
| `FaultCommand_ExecuteLine(命令文本, 输出回调, 上下文指针)` | 执行一行命令。自动识别文本还是 JSON，分发给对应的业务函数。串口和 TCP 都走这里。 |

### 4.2 串口控制台

| 函数 | 作用 |
|------|------|
| `FaultConsole_Init()` | 初始化串口接收，准备接收命令 |
| `FaultConsole_Process()` | 每 1ms 检查串口有没有新输入，有则拼成一行交给命令解析 |

### 4.3 TCP JSON 服务

| 函数 | 作用 |
|------|------|
| `TcpJsonServer_Start()` | 启动 TCP 服务，监听端口 5000，接收 JSON 命令 |

### 4.4 电压模拟（核心模块）

**常量：** 13 路电芯，电压范围 500~5000mV，默认 3200mV

| 函数 | 作用 |
|------|------|
| `VoltageSim_Init(默认电压)` | 初始化 13 路电芯的默认电压 |
| `VoltageSim_Process(当前时间)` | 每 1ms 调用，推进故障状态机（升压/保持/恢复）|
| `VoltageSim_SetCellVoltageMv(通道号, 电压值)` | 设置单路电芯电压 |
| `VoltageSim_SetAllCellsVoltageMv(电压值)` | 设置全部 13 路为同一电压 |
| `VoltageSim_InjectCellOverVoltageRamp(通道, 目标电压, 时长, 斜率)` | 过压故障：按斜率升到目标→保持→恢复 |
| `VoltageSim_InjectCellUnderVoltageRamp(通道, 目标电压, 时长, 斜率)` | 欠压故障：按斜率降到目标→保持→恢复 |
| `VoltageSim_InjectVoltageDifference(高压通道, 高压值, 低压通道, 低压值, 时长, 斜率)` | 压差过大故障：两路同时设 |
| `VoltageSim_ClearCellFault(通道号)` | 清除单路故障 |
| `VoltageSim_ClearAllFaults()` | 清除全部故障 |
| `VoltageSim_SetCellCalibrationOffsetMv(通道, 偏移量)` | 设置该路校准补偿（±500mV）|
| `VoltageSim_GetActiveFaultCount()` | 查询当前有多少路处于故障状态 |
| `VoltageSim_MillivoltsToDacCode(电压值)` | 把毫伏值转成 DAC 芯片能识别的码值 |

### 4.5 波形生成

| 函数 | 作用 |
|------|------|
| `WaveformGen_StartSquare(通道, 低电平, 高电平, 周期)` | 启动方波输出 |
| `WaveformGen_StartSine(通道, 中心电压, 幅值, 周期)` | 启动单通道正弦波 |
| `WaveformGen_StartSineAll(中心电压, 幅值, 周期)` | 启动 13 路同步正弦波 |
| `WaveformGen_Stop()` | 停止波形 |
| `WaveformGen_Process(当前时间)` | 每 1ms 推进波形采样点 |
| `WaveformGen_GetStatus()` | 查询当前波形状态（类型/是否激活/参数）|

### 4.6 DAC 安全保护

| 函数 | 作用 |
|------|------|
| `DacSafety_Init(安全电压)` | 初始化安全保护，设置安全电压 |
| `DacSafety_Process(当前时间)` | 每 50ms 检查 DAC 报警引脚，低电平则自动急停 |
| `DacSafety_EmergencyStop(安全电压)` | 手动急停：停止波形→清除故障→输出安全电压→锁存 |
| `DacSafety_Release()` | 解除急停锁存，恢复正常控制 |
| `DacSafety_IsOutputAllowed()` | 查询是否允许修改 DAC 输出（急停中返回禁止）|
| `DacSafety_GetStatus()` | 查询安全状态（是否急停/是否报警/报警次数）|

### 4.7 BMS 响应时间记录

| 函数 | 作用 |
|------|------|
| `BmsResponseLog_RecordFaultTrigger(触发类型, 主通道, 主电压, 副通道, 副电压)` | 故障注入成功后记录 T1 |
| `BmsResponseLog_Clear()` | 清除响应记录 |
| `BmsResponseLog_GetStatus()` | 查询 T1、T2、响应延时 ΔT、响应帧数据 |
| `BmsResponseLog_SetFilter(是否启用, 是否扩展帧, CAN ID, 掩码)` | 配置目标 CAN 帧过滤 |
| `BmsResponseLog_DisableFilter()` | 关闭过滤 |
| `BmsResponseLog_SetDataFilter(字节位置, 掩码, 期望值)` | 配置数据字节过滤，要求 (data & mask) == value |
| `BmsResponseLog_ClearDataFilter()` | 清除数据字节过滤 |

### 4.8 CAN 通信

| 函数 | 作用 |
|------|------|
| `BspCan_Init()` | 初始化 FDCAN1，500kbps，接收所有帧 |
| `BspCan_SendClassic(CAN ID, 是否扩展帧, 数据指针, 数据长度)` | 发送一帧 CAN 数据 |
| `BspCan_Process()` | 每 1ms 检查有没有收到新 CAN 帧 |
| `BspCan_GetStatus()` | 查询 CAN 统计信息（发送/接收/错误计数）|
| `BspCan_OnRxFrame(接收帧)` | CAN 收到帧后的回调函数，由响应记录模块接管 |

### 4.9 DAC81416 芯片驱动

| 函数 | 作用 |
|------|------|
| `DAC81416_Init()` | 初始化 DAC 芯片，配置 2.5V 内部基准和 0~5V 量程 |
| `DAC_WriteRegister(命令, 地址, 数据)` | 写 DAC 寄存器 |
| `DAC_ReadRegister(命令, 地址)` | 读 DAC 寄存器（需要发 2 帧，CS 保持低）|
| `DAC_WriteChannel(通道号, 码值)` | 写单个 DAC 通道的输出电压 |
| `DAC_WriteAllChannels(所有通道码值)` | 一次性写全部 16 路并触发更新 |
| `DAC_EnableInternalRef(使能/关闭)` | 控制内部参考电压 |
| `DAC_SetAllChannelsRange(量程)` | 配置全部通道的输出量程 |
| `DAC_ConfigAllChannels0To5V()` | 快捷配置 0~5V 量程 |
| `DAC_PowerDownChannels(通道掩码)` | 控制通道上下电 |
| `DAC_TriggerLDAC()` | 软件触发 DAC 输出更新 |

### 4.10 DAC 板级支持

| 函数 | 作用 |
|------|------|
| `BSP_DAC81416_Init()` | 初始化 DAC 相关 GPIO 引脚状态 |
| `DAC_SPI_TransmitFrames24(帧数组, 帧数)` | 发送 24-bit SPI 帧（受互斥锁保护）|
| `DAC_SPI_TransmitReceiveFrames24(发送帧, 接收帧, 帧数)` | 收发 24-bit SPI 帧（读操作用）|
| `DAC_ResetHardware()` | DAC 硬件复位：拉低复位脚 10ms→释放→等10ms |
| `DAC_LDAC_Update()` | LDAC 低脉冲 1ms，更新 DAC 输出 |
| `DAC_ReadAlarmPin()` | 读取 DAC 报警引脚，返回 1=正常, 0=报警 |

### 4.11 外部 SRAM

| 函数 | 作用 |
|------|------|
| `BspSram_Init()` | 初始化 SRAM 状态记录 |
| `BspSram_RunSelfTest(测试字节数)` | 执行 SRAM 写读自检（默认 64KB，最大 1MB）|
| `BspSram_GetStatus()` | 查询 SRAM 基地址、容量、自检结果和失败地址 |

### 4.12 以太网 / TCP

| 函数 | 作用 |
|------|------|
| `MX_LWIP_Init()` | 初始化 LwIP 网络协议栈 |
| `TcpJsonServer_Start()` | 启动 TCP JSON 服务（端口 5000）|
| `BSP_ETH_PHY_Reset()` | 以太网 PHY 硬件复位 |

---

## 五、系统中的状态机

系统用了 4 个独立的小型状态机来管理随时间变化的行为：

**故障状态机（每路电芯一个，共 13 个）：**
空闲 → 按斜率升降压 → 保持目标电压 → 按斜率恢复 → 回到空闲

**安全状态机：**
正常监测（每 50ms 读报警引脚）→ 发现报警自动急停锁存 → 手动释放 → 回到正常

**PHY 状态机：**
扫描 PHY 地址 → 软件复位 → 检测网线 → 链路 up（开始收发）→ 链路 down（清除缓存）→ 重新扫描

**波形状态机：**
空闲 → 输出方波/正弦波 → 空闲

---

## 六、模块调用关系（简单版）

```text
串口/TCP 收到命令 "ov 7 4500"
  ↓
命令解析器判断是"过压故障"
  ↓
检查安全保护是否允许输出
  ↓
允许 → 调用电压模拟的过压注入函数
         设置第 7 路的故障参数
  ↓
下次 1ms 循环时，故障状态机自动开始逐步升压
  ↓
升到 4500mV → 保持 100ms → 恢复到原始电压
```

---

## 七、关键源码文件位置

| 功能 | 文件路径 |
|------|---------|
| 系统启动入口 | `Core/Src/main.c` |
| FreeRTOS 主循环 | `Core/Src/freertos.c` |
| 命令解析 | `App/Src/fault_command.c` |
| 串口控制台 | `App/Src/fault_console.c` |
| TCP JSON 服务 | `App/Src/tcp_json_server.c` |
| 电压模拟/故障注入 | `App/Src/voltage_sim.c` |
| 波形生成 | `App/Src/waveform_gen.c` |
| DAC 安全保护 | `App/Src/dac_safety.c` |
| BMS 响应记录 | `App/Src/bms_response_log.c` |
| DAC 芯片驱动 | `Drivers/DAC81416/Src/dac81416.c` |
| CAN 通信 | `BSP/Src/bsp_can.c` |
| 外部 SRAM | `BSP/Src/bsp_sram.c` |
| 以太网底层 | `LWIP/Target/ethernetif.c` |