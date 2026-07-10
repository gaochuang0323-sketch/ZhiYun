# ZhiYun BMS Fault Simulator

## High-Speed Waveform Engine Note

`wave high <cell> <freq> <points> <amp> <offset>` now has two execution paths:

- Single-channel output uses TIM3 update DMA request (`DMA_REQUEST_TIM3_UP`) in circular mode. DMA writes precomputed 24-bit DAC frames directly to `SPI1->TXDR`, so the CPU no longer starts one SPI DMA transfer per sample.
- Multi-channel output keeps the compatible TIM3 interrupt + SPI DMA burst path, because it still needs grouped channel writes plus an LDAC pulse after each sample point.
- The single-channel turbo path keeps DAC CS low and LDAC low while streaming. If the DAC or board-level timing requires a CS pulse for every 24-bit frame, the next hardware step is to route SPI hardware NSS/LDAC timing instead of software GPIO.
- With SPI1 at 50MHz and 24-bit frames, the practical single-channel DMA pacing target is around 1Msps before analog settling and DAC timing margins are considered. Verify the real limit with an oscilloscope.

---

便携式储能电站 BMS 故障模拟装置固件工程。

**硬件平台：** STM32H743 + FreeRTOS + LwIP  
**电压输出：** DAC81416（16 位精度，13 路独立可调 500~5000mV）  
**通信方式：** 串口（USART1 115200）+ 以太网（TCP JSON 端口 5000）  
**CAN 接口：** FDCAN1 Classic CAN 500kbps

---

## 📖 目录

