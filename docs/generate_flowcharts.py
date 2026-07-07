#!/usr/bin/env python3
"""生成 ZhiYun BMS 故障模拟器项目架构流程图 — 使用 matplotlib"""

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch
import os

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "images")
os.makedirs(OUTPUT_DIR, exist_ok=True)

# 中文字体支持
plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

def save(name):
    path = os.path.join(OUTPUT_DIR, name)
    plt.savefig(path, dpi=150, bbox_inches='tight', pad_inches=0.3)
    print(f"  ✅ 已生成: {path}")
    plt.close()

# =====================================================
# 1. 软件分层架构图
# =====================================================
def make_architecture():
    fig, ax = plt.subplots(figsize=(10, 8))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 10)
    ax.axis('off')

    layers = [
        (8.5, "控制入口层\n串口命令 (fault_console)\nTCP JSON 服务 (tcp_json_server)\nCAN 通信 (bsp_can)", "#E3F2FD"),
        (6.8, "命令解析层 (fault_command)\n把文本或 JSON 翻译成业务动作", "#E8F5E9"),
        (5.1, "业务逻辑层（状态机都在这一层）\n电压模拟 · 波形生成 · 安全保护 · 响应记录", "#FFF3E0"),
        (3.4, "硬件驱动层\nDAC81416 驱动 · CAN BSP · ETH BSP · SRAM BSP", "#F3E5F5"),
        (1.7, "芯片外设层 (CubeMX 自动生成)\nSPI1 / GPIO / USART1 / FDCAN1 / ETH / FMC", "#ECEFF1"),
    ]

    for i, (y, label, color) in enumerate(layers):
        box = FancyBboxPatch((0.5, y-0.7), 9, 1.3, 
                             boxstyle="round,pad=0.1",
                             facecolor=color, edgecolor="#555555", linewidth=1.5)
        ax.add_patch(box)
        ax.text(5, y, label, ha='center', va='center', fontsize=10,
                fontweight='bold' if i < 2 else 'normal')

    # 箭头
    for y in [8.3, 6.6, 4.9, 3.2]:
        ax.annotate('', xy=(5, y-0.7), xytext=(5, y+0.7),
                    arrowprops=dict(arrowstyle='->', lw=2, color='#888888'))

    ax.set_title("ZhiYun BMS 故障模拟器 — 软件分层架构", fontsize=14, fontweight='bold', pad=20)
    save("01_architecture.png")

# =====================================================
# 2. 故障状态机
# =====================================================
def make_fault_sm():
    fig, ax = plt.subplots(figsize=(12, 4))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 4)
    ax.axis('off')

    states = [
        (1.5, "① 空闲\n(NONE)", "#E8F5E9"),
        (4.5, "② 故障上升/下降\n(RAMPING)\n按斜率逼近目标电压", "#FFF3E0"),
        (7.5, "③ 故障保持\n(HOLDING)\n保持目标电压不变", "#FFE0B2"),
        (10.5, "④ 故障恢复\n(RESTORING)\n按斜率回到原电压", "#C8E6C9"),
    ]

    for x, label, color in states:
        box = FancyBboxPatch((x-1.2, 0.8), 2.4, 2.5,
                             boxstyle="round,pad=0.1",
                             facecolor=color, edgecolor="#555555", linewidth=1.5)
        ax.add_patch(box)
        ax.text(x, 2.0, label, ha='center', va='center', fontsize=9)

    # 箭头
    arrows = [
        (2.7, "执行 ov/uv/diff", 'black'),
        (5.7, "到达目标值", 'black'),
        (8.7, "保持时间到", 'black'),
        (10.5, "", 'black'),  # restore -> none 直接画
        (3.0, "clear 命令", 'red'),
        (6.0, "clear 命令", 'red'),
        (9.0, "clear 命令", 'red'),
    ]
    
    # 正常转换箭头
    for i, (x, lbl, col) in enumerate(arrows[:3]):
        ax.annotate('', xy=(x+1.3, 2.0), xytext=(x-0.2, 2.0),
                    arrowprops=dict(arrowstyle='->', lw=2, color=col))
        if lbl:
            ax.text(x+0.4, 2.4, lbl, ha='center', fontsize=7, color=col)

    ax.set_title("故障状态机（每路电芯独立运行，共 13 个实例）", fontsize=12, fontweight='bold')
    save("02_fault_state_machine.png")

