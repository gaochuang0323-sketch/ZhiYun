# CAN 协议实现核对报告

对照 `docs/frame.csv`，逐项检查 `can_protocol.h` / `can_protocol.c` 的完成度。

## ✅ 已正确实现

| CSV ID | 名称 | 方向 | 代码状态 |
|--------|------|------|---------|
| 0x011 | PC_msg1 | Rx (PC→模拟器) | ✅ VolCh1~4_set_mv 全部实现 |
| 0x012 | PC_msg2 | Rx | ✅ VolCh5~8_set_mv 全部实现 |
| 0x013 | PC_msg3 | Rx | ✅ VolCh9~12_set_mv 全部实现 |
| 0x014 | PC_msg4 | Rx | ✅ VolCh13_set_mv + active + wave + estop 全部实现 |
| 0x015 | PC_msg5 | Rx | ✅ TempCh1~7_set + active 全部实现 |
| 0x016 | PC_msg6 | Rx | ✅ VolCh1~8_offset_set_mv 全部实现 |
| 0x017 | PC_msg7 | Rx | ✅ VolCh9~13_offset + code 已实现 |
| 0x01A | Replayer_msg2 | Tx (500ms) | ✅ VolCh1~4_back_mv 已实现 |
| 0x01B | Replayer_msg3 | Tx (500ms) | ✅ VolCh5~8_back_mv 已实现 |
| 0x01C | Replayer_msg4 | Tx (500ms) | ✅ VolCh9~12_back_mv 已实现 |
| 0x01D | Replayer_msg5 | Tx (500ms) | ✅ VolCh13_back + VolWave_back 已实现 |
| 0x01E | Replayer_msg6 | Tx (500ms) | ✅ TempCh1~7_back 已实现 |
| 0x01F | Replayer_msg7 | Tx (触发后500ms) | ✅ VolCh1~8_offset_back 已实现 |
| 0x020 | Replayer_msg8 | Tx (触发后500ms) | ✅ VolCh9~13_offset_back 已实现 |
| 0x021 | Replayer_msg9 | Tx (500ms) | ✅ VolWave_back + Estop_back + 故障码/状态码已实现 |

## ❌ 缺失或需要确认

### 1. 0x017 有两个帧共用一个 ID

CSV 中有两行使用 **同一个 ID 0x017**：
- **PC_msg7**：VolCh9~13_offset_set_mv（byte 0~4, byte 5 active, byte 6~7 code）— ✅ 已实现
- **PC_msg11**：Replayer_sever_IP1~4_set + Port_set（byte 0~3 IP, byte 4~5 port）— ❌ 未实现

> 同一个 ID 无法区分这两种不同用途的帧。需要确认：**这段 CAN 帧是给 EIS 回放器用的网络配置命令，BMS 模拟器是否需要处理？**

### 2. 0x018 PC_msg12 — 板卡 IP 设置

```
bit[0](8)  Replayer_IP1_set
bit[8](8)  Replayer_IP2_set
bit[16](8) Replayer_IP3_set
bit[24](8) Replayer_IP4_set
bit[32](8) Replayer_static_IP_flg_set
```

> 这也是 EIS 回放器的网络配置，**BMS 模拟器是否需要？**

### 3. 0x019 Replayer_msg1 — ASCII 码字符串响应

```
bit[0](8)  Replayer_ASCII_CODE1~8
```

> 字符串打印功能。**是否需要实现？**

### 4. 0x021 状态码字段说明

CSV 中 `0x021` 最后两个字段都叫 `Replayer_StatusCode`，可能是个笔误（第二个应是不同含义）。目前代码已预留故障码和状态码位置。

---

## 结论

**CAN 协议核心部分（电压设置/反馈、温度、偏移、波形、急停）已全部按要求实现。** 缺失的部分是 EIS 回放器的网络配置命令（IP 设置、ASCII 响应），这些可能需要确认是否属于本项目的范围。

**请回答：**
1. 0x017 中的 PC_msg11（服务器IP设置）是否需要实现？
2. 0x018 PC_msg12（板卡IP设置）是否需要实现？
3. 0x019 Replayer_msg1（ASCII码响应）是否需要实现？