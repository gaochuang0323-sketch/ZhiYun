Here's my完整的测试补充计划：
测试补充计划
第一阶段：激活现有草稿测试
文件    操作
_pending_fault_con.c    重命名为 test_fault_console.c + 修复 HAL_UART_Receive 冲突
_hang_bsp_can.c    重命名为 test_bsp_can.c + 补充
test_fault_console.c 需要处理的关键问题：stubs.c 中已有 HAL_UART_Receive 实现，与测试文件的 override 冲突。需将 stubs.c 的 HAL_UART_Receive 改为可被测试控制的版本（使用全局 buffer 机制）。
test_bsp_can.c 需补充：started==0 时拒绝发送、扩展帧 RX 回调验证。
第二阶段：新增模块测试

1. test_waveform_engine.c（~25 个测试）
   这是最大的覆盖缺口。当前整个 waveform_engine.c 被 test/support/waveform_engine.c 空桩替换，零覆盖。
   方案： 移除 test/support/waveform_engine.c stub，在所有测试中链接真实 App/Src/waveform_engine.c，补齐硬件 stubs。
   需要新增的硬件 stub（在 stubs.c/stubs.h 中）：
   函数
   HAL_RCC_GetPCLK1Freq()
   HAL_SPI_Transmit_DMA()
   HAL_DMA_Init() / HAL_DMA_Start_IT() / HAL_DMA_Abort() / HAL_DMA_DeInit()
   HAL_TIM_Base_Start() / HAL_TIM_Base_Start_IT() / HAL_TIM_Base_Stop() / HAL_TIM_Base_Stop_IT() / HAL_TIM_GenerateEvent()
   SPI_HandleTypeDef hspi1 定义 + DMA_HandleTypeDef hdma_spi1_tx
   TIM_HandleTypeDef htim3 定义
   RCC_TypeDef stub + D2CFGR 等寄存器
   还需在 test/support/main.h 中补充 BSP_SRAM_BASE_ADDRESS、DAC_CS_GPIO_Port 等宏定义。
   可测试的 API 函数：
   函数    测试内容
   WaveformEngine_Init    状态清零，多次调用安全
   WaveformEngine_GenerateSineTable    NULL/0/超1024→ERROR；有效值→正弦生成范围正确；边界裁剪（<500mV→500, >5000mV→5000）
   WaveformEngine_GetStatus    返回正确结构体
   WaveformEngine_Stop    未运行时调用不崩溃
   WaveformEngine_TimerElapsed    running=0直接返回；txBusy=1 error++；正常路径调 DMA
   HAL_SPI_TxCpltCallback    非SPI1忽略；IRQ_DMA模式→CS高+LDAC+txBusy=0
   HAL_SPI_ErrorCallback    SPI1→DmaError
   WaveformEngine_DmaError    errorCount++，停止硬件
2. test_bsp_dac81416.c（~20 个测试）
   测试 bsp_dac81416.c（SPI 传输层、互斥锁、GPIO 控制）。
   需要新增的 stub：HAL_SPI_Transmit、HAL_SPI_TransmitReceive、osMutexNew/osMutexAcquire/osMutexRelease、osKernelGetState 等。
   测试内容：
- BSP_DAC81416_Init → CS/Reset/LDAC 初始化为高
- DAC_SPI_Transmit → NULL/非3字节→ERROR；数据拼装正确
- DAC_SPI_TransmitReceive → NULL→ERROR；接收数据解析正确
- DAC_SPI_TransmitFrames24 → NULL/0帧→ERROR；CS时序正确
- DAC_SPI_TransmitReceiveFrames24 → NULL→ERROR
- BSP_DAC81416_SetTimeout/GetTimeout → 存取一致
- DAC_ResetHardware → 复位引脚先低后高
- DAC_LDAC_Update → LDAC 引脚脉冲
- DAC_ReadAlarmPin → 读取报警脚状态
- 互斥锁锁定/超时场景
3. test_bsp_sram.c（~12 个测试）
   测试 bsp_sram.c（SRAM 自检逻辑）。
   需要新增的 stub：用全局数组模拟 SRAM 地址空间（0x60000000），使写读验证可测试。
   测试内容：
- BspSram_Init → 基址/容量/总线宽度/状态正确
- BspSram_RunSelfTest → 0→默认64KB；超限→裁剪到1MB；奇数→向下对齐
- pattern 测试：address pattern、0x0000、0xFFFF、0xAAAA、0x5555
- 模拟写入失败→记录故障地址/期望值/实际值
- BspSram_GetStatus → 状态读取
- BspSram_GetResultName → 4种结果枚举名称
  第三阶段：扩展现有测试
  test_fault_command.c（当前21→35个测试）
  当前测试只检查了输出长度>0，需要补充：
- wave sine / wave sine all / wave high 命令
- diff、cal status/set/trim/clear、safe stop/release
- can send、sram status/test
- test static/slew 命令
- JSON 格式更多命令：{"cmd":"wave","type":"sine",...}
- 异常输入：参数不足、参数超范围、无效通道
  test_voltage_sim.c（当前36→40个测试）
- InjectCellOverVoltageRamp / InjectCellUnderVoltageRamp（带斜率参数）
- Process 中 ramp 爬坡过程的逐步验证
- 并发故障（同时多路 OV/UV）的状态检查
- DAC 写入失败时的错误处理
  test_dac_safety.c（当前13→16个测试）
- 已 emergency stop 时重复调用不触发副作用
- Release 在各种状态下的行为
- IsOutputAllowed 在未 Init 时返回正确值
  影响评估
  指标    当前    优化后
  运行测试文件数    5    8 (+3)
  总测试数    110    ~220 (+110)
  覆盖的源模块    5/12    8/12
  未覆盖模块    waveform_engine, bsp_dac81416, bsp_sram, tcp_json_server, bsp_eth    tcp_json_server, bsp_eth（依赖 LwIP，不适合单元测试）
  tcp_json_server.c 和 bsp_eth.c 因深度依赖 LwIP 网络协议栈和以太网 PHY 硬件，不适合 Ceedling 单元测试，建议通过集成测试验证。
  这个方案是否满足你的需求？有什么需要调整或优先实施的部分吗？如果确认没问题，我可以开始执行。