# =====================================================
# 3. 安全状态机
# =====================================================
def make_safety_sm():
    fig, ax = plt.subplots(figsize=(10, 3.5))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 3.5)
    ax.axis('off')

    states = [
        (2, "① 正常运行\n(NORMAL)\n每 50ms 监测 nALMOUT", "#E8F5E9"),
        (5, "② 急停锁存\n(EMERGENCY)\n停止波形·清除故障·设安全电压\n拒绝所有输出命令", "#FFCDD2"),
        (8, "③ 释放\n(RELEASE)\n恢复正常控制", "#FFF9C4"),
    ]

    for x, label, color in states:
        box = FancyBboxPatch((x-1.2, 0.5), 2.4, 2.5,
                             boxstyle="round,pad=0.1",
                             facecolor=color, edgecolor="#555555", linewidth=1.5)
        ax.add_patch(box)
        ax.text(x, 1.8, label, ha='center', va='center', fontsize=9)

    ax.annotate('', xy=(4, 1.8), xytext=(2.7, 1.8),
                arrowprops=dict(arrowstyle='->', lw=2.5, color='red'))
    ax.text(3.3, 2.2, "nALMOUT=低电平\n(DAC 报警)", ha='center', fontsize=8, color='red')

    ax.annotate('', xy=(7, 1.8), xytext=(5.7, 1.8),
                arrowprops=dict(arrowstyle='->', lw=2, color='black'))
    ax.text(6.3, 2.2, "用户 safe release", ha='center', fontsize=8)

    ax.annotate('', xy=(9.5, 1.8), xytext=(9.5, 0.3),
                arrowprops=dict(arrowstyle='->', lw=1.5, color='gray', linestyle='dashed'))

    ax.set_title("DAC 安全状态机", fontsize=12, fontweight='bold')
    save("03_safety_state_machine.png")

# =====================================================
# 4. PHY 状态机
# =====================================================
def make_phy_sm():
    fig, ax = plt.subplots(figsize=(12, 7))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 7)
    ax.axis('off')

    states = [
        (2, 5.5, "① 扫描 PHY\n每 100ms 扫描 MDIO 地址 0~31\n读取 PHY ID1 寄存器", "#E3F2FD"),
        (2, 3.5, "② 软件复位\n写 BMCR 复位位\n每 10ms 检查，超时 500ms", "#E8F5E9"),
        (6, 5.5, "③ 链路检测\n每 100ms 读取 BSR\n双重读取避 latch-low", "#FFF3E0"),
        (10, 5.5, "④ 链路已连接\n配置 MAC 速率/双工\n启动 DMA，通知 LwIP", "#C8E6C9"),
        (6, 3.5, "⑤ 链路断开\n停止 DMA\n清除 PHY 缓存", "#FFCDD2"),
    ]

    for x, y, label, color in states:
        box = FancyBboxPatch((x-1.8, y-0.8), 3.6, 1.6,
                             boxstyle="round,pad=0.1",
                             facecolor=color, edgecolor="#555555", linewidth=1.5)
        ax.add_patch(box)
        ax.text(x, y, label, ha='center', va='center', fontsize=8)

    # 箭头
    ax.annotate('', xy=(6.5, 5.5), xytext=(4, 5.5),
                arrowprops=dict(arrowstyle='->', lw=2, color='black'))
    ax.text(5.2, 5.8, "找到 PHY", ha='center', fontsize=8)
    
    ax.annotate('', xy=(2, 5.5), xytext=(2, 4.8),
                arrowprops=dict(arrowstyle='->', lw=2, color='black'))
    ax.text(2.4, 5.1, "复位完成", fontsize=8)
    
    ax.annotate('', xy=(2, 3.2), xytext=(2, 2.7),
                arrowprops=dict(arrowstyle='->', lw=1.5, color='red'))
    ax.text(2.6, 3.0, "超时", fontsize=8, color='red')
    
    ax.annotate('', xy=(8, 2.7), xytext=(7, 2.7),
                arrowprops=dict(arrowstyle='->', lw=1.5, color='black'))
    ax.annotate('', xy=(7, 2.7), xytext=(6, 3.2),
                arrowprops=dict(arrowstyle='->', lw=1.5, color='black'))
    ax.text(7, 2.4, "链路 down", ha='center', fontsize=8, color='red')

    ax.annotate('', xy=(9, 5.5), xytext=(7.5, 5.5),
                arrowprops=dict(arrowstyle='->', lw=2, color='black'))
    ax.text(8.2, 5.8, "链路 up", ha='center', fontsize=8)

    ax.annotate('', xy=(10, 4.5), xytext=(10, 4.2),
                arrowprops=dict(arrowstyle='->', lw=1.5, color='red'))
    ax.text(10.5, 4.4, "断开", fontsize=8, color='red')

    ax.set_title("PHY 状态机（以太网物理层生命周期管理）", fontsize=12, fontweight='bold')
    save("04_phy_state_machine.png")

