# ZhiYun BMS Fault Simulator

便携式储能电站 BMS 故障模拟装置固件工程。

## 当前能力

- STM32H743 + FreeRTOS 基础工程
- USART1 启动日志与串口命令控制台
- DAC81416 SPI1 驱动，内部 2.5V VREF，全部通道 0~5V 量程
- 13 串电芯电压模拟，默认 3200mV
- 单体过压、单体欠压、压差过大故障注入
- 支持故障持续时间和 mV/ms 斜率控制
- 支持单通道方波、正弦波输出，用于 DAC/后级功放动态验证
- 支持每通道 mV 级软件 offset 校准
- 支持 USART 文本/JSON 命令，LwIP TCP JSON server 默认端口 5000

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
wave status
wave stop
wave square <cell> <low_mv> <high_mv> <period_ms>
wave sine <cell> <offset_mv> <amplitude_mv> <period_ms>
cal status
cal set <cell> <offset_mv>
cal trim <cell> <target_mv> <measured_mv>
cal clear [cell]
test static <mv>
test slew <cell> <low_mv> <high_mv> <period_ms>
clear [cell]
```

示例：

```text
status
ov 7 4500 100 3000
uv 3 1000 100 3000
diff 1 3500 2 2800 200 3000
wave square 1 1000 4000 1000
wave sine 1 2500 1000 1000
test static 2500
test slew 1 1000 4000 1000
cal trim 1 2500 2485
cal status
clear
normal 3200
```

说明：

- `wave square`/`test slew` 会持续输出方波，直到执行 `wave stop`、`clear`、`normal`、`set`、`ov`、`uv` 或 `diff`。
- `cal trim 1 2500 2485` 表示 C01 目标 2500mV、实测 2485mV，固件会在当前 offset 基础上自动增加 15mV。
- offset 限幅为 `+/-500mV`，避免误输入把输出推到危险状态。

### JSON 命令

同一套命令也支持 JSON 格式。当前 USART 控制台和 LwIP TCP server 都可以接收一行 JSON；其他 WiFi/以太网适配器只需要把收到的一行 JSON 传给 `FaultCommand_ExecuteLine()`。

```json
{"cmd":"help"}
{"cmd":"status"}
{"cmd":"normal","mv":3200}
{"cmd":"set","cell":5,"mv":3300}
{"cmd":"ov","cell":7,"mv":4500,"duration_ms":100,"slew_mv_per_ms":3000}
{"cmd":"uv","cell":3,"mv":1000,"duration_ms":100,"slew_mv_per_ms":3000}
{"cmd":"diff","high_cell":1,"high_mv":3500,"low_cell":2,"low_mv":2800,"duration_ms":200,"slew_mv_per_ms":3000}
{"cmd":"wave","type":"square","cell":1,"low_mv":1000,"high_mv":4000,"period_ms":1000}
{"cmd":"wave","type":"sine","cell":1,"offset_mv":2500,"amplitude_mv":1000,"period_ms":1000}
{"cmd":"wave_stop"}
{"cmd":"cal","action":"status"}
{"cmd":"cal","action":"set","cell":1,"offset_mv":15}
{"cmd":"cal","action":"trim","cell":1,"target_mv":2500,"measured_mv":2485}
{"cmd":"cal","action":"clear","cell":1}
{"cmd":"test","type":"static","mv":2500}
{"cmd":"test","type":"slew","cell":1,"low_mv":1000,"high_mv":4000,"period_ms":1000}
{"cmd":"clear"}
{"cmd":"clear","cell":7}
```

响应也是 JSON：

```json
{"ok":true,"cmd":"ov"}
{"ok":false,"cmd":"set"}
```

`status` 会返回 13 串当前电压和故障状态。

## DAC 调试流程

参考 `zhinan.md`，建议先做 1 路原型闭环，再扩到 13 路：

1. 静态校准：依次发送 `test static 1000`、`test static 2500`、`test static 4500`，用万用表记录每路输出误差。
2. 软件补偿：例如 C01 目标 2500mV、实测 2485mV，发送 `cal trim 1 2500 2485`，再用 `cal status` 检查 offset。
3. 压摆率测试：发送 `test slew 1 1000 4000 1000`，示波器 CH1 接 DAC/功放前，CH2 接输出端，测 CH2 的 10%~90% 上升时间。
4. 正弦验证：发送 `wave sine 1 2500 1000 1000`，确认后级输出无明显削顶、振荡或异常漂移。

当前波形发生器由 FreeRTOS 1ms 周期推进，适合低频功能验证和硬件调试。需要更高频率、更低抖动时，应进入硬件定时器 + SPI DMA + LDAC 同步刷新阶段。

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