1. [快速导航：第一次使用该做什么](#-快速导航第一次使用该做什么)
2. [当前能力](#-当前能力)
3. [编译与下载](#-编译与下载)
4. [串口控制（最常用）](#-串口控制最常用)
5. [TCP JSON 网络控制](#-tcp-json-网络控制)
6. [CAN 通信调试](#-can-通信调试)
7. [DAC 输出调试教程](#-dac-输出调试教程)
8. [故障注入教程](#-故障注入教程)
9. [BMS 响应时间测量教程](#-bms-响应时间测量教程)
10. [外部 SRAM 自检](#-外部-sram-自检)
11. [常见问题与排查](#-常见问题与排查)
12. [项目文档索引](#-项目文档索引)
13. [代码质量改进记录](#-代码质量改进记录)
14. [当前限制](#-当前限制)

---

## 🚀 快速导航：

```text
第 1 步：编译下载
   → 执行 mingw32-make 
   → 执行 mingw32-make flash（需要 ST-Link）

第 2 步：确认启动正常
   → 打开串口 115200,8N1
   → 看到 [boot] project start 等日志
   → 输入 help 确认控制台可用

第 3 步：确认电压输出
   → 输入 status 查看 13 路电压（默认都是 3200mV）
   → 输入 test static 2500，万用表测 C01 是否输出 2500mV

第 4 步：试试故障注入
   → 输入 ov 7 4500 100 3000
   → 观察第 7 路电压从 3200mV 升到 4500mV，保持 100ms 后恢复

第 5 步：试试波形输出
   → 输入 wave sine 1 2500 1000 1000
   → 示波器看 C01 输出正弦波（中心 2500mV，幅值 1000mV）

第 6 步：试试网络控制
   → 网线连接板子和 PC
   → 串口看到 [eth] link up
   → PC 执行 nc 192.168.137.202 5000
   → 发送 {"cmd":"status"}，查看返回值
```

---

## ✅ 当前能力

| 功能模块         | 说明                             |
| ------------ | ------------------------------ |
| 13 路电芯电压输出   | 500~5000mV 独立可调，默认 3200mV      |
| 单体过压/欠压/压差故障 | 支持斜率和持续时间控制，自动恢复               |
| 方波/正弦波输出     | 单通道或 13 路同步                    |
| 每通道校准补偿      | 软件 offset，范围 ±500mV            |
| DAC 安全保护     | 急停锁存 + nALMOUT 自动监测            |
| BMS 响应时间记录   | 故障注入后自动记录 T1→CAN 响应→T2→计算 ΔT   |
| CAN 帧过滤      | 支持 ID/mask + 数据字节掩码            |
| CAN 收发       | 标准帧/扩展帧                        |
| 串口控制         | 文本命令或 JSON，115200 波特率          |
| TCP JSON 控制  | 以太网，IP 192.168.137.202，端口 5000 |
| 以太网 PHY      | LAN8720A，自动扫描+热插拔支持            |
| 外部 SRAM 自检   | FMC 接口，1MB，16bit               |
| 多任务保护        | printf 互斥锁、SPI 互斥锁             |

---

## 🔧 编译与下载

### 编译

```bash
mingw32-make -j4
```

| 参数    | 说明             |
| ----- | -------------- |
| `-j4` | 4 个线程并行编译，加快速度 |

生成文件：

```text
build/project_01.elf     # 调试用（带符号表）
build/project_01.hex     # 下载用（Intel HEX 格式）
build/project_01.bin     # 下载用（纯二进制）
```

### 下载

```bash
mingw32-make flash
```

实际执行的是：

```bash
STM32_Programmer_CLI -c port=SWD -w build/project_01.hex -v -rst
```

**需要提前安装：** STM32CubeProgrammer，并连接 ST-Link 调试器到板子的 SWD 接口。

### 启动确认

下载后打开串口（115200, 8N1），会看到：

```
[boot] project start
[boot] DAC81416 ready, VREF=2.5V internal, range=0-5V
[boot] voltage simulator ready, cells=13, normal=3200mV
[can] FDCAN1 started, classic CAN 500kbps
[rtos] defaultTask started
[tcp] JSON server listening on port 5000
```

**如果看不到这些日志：**

1. 检查串口波特率是否设置为 115200
2. 检查串口接线（TX→RX, RX→TX, GND→GND）
3. 检查板子是否上电
4. 检查 ST-Link 是否下载成功

---

## 💻 串口控制（最常用）

### 串口参数

| 参数  | 值             |
| --- | ------------- |
| 波特率 | 115200        |
| 数据位 | 8             |
| 校验  | 无             |
| 停止位 | 1             |
| 换行  | 回车（\r）或换行（\n） |

### 所有文本命令

| 命令                                                  | 作用                     |
| --------------------------------------------------- | ---------------------- |
| `help`                                              | 查看所有可用命令               |
| `status`                                            | 查看 13 路当前电压、故障状态、波形状态  |
| `normal [mv]`                                       | 设置全部电芯电压，不指定则 3200mV   |
| `set <cell> <mv>`                                   | 设置单路电压                 |
| `ov <cell> [mv] [dur] [slew]`                       | 单体过压                   |
| `uv <cell> [mv] [dur] [slew]`                       | 单体欠压                   |
| `diff <h_cell> <h_mv> <l_cell> <l_mv> [dur] [slew]` | 压差过大                   |
| `wave square <cell> <low> <high> <period>`          | 方波输出                   |
| `wave sine <cell> <offset> <amp> <period>`          | 正弦波输出                  |
| `wave sine all <offset> <amp> <period>`             | 13 路同步正弦波              |
| `wave high <cell> <freq> <points> <amp> <offset>`   | TIM3 节拍 + SPI DMA 高频正弦 |
| `wave status`                                       | 查看当前波形状态               |
| `wave stop`                                         | 停止波形                   |
| `cal status`                                        | 查看每路校准 offset          |
| `cal set <cell> <offset>`                           | 手动设置校准 offset          |
| `cal trim <cell> <target> <measured>`               | 按实测值自动计算 offset        |
| `cal clear [cell]`                                  | 清除校准                   |
| `test static <mv>`                                  | 全部输出固定电压，用于万用表测量       |
| `test slew <cell> <low> <high> <period>`            | 方波压摆率测试（同 wave square） |
| `can status`                                        | 查看 CAN 通信统计            |
| `can send <id_hex> <len> [data...]`                 | 发送 CAN 帧               |
| `safe status`                                       | 查看安全保护状态               |
| `safe stop [mv]`                                    | 紧急停止                   |
| `safe release`                                      | 解除急停                   |
| `emergency stop [mv]`                               | 同 safe stop            |
| `log status`                                        | 查看 BMS 响应记录            |
| `log clear`                                         | 清除响应记录                 |
| `log filter <std/ext> <id> [mask]`                  | 配置 CAN 响应帧过滤           |
| `log filter off`                                    | 关闭过滤                   |
| `log data <index> <mask> <value>`                   | 数据字节过滤                 |
| `log data clear`                                    | 清除数据过滤                 |
| `sram status`                                       | 查看 SRAM 状态             |
| `sram test [bytes]`                                 | 执行 SRAM 自检             |
| `clear [cell]`                                      | 清除故障                   |

### 命令参数速查

| 参数         | 说明            | 范围                           |
| ---------- | ------------- | ---------------------------- |
| `<cell>`   | 电芯通道号         | 1~13                         |
| `<mv>`     | 电压值（毫伏）       | 500~5000                     |
| `[dur]`    | 故障保持时间（毫秒）    | 任意正整数                        |
| `[slew]`   | 电压变化斜率（mV/ms） | 任意正整数                        |
| `<low>`    | 方波低电平（毫伏）     | 500~5000                     |
| `<high>`   | 方波高电平（毫伏）     | 500~5000                     |
| `<offset>` | 正弦波中心电压       | 500~5000                     |
| `<amp>`    | 正弦波幅值         | ≤2500（保证不超限）                 |
| `<period>` | 波形周期（毫秒）      | ≥ 2                          |
| `<freq>`   | 高频正弦目标频率（Hz） | 建议先从 1000 开始验证               |
| `<points>` | 高频正弦每周期点数     | 1~1024，推荐 20 或 50              |
| `<id_hex>` | CAN ID（十六进制）  | 标准帧 ≤ 0x7FF，扩展帧 ≤ 0x1FFFFFFF |
| `<len>`    | CAN 数据长度      | 0~8                          |

### 常用示例

```text
# 查看状态
status

# 设置全部为默认电压
normal

# 设置全部为 3500mV
normal 3500

# 单独设置第 5 路为 3300mV
set 5 3300

# 第 7 路过压到 4500mV，保持 100ms，斜率 3000mV/ms
ov 7 4500 100 3000

# 第 3 路欠压到 1000mV，保持 200ms，斜率 2000mV/ms
uv 3 1000 200 2000

# 制造压差：第 1 路 3500mV，第 2 路 2800mV，保持 200ms
diff 1 3500 2 2800 200 3000

# 第 1 路输出方波，1000~4000mV，周期 1 秒
wave square 1 1000 4000 1000

# 第 1 路输出正弦波，中心 2500mV，幅值 1000mV，周期 1 秒
wave sine 1 2500 1000 1000

# 13 路同步正弦波
wave sine all 2500 1000 1000

# 第 1 路输出高频正弦波，1kHz，50 点/周期，中心 2500mV，幅值 1000mV
wave high 1 1000 50 1000 2500

# 高频目标验证点：第 1 路 50kHz，20 点/周期
wave high 1 50000 20 1000 2500

# 停止波形
wave stop

# 急停
safe stop

# 解除急停
safe release

# 查看 CAN 状态
can status

# 发送标准帧 ID=0x123，8 字节数据
can send 123 8 01 02 03 04 05 06 07 08

# 清除所有故障
clear
```

### JSON 命令（串口和 TCP 通用）

```json
{"cmd":"status"}
{"cmd":"normal","mv":3200}
{"cmd":"set","cell":5,"mv":3300}
{"cmd":"ov","cell":7,"mv":4500,"duration_ms":100,"slew_mv_per_ms":3000}
{"cmd":"uv","cell":3,"mv":1000,"duration_ms":100,"slew_mv_per_ms":3000}
{"cmd":"diff","high_cell":1,"high_mv":3500,"low_cell":2,"low_mv":2800,"duration_ms":200,"slew_mv_per_ms":3000}
{"cmd":"wave","type":"square","cell":1,"low_mv":1000,"high_mv":4000,"period_ms":1000}
{"cmd":"wave","type":"sine","cell":1,"offset_mv":2500,"amplitude_mv":1000,"period_ms":1000}
{"cmd":"wave","type":"sine_all","offset_mv":2500,"amplitude_mv":1000,"period_ms":1000}
{"cmd":"wave","type":"high","cell":1,"freq_hz":1000,"points":50,"amplitude_mv":1000,"offset_mv":2500}
{"cmd":"wave_stop"}
{"cmd":"cal","action":"status"}
{"cmd":"cal","action":"set","cell":1,"offset_mv":15}
{"cmd":"cal","action":"trim","cell":1,"target_mv":2500,"measured_mv":2485}
{"cmd":"cal","action":"clear","cell":1}
{"cmd":"test","type":"static","mv":2500}
{"cmd":"test","type":"slew","cell":1,"low_mv":1000,"high_mv":4000,"period_ms":1000}
{"cmd":"can","action":"status"}
{"cmd":"can","action":"send","id":291,"len":8,"d0":1,"d1":2,"d2":3,"d3":4,"d4":5,"d5":6,"d6":7,"d7":8}
{"cmd":"can","action":"send","extended":1,"id":418385920,"len":2,"d0":170,"d1":85}
{"cmd":"safe","action":"status"}
{"cmd":"safe","action":"stop","mv":3200}
{"cmd":"safe","action":"release"}
{"cmd":"log","action":"status"}
{"cmd":"log","action":"clear"}
{"cmd":"log","action":"filter","type":"std","id":291}
{"cmd":"log","action":"filter","type":"ext","id":418316544,"mask":536870656}
{"cmd":"log","action":"data","index":0,"mask":255,"value":4}
{"cmd":"log","action":"data_clear"}
{"cmd":"log","action":"filter","type":"off"}
{"cmd":"sram","action":"status"}
{"cmd":"sram","action":"test","bytes":65536}
{"cmd":"clear"}
{"cmd":"clear","cell":7}
```

JSON 响应示例：

```json
{"ok":true,"cmd":"ov"}
{"ok":false,"cmd":"set"}
```

---

## 🌐 TCP JSON 网络控制

### 连接方法

| 项目    | 值               |
| ----- | --------------- |
| IP 地址 | 192.168.137.202 |
| 子网掩码  | 255.255.255.0   |
| 端口    | 5000            |
| 协议    | TCP，一行一个 JSON   |

### PC 设置

1. 把 PC 的以太网 IP 设为 **192.168.137.x**（如 192.168.137.10）
2. 子网掩码 **255.255.255.0**
3. 网关留空

### 连接测试

Windows 可以用 `nc`（需安装）或 Python：

```bash
# 方法 1：用 netcat
nc 192.168.137.202 5000

# 方法 2：用 Python 测试
python -c "
import socket
s = socket.socket()
s.connect(('192.168.137.202', 5000))
print(s.recv(1024))          # 接收欢迎信息
s.send(b'{\"cmd\":\"status\"}\n')
print(s.recv(4096))
s.close()
"
```

连接成功后板子会返回：

```json
{"ok":true,"service":"bms-fault-simulator","hint":"send one JSON command per line"}
```

### 完整操作示例

```bash
# 连接
nc 192.168.137.202 5000

# 按顺序输入以下命令（每行一个）：
{"cmd":"status"}
{"cmd":"normal","mv":3200}
{"cmd":"ov","cell":7,"mv":4500,"duration_ms":100,"slew_mv_per_ms":3000}
{"cmd":"safe","action":"stop"}
{"cmd":"safe","action":"release"}
```

---

## 🚗 CAN 通信调试

### 硬件参数

| 项目       | 配置          |
| -------- | ----------- |
| 外设       | FDCAN1      |
| 模式       | Classic CAN |
| 波特率      | 500kbps     |
| RX 引脚    | PA11        |
| TX 引脚    | PA12        |
| Rx FIFO0 | 8 帧         |
| Tx FIFO  | 8 帧         |

### 接线

```text
板子 CANH ──── USB-CAN 工具 CANH
板子 CANL ──── USB-CAN 工具 CANL

重要：总线两端各需要一个 120Ω 终端电阻
如果只是板子 ↔ USB-CAN 一对一，USB-CAN 通常内置了 120Ω
```

### 快速验证步骤

```text
第 1 步：板子上电，串口看到 [can] FDCAN1 started
第 2 步：PC 用 USB-CAN 工具发送一帧标准 CAN（任意 ID 和数据）
第 3 步：串口应打印 [can] rx std id=... len=... data=...
第 4 步：串口输入 can send 123 8 01 02 03 04 05 06 07 08
第 5 步：USB-CAN 工具应收到 ID=0x123 的标准帧
```

### CAN 日志解读

```
[can] rx std id=0x123 len=8 data=01 02 03 04 05 06 07 08
         │    │     │    │
         │    │     │    └── 数据（十六进制）
         │    │     └── 数据长度
         │    └── CAN ID（十六进制）
         └── 帧类型（std=标准帧, ext=扩展帧）
```

### 常见问题

| 问题           | 原因           | 解决                |
| ------------ | ------------ | ----------------- |
| 没看到 [can] 日志 | FDCAN1 初始化失败 | 检查 500kbps 配置是否正确 |
| 能发不能收        | 终端电阻问题       | 确认总线上有 120Ω       |
| 能收不能发        | TX/RX 接反     | 交换 CANH/CANL      |
| 乱码或丢帧        | 波特率不匹配       | 确认双方都是 500kbps    |

---

## 📐 DAC 输出调试教程

### 你需要准备的工具

- **万用表**（6.5 位更佳，普通 4 位也可）：测量静态电压精度
- **示波器**（100MHz 带宽足够）：测量动态响应和波形
- 导线若干

### 教程 1：静态输出测试（验证 DAC 硬件是否正常）

这是最基本的测试，确认 DAC 芯片能正确输出电压。

```text
步骤：
1. 万用表红表笔接 C01 输出端，黑表笔接 GND
2. 串口输入 test static 1000
3. 万用表读数应该是 1000mV（允许 ±5mV 误差）
4. 输入 test static 2500，读数应为 2500mV
5. 输入 test static 4500，读数应为 4500mV
6. 如果某一路偏差较大，用 cal trim 校准
```

**预期输出：**

```text
[cmd] test OK      # 如果成功
[cmd] test ERR     # 如果参数非法
```

### 教程 2：软件校准（补偿 DAC 输出误差）

如果万用表实测值偏离目标值，用此方法修正。

```text
场景：C01 设置 2500mV，万用表实测 2485mV（偏低 15mV）

方法 1：自动计算（推荐）
  输入 cal trim 1 2500 2485
  固件自动把 C01 的 offset 增加 15mV
  之后再测 2500mV，应该更接近目标值

方法 2：手动设置
  输入 cal set 1 15
  把 C01 offset 设为 +15mV

查看校准状态：
  输入 cal status
  显示每路的当前 offset 值

清除校准：
  输入 cal clear 1     # 清除第 1 路
  输入 cal clear       # 清除全部
```

### 教程 3：波形输出测试（验证功放电路）

如果你已经在 DAC 输出后面接了运放+MOSFET 功放电路，用此方法验证。

```text
接线：
  示波器 CH1 → DAC 输出端（功放前，作为对比）
  示波器 CH2 → 最终输出端（功放后）
  地线夹 → 电源 GND

测试方波：
  wave square 1 1000 4000 1000
  观察 CH2 波形是否在 1000mV 和 4000mV 之间切换
  检查上升沿是否有过冲（振铃）

测试正弦波：
  wave sine 1 2500 1000 1000
  观察 CH2 波形是否为正弦波
  检查是否有削顶（顶部/底部变平）
  检查是否平滑（无阶梯状）

压摆率测量：
  用示波器的 Rise Time 功能测 CH2 的 10%~90% 上升时间
  压摆率 = (4000-1000)×0.8 / 上升时间
  要求 ≥ 3V/ms，对应上升时间 ≤ 800μs
```

### 教程 4：13 路同步输出验证

```text
接线：
  把 13 路输出接到示波器的多个通道（或依次测量）

步骤：
  输入 wave sine all 2500 1000 1000
  所有 13 路应同时输出同相同步的正弦波
  用示波器比较任意两路，确认没有相位差

说明：当前 13 路是软件同步（1ms 内依次写完 13 路 DAC 后统一触发 LDAC）
```

---

## 🔥 故障注入教程

### 过压故障

```text
场景：第 7 路从 3200mV 升到 4500mV，保持 100ms，然后恢复

命令：ov 7 4500 100 3000
        │  │    │   │
        │  │    │   └── 斜率 3000mV/ms（约 0.43ms 从 3200 升到 4500）
        │  │    └── 保持时间 100ms
        │  └── 目标电压 4500mV
        └── 通道 7

用 status 实时观察第 7 路的电压变化：
  第一次 status：3200mV（注入前）
  第二次 status：4500mV（注入后，保持中）
  第三次 status：3200mV（恢复完成）
```

### 欠压故障

```text
场景：第 3 路从 3200mV 降到 1000mV，保持 200ms，然后恢复

命令：uv 3 1000 200 2000
        │  │    │   │
        │  │    │   └── 斜率 2000mV/ms（约 1.1ms 从 3200 降到 1000）
        │  │    └── 保持时间 200ms
        │  └── 目标电压 1000mV
        └── 通道 3
```

### 压差过大故障

```text
场景：第 1 路升到 3500mV，第 2 路降到 2800mV，制造 700mV 压差

命令：diff 1 3500 2 2800 200 3000
         │  │    │  │    │   │
         │  │    │  │    │   └── 斜率
         │  │    │  │    └── 持续时间
         │  │    │  └── 低压路电压
         │  │    └── 低压路通道
         │  └── 高压路电压
         └── 高压路通道
```

### 清除故障

```text
clear       # 清除所有路的故障
clear 7     # 只清除第 7 路的故障
```

---

## ⏱ BMS 响应时间测量教程

### 什么是 T1/T2/ΔT？

```text
T1 = 故障注入时刻（你执行 ov/uv/diff 的那一刻）
T2 = BMS 响应时刻（BMS 通过 CAN 发出告警帧的那一刻）
ΔT = T2 - T1（BMS 的响应延时，单位毫秒）
```

### 简单模式（不配置过滤）

```text
不配置任何过滤规则时，故障后的第一帧 CAN 就会被记录为 T2。

步骤：
  1. log clear            # 清除上次记录
  2. ov 7 4500 100 3000   # 注入故障（自动记录 T1）
  3. （BMS 或 USB-CAN 发送一帧 CAN）
  4. log status           # 查看结果
```

### 精确模式（配置 ID 过滤）

如果你的 BMS 在过压时发送 ID=0x351 的 CAN 帧，但总线上还有其他 CAN 帧干扰，你需要配置过滤规则。

```text
步骤：
  1. log filter std 351         # 只捕获 ID=0x351 的标准帧
  2. log clear                  # 清除上次记录
  3. ov 7 4500 100 3000         # 注入故障（自动记录 T1）
  4. （等待 BMS 发送 ID=0x351 的响应帧）
  5. log status                 # 查看 T1、T2、ΔT、响应帧数据
```

### 精确模式（配置 ID + 数据字节过滤）

如果 BMS 用同一个 ID 发多种告警，你需要区分具体是哪个告警。

```text
场景：ID=0x351 的帧，第 0 字节 = 0x04 表示过压告警

步骤：
  1. log filter std 351         # 只捕获 ID=0x351
  2. log data 0 FF 04           # 要求第 0 字节 == 0x04
  3. log clear
  4. ov 7 4500 100 3000
  5. log status
```

### log status 输出解读

```
[log] armed=1 trigger=OV t1=12345 primary=C07/4500mV ...
      filter enabled=1 type=std id=0x351 mask=0x7FF ...
      captured=1 t2=12456 delay=111ms id=0x351 ext=0 len=8 data=...

关键信息：
  captured=1          → 已捕获到响应
  delay=111ms         → BMS 响应延时 111ms
  t1=12345            → 故障注入时间 tick
  t2=12456            → 收到响应帧时间 tick
  id=0x351, data=...  → 响应帧的 ID 和数据
  filter_miss=5       → 有 5 帧不匹配过滤条件被丢弃
```

---

## 💾 外部 SRAM 自检

### 查看 SRAM 状态

```text
sram status
```

返回内容：

```
[sram] base=0x60000000 size=1048576bytes bus=16-bit last=OK ...
```

### 执行自检

```text
sram test            # 默认测试 64KB（快速验证）
sram test 1048576    # 测试全部 1MB（需要几秒钟）
```

**注意：** 自检会覆盖 SRAM 中的数据，如果 SRAM 已被用于日志缓存或其他用途，不要随意执行全片测试。

---

## ❓ 常见问题与排查

### 串口看不到启动日志

| 可能原因     | 解决方法                      |
| -------- | ------------------------- |
| 串口波特率不对  | 确认设置为 115200              |
| TX/RX 接反 | 交换 TX 和 RX 线              |
| 板子没上电    | 检查电源指示灯                   |
| 固件没下载成功  | 重新执行 `mingw32-make flash` |

### DAC 输出电压不对

| 现象      | 可能原因      | 检查                             |
| ------- | --------- | ------------------------------ |
| 所有路都不输出 | DAC 初始化失败 | 启动日志应有 `[boot] DAC81416 ready` |
| 某一路偏差大  | 需要校准      | 用 `cal trim` 补偿                |
| 输出漂移    | 参考电压不稳    | 检查 VREF 电路                     |
| 输出跳动    | SPI 通信干扰  | 检查 SPI 接线和屏蔽                   |

### 以太网连不上

| 现象                     | 解决                              |
| ---------------------- | ------------------------------- |
| 串口没显示 `[eth] link up`  | 检查网线是否插好，PHY 是否供电               |
| 能 link up 但 ping 不通    | 检查 PC IP 是否在同一网段（192.168.137.x） |
| ping 通但 nc 连不上 5000 端口 | 检查防火墙是否拦截 TCP 5000              |

### CAN 通信失败

| 现象          | 解决                 |
| ----------- | ------------------ |
| 没收到任何 CAN 帧 | 检查 120Ω 终端电阻       |
| 收到乱码数据      | 检查波特率是否一致（500kbps） |
| 发送失败        | 检查 TX 引脚是否正确（PA12） |

### 串口输入没反应

| 可能原因   | 解决方法                                 |
| ------ | ------------------------------------ |
| 换行符不对  | 确保发送的是回车或换行                          |
| 命令拼写错误 | 先输入 `help` 确认控制台正常                   |
| 急停锁存中  | 输入 `safe status` 检查，先 `safe release` |

---

## 📚 项目文档索引

| 文档               | 说明                      | 适合读者       |
| ---------------- | ----------------------- | ---------- |
| `流程图.md`         | Mermaid 流程图：架构、状态机、任务调度 | 所有人        |
| `项目分层架构与接口说明.md` | 完整架构、模块输入输出、状态机详解       | 开发人员、项目管理者 |
| `当前功能与使用说明.md`   | 详细功能列表、所有命令、验证流程        | 测试人员       |
| `技术方案.md`        | 原始技术方案                  | 项目管理者      |
| `zhinan.md`      | 运放+MOSFET 功放调试指南        | 硬件工程师      |

---

## 📝 代码质量改进记录

| 改进项          | 文件                             | 说明                   |
| ------------ | ------------------------------ | -------------------- |
| printf 互斥锁保护 | `Core/Src/main.c`、`freertos.c` | 多任务同时 printf 不输出交错   |
| SPI 互斥锁保护    | `BSP/Src/bsp_dac81416.c`       | 多任务访问 DAC 不冲突        |
| EthIf 任务栈扩容  | `LWIP/Target/ethernetif.c`     | 350→512 字节           |
| PHY 热插拔      | `LWIP/Target/ethernetif.c`     | 链路断开自动清除缓存           |
| UART 轮询让步    | `App/Src/fault_console.c`      | 每次轮询后 osDelay(1)     |
| DCLP 内存屏障    | `BSP/Src/bsp_dac81416.c`       | 添加 __DSB() 确保互斥锁可见性  |
| TX 超时减小      | `LWIP/Target/ethernetif.c`     | 2000ms→100ms 减少锁持有时间 |
| 高频波形引擎      | `App/Src/waveform_engine.c`    | TIM3 节拍触发每采样点 SPI DMA burst |

---

## ⚠️ 当前限制

| 项目       | 当前状态                    | 后续计划                 |
| -------- | ----------------------- | -------------------- |
| 低频波形刷新   | FreeRTOS 1ms 软件推进，适合慢速功能验证 | 保留当前实现 |
| 高频波形刷新   | `wave high` 使用 TIM3 中断节拍启动 SPI DMA burst；50kHz@20 点需示波器验证 CPU/中断余量 | 进一步改为 TIM/DMAMUX/硬件 CS-LDAC 全硬件触发 |
| 校准保存     | 保存在 RAM，掉电丢失            | 增加 Flash 存储          |
| 温度模拟     | 尚未实现                    | 需增加 NTC 电阻切换电路       |
| BMS 协议解析 | 手动配置过滤规则                | 拿到 DBC 后实现自动解析       |
| 上位机      | TCP JSON 可用             | 开发 PC 图形界面           |
| 网络       | 静态 IP，无 DHCP            | 可启用 DHCP             |

---

> 如有更多问题，请参考 `当前功能与使用说明.md` 获取更详细的命令说明和验证流程。