# =====================================================
# 5. 命令处理流程
# =====================================================
def make_cmd_flow():
    fig, ax = plt.subplots(figsize=(10, 8))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 8)
    ax.axis('off')

    nodes = [
        (5, 7.3, "串口收到 \"ov 7 4500 100 3000\"\n或 TCP 收到 JSON", "#E3F2FD", 2),
        (5, 6.0, "FaultCommand_ExecuteLine() 统一入口\n判断文本还是 JSON → 解析参数", "#E8F5E9", 2),
        (5, 4.7, "安全状态机是否在急停中？", "#FFF9C4", 1.5),  # diamond-ish
        (7.5, 3.5, "急停中 → 返回拒绝", "#FFCDD2", 1.5),
        (2.5, 3.5, "正常 → 停止波形\n设置第7路故障状态机\n(type=OV,target=4500)", "#C8E6C9", 2),
        (2.5, 2.2, "自动记录 T1\n(bms_response_log)", "#FFF3E0", 2),
        (2.5, 0.8, "defaultTask 下一次 1ms 循环\n故障状态机逐步升压", "#E1BEE7", 2.5),
    ]

    for x, y, label, color, w in nodes:
        if w == 1.5:
            box = FancyBboxPatch((x-1.2, y-0.5), 2.4, 1.0,
                                 boxstyle="round,pad=0.15",
                                 facecolor=color, edgecolor="#555555", linewidth=1.5)
        else:
            box = FancyBboxPatch((x-w/2, y-0.5), w, 1.0,
                                 boxstyle="round,pad=0.1",
                                 facecolor=color, edgecolor="#555555", linewidth=1.5)
        ax.add_patch(box)
        ax.text(x, y, label, ha='center', va='center', fontsize=8.5)

    # arrows
    ax.annotate('', xy=(5, 6.6), xytext=(5, 6.3), arrowprops=dict(arrowstyle='->', lw=1.5))
    ax.annotate('', xy=(5, 5.3), xytext=(5, 5.0), arrowprops=dict(arrowstyle='->', lw=1.5))
    ax.annotate('', xy=(7, 4.2), xytext=(7.5, 3.8), arrowprops=dict(arrowstyle='->', lw=1.5, color='red'))
    ax.text(7.2, 4.3, "是", fontsize=8, color='red')
    ax.annotate('', xy=(4, 4.2), xytext=(3, 3.8), arrowprops=dict(arrowstyle='->', lw=1.5))
    ax.text(3.8, 4.3, "否", fontsize=8)
    ax.annotate('', xy=(2.5, 3.0), xytext=(2.5, 2.5), arrowprops=dict(arrowstyle='->', lw=1.5))
    ax.annotate('', xy=(2.5, 1.7), xytext=(2.5, 1.3), arrowprops=dict(arrowstyle='->', lw=1.5))

    # note
    ax.text(7.5, 1.5, "设计原则：\n写入配置 · 下次循环生效\n收到命令只修改参数\n实际电压由状态机驱动", 
            fontsize=7, style='italic', color='#666666',
            bbox=dict(boxstyle="round,pad=0.5", facecolor="#F5F5F5", edgecolor="#CCCCCC"))

    ax.set_title("命令处理流程", fontsize=13, fontweight='bold')
    save("05_command_flow.png")

