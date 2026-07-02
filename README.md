# ZhiYun BMS Fault Simulator

便携式储能电站 BMS 故障模拟装置固件工程。

## 当前能力

- STM32H743 + FreeRTOS 基础工程
- USART1 启动日志与串口命令控制台
- DAC81416 SPI1 驱动，内部 2.5V VREF，全部通道 0~5V 量程
- 13 串电芯电压模拟，默认 3200mV
- 单体过压、单体欠压、压差过大故障注入
- 支持故障持续时间和 mV/ms 斜率控制

## 编译与下载

```bash
mingw32-make
mingw32-make flash
```

`flash` 目标使用：

```bash
STM32_Programmer_CLI -c port=SWD -w build/project_01.hex -v -rst
```

## 串口控制

USART1 参数：

- 115200 baud
- 8 data bits
- no parity
- 1 stop bit

命令以回车或换行结束。

### 文本命令

```text
help
status
normal [mv]
set <cell> <mv>
ov <cell> [mv] [duration_ms] [slew_mv_per_ms]
uv <cell> [mv] [duration_ms] [slew_mv_per_ms]
diff <high_cell> <high_mv> <low_cell> <low_mv> [duration_ms] [slew_mv_per_ms]
clear [cell]
```

示例：

```text
status
ov 7 4500 100 3000
uv 3 1000 100 3000
diff 1 3500 2 2800 200 3000
clear
normal 3200
```

### JSON 命令

同一套命令也支持 JSON 格式。当前 USART 控制台已经可以直接接收 JSON；后续 TCP/WiFi/以太网只需要把收到的一行 JSON 传给 `FaultCommand_ExecuteLine()`。

```json
{"cmd":"help"}
{"cmd":"status"}
{"cmd":"normal","mv":3200}
{"cmd":"set","cell":5,"mv":3300}
{"cmd":"ov","cell":7,"mv":4500,"duration_ms":100,"slew_mv_per_ms":3000}
{"cmd":"uv","cell":3,"mv":1000,"duration_ms":100,"slew_mv_per_ms":3000}
{"cmd":"diff","high_cell":1,"high_mv":3500,"low_cell":2,"low_mv":2800,"duration_ms":200,"slew_mv_per_ms":3000}
{"cmd":"clear"}
{"cmd":"clear","cell":7}
```

响应也是 JSON：

```json
{"ok":true,"cmd":"ov"}
{"ok":false,"cmd":"set"}
```

`status` 会返回 13 串当前电压和故障状态。

## 命令层复用

命令执行层在：

```text
App/Inc/fault_command.h
App/Src/fault_command.c
```

传输层只需要提供一行字符串和输出回调：

```c
FaultCommand_ExecuteLine(line, write_callback, context);
```

当前 USART 控制台是一个适配器：

```text
App/Inc/fault_console.h
App/Src/fault_console.c
```

## 当前限制

当前故障调度由 FreeRTOS 1ms 周期任务推进，适合功能验证和 BMS 联动初测。若要达到技术方案中的多通道同步误差小于 100us，下一阶段应切换为硬件定时器触发 + SPI DMA + LDAC 同步刷新。