# =====================================================
# 6. 安全保护路径
# =====================================================
def make_safety_flow():
    fig, ax = plt.subplots(figsize=(8.5, 6))
    ax.set_xlim(0, 8.5)
    ax.set_ylim(0, 6)
    ax.axis('off')

    nodes = [
        (4.2, 5.2, "DAC 芯片检测到异常\nnALMOUT 引脚变低电平", "#FFCDD2"),
        (4.2, 4.0, "安全状态机：正常 → 报警转换\n自动触发 EmergencyStop()", "#FFE0B2"),
        (4.2, 2.8, "① 停止波形\n② 清除所有故障\n③ 13 路设为安全电压 (3200mV)\n④ 急停锁存 (emergencyActive=1)", "#FFF3E0"),
        (4.2, 1.6, "急停锁存生效\n所有改变电压的命令被拒绝\nDacSafety_IsOutputAllowed() = 0", "#F5F5F5"),
        (4.2, 0.5, "用户手动 safe release\n→ 恢复正常控制", "#C8E6C9"),
    ]

    for x, y, label, color in nodes:
        box = FancyBboxPatch((x-2.5, y-0.5), 5, 1.0,
                             boxstyle="round,pad=0.1",
                             facecolor=color, edgecolor="#555555", linewidth=1.5)
        ax.add_patch(box)
        ax.text(x, y, label, ha='center', va='center', fontsize=9)

    for i in range(len(nodes)-1):
        ax.annotate('', xy=(4.2, nodes[i][1]-0.5), xytext=(4.2, nodes[i+1][1]+0.5),
                    arrowprops=dict(arrowstyle='->', lw=1.5, color='red' if i == 0 else 'black'))

    ax.annotate('', xy=(4.2, 2.2), xytext=(4.2, 2.2),
                arrowprops=dict(arrowstyle='->', lw=1.5, color='black'))

    ax.set_title("DAC 安全保护完整路径", fontsize=13, fontweight='bold')
    save("06_safety_flow.png")

# =====================================================
# 7. BMS 响应记录
# =====================================================
def make_bms_flow():
    fig, ax = plt.subplots(figsize=(10, 7))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 7)
    ax.axis('off')

    nodes = [
        (5, 6.3, "用户执行 ov 7 4500\n故障注入成功", "#E3F2FD"),
        (5, 5.2, "bms_response_log.RecordFaultTrigger()\n记录 T1 = 当前时间", "#E8F5E9"),
        (5, 4.1, "BMS 检测到过压\n通过 CAN 总线发出告警帧", "#FFF3E0"),
        (5, 3.0, "bsp_can 收到 CAN 帧\nBspCan_OnRxFrame() 回调", "#E1BEE7"),
        (3, 2.0, "不匹配过滤规则\n→ 丢弃，filter_miss +1", "#FFCDD2"),
        (7, 2.0, "匹配过滤规则\n→ 记录 T2，保存响应帧 ID/数据\n→ 计算 ΔT = T2 - T1", "#C8E6C9"),
        (7, 1.0, "用户执行 log status\n查询响应记录", "#F5F5F5"),
    ]

    for x, y, label, color in nodes:
        box = FancyBboxPatch((x-2.2, y-0.4), 4.4, 0.8,
                             boxstyle="round,pad=0.1",
                             facecolor=color, edgecolor="#555555", linewidth=1.5)
        ax.add_patch(box)
        ax.text(x, y, label, ha='center', va='center', fontsize=8)

    # 主路径
    for i in range(3):
        ax.annotate('', xy=(5, nodes[i][1]-0.4), xytext=(5, nodes[i+1][1]+0.4),
                    arrowprops=dict(arrowstyle='->', lw=1.5))
    
    # 分叉
    ax.annotate('', xy=(4.5, 2.7), xytext=(3.5, 2.3),
                arrowprops=dict(arrowstyle='->', lw=1.5, color='red'))
    ax.annotate('', xy=(5.5, 2.7), xytext=(6.5, 2.3),
                arrowprops=dict(arrowstyle='->', lw=1.5))
    
    ax.annotate('', xy=(7, 1.6), xytext=(7, 1.3),
                arrowprops=dict(arrowstyle='->', lw=1.5))

    ax.set_title("BMS 响应时间记录完整路径", fontsize=13, fontweight='bold')
    save("07_bms_flow.png")

# =====================================================
# 8. 启动流程
# =====================================================
def make_boot_flow():
    fig, ax = plt.subplots(figsize=(10, 8))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 8)
    ax.axis('off')

    nodes = [
        (5, 7.5, "上电复位", "#E3F2FD", "ellipse"),
        (5, 6.5, "芯片底层初始化\nHAL_Init / 时钟配置 / MPU 配置", "#E8F5E9", "box"),
        (5, 5.5, "外设初始化\nGPIO / FMC(SRAM) / SPI1 / USART1 / FDCAN1", "#E8F5E9", "box"),
        (5, 4.3, "应用模块初始化\n① DAC81416 (配置 0~5V 量程)\n② 电压模拟 (13 路 3200mV)\n③ DAC 安全保护\n④ CAN 通信\n⑤ 串口控制台", "#FFF3E0", "box"),
        (5, 3.0, "启动 FreeRTOS 调度器", "#E1BEE7", "box"),
        (5, 1.5, "各任务开始运行\n\n• defaultTask: 初始化 LwIP → TCP 服务 → 1ms 循环\n  (开始周期运行所有状态机)\n• EthIf: 等待以太网数据\n• TcpJson: 等待 TCP 客户端\n• ethernet_link_thread: PHY 状态机开始扫描", "#F5F5F5", "box"),
    ]

    for x, y, label, color, shape in nodes:
        if shape == "ellipse":
            box = FancyBboxPatch((x-1.0, y-0.4), 2, 0.8,
                                 boxstyle="round,pad=0.2",
                                 facecolor=color, edgecolor="#555555", linewidth=2)
        else:
            box = FancyBboxPatch((x-3.0, y-0.5), 6, 1.0,
                                 boxstyle="round,pad=0.1",
                                 facecolor=color, edgecolor="#555555", linewidth=1.5)
        ax.add_patch(box)
        ax.text(x, y, label, ha='center', va='center', fontsize=8.5)

    for i in range(len(nodes)-1):
        ax.annotate('', xy=(5, nodes[i][1]-0.5 if nodes[i][3]!="ellipse" else nodes[i][1]-0.4), 
                    xytext=(5, nodes[i+1][1]+0.5),
                    arrowprops=dict(arrowstyle='->', lw=1.5))

    ax.set_title("系统启动流程", fontsize=13, fontweight='bold')
    save("08_boot_flow.png")

# =====================================================
# 9. 数据流总览
# =====================================================
def make_data_flow():
    fig, ax = plt.subplots(figsize=(11, 6))
    ax.set_xlim(0, 11)
    ax.set_ylim(0, 6)
    ax.axis('off')

    # 输入区
    ax.text(1.5, 5.3, "外部输入", ha='center', fontsize=10, fontweight='bold', color='#1565C0')
    for i, (x, label, color) in enumerate([(0.5, "串口命令\n(USART1)", "#BBDEFB"), 
                                             (1.5, "TCP JSON\n(端口 5000)", "#BBDEFB"),
                                             (2.5, "CAN 帧\n(FDCAN1)", "#BBDEFB")]):
        box = FancyBboxPatch((x-0.45, 4.5), 0.9, 0.7,
                             boxstyle="round,pad=0.05",
                             facecolor=color, edgecolor="#555555")
        ax.add_patch(box)
        ax.text(x, 4.85, label, ha='center', va='center', fontsize=7)

    # 命令解析
    box = FancyBboxPatch((3.5, 4.2), 3, 1.2,
                         boxstyle="round,pad=0.1",
                         facecolor="#E8F5E9", edgecolor="#555555", linewidth=1.5)
    ax.add_patch(box)
    ax.text(5, 4.8, "fault_command\n命令解析器\n文本/JSON → 业务动作", ha='center', va='center', fontsize=9)

    # 业务模块
    ax.text(8.5, 5.3, "业务逻辑模块", ha='center', fontsize=10, fontweight='bold', color='#E65100')
    biz = [(7, 4.2, "voltage_sim\n电压模拟\n(13 路故障状态机)", "#FFF3E0"),
           (8.5, 4.2, "waveform_gen\n波形生成\n(波形状态机)", "#FFF3E0"),
           (10, 4.2, "dac_safety\n安全保护\n(安全状态机)", "#FFF3E0"),
           (9, 2.8, "bms_response_log\n响应时间记录", "#FFF3E0")]
    for x, y, label, color in biz:
        box = FancyBboxPatch((x-0.8, y-0.5), 1.6, 1.0,
                             boxstyle="round,pad=0.05",
                             facecolor=color, edgecolor="#555555")
        ax.add_patch(box)
        ax.text(x, y, label, ha='center', va='center', fontsize=7)

    # 硬件输出
    for x, label in [(3, 1.5, "DAC81416\n13 路电压输出"),
                      (8, 1.5, "CAN 总线\n与 BMS 通信")]:
        box = FancyBboxPatch((x-1, 1.0), 2, 1.0,
                             boxstyle="round,pad=0.1",
                             facecolor="#F3E5F5", edgecolor="#555555")
        ax.add_patch(box)
        ax.text(x, 1.5, label, ha='center', va='center', fontsize=8)

    # 箭头
    ax.annotate('', xy=(4, 3.8), xytext=(4, 3.0), arrowprops=dict(arrowstyle='->', lw=1.5))
    ax.annotate('', xy=(7.5, 3.7), xytext=(7.5, 3.0), arrowprops=dict(arrowstyle='->', lw=1.5))
    ax.annotate('', xy=(8.5, 3.7), xytext=(8.5, 3.0), arrowprops=dict(arrowstyle='->', lw=1.5))
    
    ax.annotate('', xy=(3, 2.3), xytext=(3, 2.0), arrowprops=dict(arrowstyle='->', lw=1.5))
    ax.annotate('', xy=(8, 2.3), xytext=(8, 2.0), arrowprops=dict(arrowstyle='->', lw=1.5))
    
    # 输入到解析
    ax.annotate('', xy=(3.5, 4.8), xytext=(3, 4.8),
                arrowprops=dict(arrowstyle='->', lw=1.5))
    ax.annotate('', xy=(3.5, 5.0), xytext=(2.5, 5.0),
                arrowprops=dict(arrowstyle='->', lw=1.5))
    ax.annotate('', xy=(3.5, 5.2), xytext=(2, 5.2),
                arrowprops=dict(arrowstyle='->', lw=1, color='green'))
    
    # 解析到业务
    for dx in [7, 8.5, 10]:
        ax.annotate('', xy=(dx, 4.2), xytext=(6.5, 4.2),
                    arrowprops=dict(arrowstyle='->', lw=1.5))

    ax.set_title("模块数据流总览", fontsize=13, fontweight='bold')
    save("09_data_flow_overview.png")

# =====================================================
# 10. defaultTask 主循环
# =====================================================
def make_default_loop():
    fig, ax = plt.subplots(figsize=(10, 7))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 7)
    ax.axis('off')

    ax.text(5, 6.6, "defaultTask 主循环（每 1ms 执行一次）", ha='center', fontsize=13, fontweight='bold',
            bbox=dict(boxstyle="round,pad=0.3", facecolor="#E8F5E9", edgecolor="#2E7D32", linewidth=2))

    steps = [
        (5, 5.5, "Step 1: VoltageSim.Process()\n运行 13 路故障状态机\n计算每路当前应输出的电压", "#C8E6C9"),
        (5, 4.4, "Step 2: WaveformGen.Process()\n运行波形状态机\n计算当前采样点电压", "#C8E6C9"),
        (5, 3.3, "Step 3: DacSafety.Process()\n运行安全状态机\n读取 nALMOUT 引脚", "#C8E6C9"),
        (5, 2.2, "Step 4: FaultConsole.Process()\n检查串口是否有新输入", "#C8E6C9"),
        (5, 1.1, "Step 5: BspCan.Process()\n检查 FDCAN1 的 RX FIFO", "#C8E6C9"),
    ]

    for x, y, label, color in steps:
        box = FancyBboxPatch((x-3, y-0.35), 6, 0.7,
                             boxstyle="round,pad=0.05",
                             facecolor=color, edgecolor="#555555", linewidth=1)
        ax.add_patch(box)
        ax.text(x, y, label, ha='center', va='center', fontsize=8)

    for i in range(len(steps)-1):
        ax.annotate('', xy=(5, steps[i][1]-0.35), xytext=(5, steps[i+1][1]+0.35),
                    arrowprops=dict(arrowstyle='->', lw=1.5))

    ax.text(5, 0.4, "每 1000ms 额外执行一次：心跳 LED 翻转 + 打印运行日志", ha='center', fontsize=9,
            style='italic', color='#666666')

    ax.set_title("defaultTask 主循环（所有状态机都在这里运行）", fontsize=12, fontweight='bold')
    save("10_default_loop.png")


# =====================================================
if __name__ == "__main__":
    print("正在生成流程图...")
    make_architecture()
    make_fault_sm()
    make_safety_sm()
    make_phy_sm()
    make_cmd_flow()
    make_safety_flow()
    make_bms_flow()
    make_boot_flow()
    make_data_flow()
    make_default_loop()
    print(f"\n所有流程图已生成到: {OUTPUT_DIR}")