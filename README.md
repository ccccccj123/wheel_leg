# 轮腿机器人复刻项目

这是一个轮腿机器人复刻工程资料仓库，包含主控固件、FOC 电机驱动固件、MATLAB 建模与代码生成文件、硬件电路文件以及 SolidWorks 结构件。项目核心控制思路是：ESP32 主控通过 IMU、CAN 电机反馈和腿部运动学计算机器人状态，再使用 LQR、PID 与 VMC 生成轮电机和关节电机的扭矩/电压指令；底层 STM32 FOC 板接收 CAN 电压指令，完成无刷电机闭环驱动并回传角度和速度。

## 目录结构

```text
.
├── ESP32_WROOM/      # ESP32 主控程序，PlatformIO + Arduino 框架
├── FOC/              # STM32F103C6 + AS5600 FOC 电机驱动工程与立创 EDA 文件
├── Matlab/           # 腿部运动学、VMC、LQR 建模脚本和 MATLAB Coder 输出
├── CTRL/             # ESP32 主控板原理图/PCB JSON 文件
└── SW/               # SolidWorks 结构件和调参参考图片
```

## 主要功能

- ESP32 主控负责系统初始化、传感器读取、CAN 通信、电机指令下发和姿态控制。
- STM32 FOC 节点负责读取 AS5600 编码器、运行 FOC、接收 CAN 目标电压并回传状态。
- MATLAB 脚本用于生成腿部正运动学、速度雅可比、VMC 力矩转换和腿长相关 LQR 反馈矩阵。
- 硬件资料包含主控板和 FOC 板的立创 EDA 标准版文件。
- 结构资料包含车轮、大小腿、底板和适配件等 SolidWorks 零件。

## ESP32 主控固件

工程位置：`ESP32_WROOM/`

开发环境：

- PlatformIO
- Arduino framework
- 开发板配置：`esp32doit-devkit-v1`
- 主要依赖：
  - `electroniccats/MPU6050`
  - `adafruit/Adafruit ADS1X15`
  - `SPI`

入口文件是 `ESP32_WROOM/src/main.cpp`。初始化顺序如下：

1. `Serial_Init()`：串口调试输出。
2. `CAN_Init()`：初始化 ESP32 TWAI/CAN，总线速率 1 Mbps。
3. `IMU_Init()`：初始化 MPU6050 DMP，读取姿态角、角速度和 Z 轴加速度。
4. `Motor_InitAll()`：初始化 6 个电机对象并启动电机指令发送任务。
5. `Legs_Init()`：根据关节角度计算左右腿腿长、腿角和速度。
6. `Ctrl_Init()`：启动目标更新和基础平衡控制任务。

主控模块说明：

| 模块 | 文件 | 说明 |
| --- | --- | --- |
| CAN | `src/can.cpp` | 使用 ESP32 TWAI，接收电机反馈，向左右侧 FOC 板发送电压指令。 |
| Motor | `src/motor.cpp` | 管理 4 个关节电机和 2 个轮电机，做反电动势补偿、电压限幅和 CAN 打包。 |
| IMU | `src/imu.cpp` | 读取 MPU6050 DMP 姿态，维护 yaw 连续角。 |
| Legs | `src/legs.cpp` | 调用 MATLAB 生成函数计算腿部姿态和速度。 |
| Ctrl | `src/ctrl.cpp` | 4 ms 周期控制任务，包含 LQR、PID、VMC 和 yaw/roll/腿长控制。 |
| BLE | `src/ble.cpp` | 预留蓝牙遥控入口，可接收速度、转向速度和腿长目标。当前 `main.cpp` 中未启用。 |
| ADC | `src/adc.cpp` | 使用 ADS1115 读取电池电压，并根据电压调整电机输出比例。当前 `main.cpp` 中未启用。 |
| PID | `src/pid.cpp` | 单级 PID 和串级 PID 工具函数。 |

### ESP32 构建和烧录

进入主控工程目录：

```bash
cd ESP32_WROOM
pio run
pio run -t upload
pio device monitor -b 115200
```

如果使用 VS Code，可以安装 PlatformIO 插件后直接打开 `ESP32_WROOM/`。

## FOC 电机驱动

工程位置：`FOC/C6T6_AS5600/`

硬件和软件特征：

- MCU：STM32F103C6
- 编码器：AS5600
- 电机控制：SimpleFOC 风格的简化实现
- 通信：CAN + USART1 调试
- 工程文件：
  - `C6T6_AS5600.ioc`：STM32CubeMX 配置
  - `MDK-ARM/C6T6_AS5600.uvprojx`：Keil MDK 工程
  - `CMakeLists.txt`：CMake 工程描述

FOC 主循环在 `Core/Src/main.c` 中：

```c
setTargetVotage(targetVotage);
loopFOC();
```

其中 `targetVotage` 来自 CAN 或串口调试命令，最终进入 FOC q 轴电压输出。

关键模块：

| 模块 | 文件 | 说明 |
| --- | --- | --- |
| CAN 协议 | `Motor/mission_can.c` | 接收主控电压指令，回传角度和速度，500 ms 离线保护。 |
| 周期任务 | `Motor/mission_loop.c` | 心跳灯、离线检测、角度滤波、速度估算和状态回传。 |
| 串口调试 | `Motor/mission_uart.c` | 支持设置电机 ID、擦除 Flash、重启、直接设置目标电压和查询电角度。 |
| FOC 核心 | `SimpleFOC/` | BLDC、电角度、SVPWM/FOC 相关实现。 |
| 编码器 | `SimpleFOC/as5600.c` | AS5600 角度读取。 |
| 参数存储 | `Motor/FlashStorage.c` | 保存电机 ID 和标定参数。 |

### FOC 串口调试命令

串口波特率：`115200`

常用命令：

```text
setid:3      # 设置当前 FOC 板电机 ID 为 3，写入 Flash 后自动复位
erase        # 擦除 Flash 参数
reboot       # 软件复位
vot:1000     # 设置目标电压为 1000 mV
angle        # 输出当前电角度
```

### CAN 通信约定

主控发送：

- `0x100`：控制 ID 1~4 的电机。
- `0x200`：控制 ID 5~8 的电机。
- 每台电机占 2 字节，类型为 `int16_t`，单位为 mV。

FOC 回传：

- 回传 ID：`0x100 + motorID`。
- byte 0~3：角度，`int32_t`，单位为 `rad * 1000`。
- byte 4~5：速度，`int16_t`，单位为 `rpm * 10`。

ESP32 当前使用的反馈 ID 映射：

| CAN ID | 电机 |
| --- | --- |
| `0x101` | 左腿关节 0 |
| `0x102` | 左腿关节 1 |
| `0x103` | 左轮 |
| `0x105` | 右腿关节 0 |
| `0x106` | 右腿关节 1 |
| `0x107` | 右轮 |

ESP32 当前发送的控制帧：

- `0x100`：左侧两个关节电机和左轮。
- `0x200`：右侧两个关节电机和右轮。

## MATLAB 建模与代码生成

工程位置：`Matlab/`

核心脚本：

| 文件 | 说明 |
| --- | --- |
| `leg_function.m` | 根据五连杆几何关系生成腿长、腿角、腿部速度和 VMC 转换函数。 |
| `system_calculate.m` | 建立轮腿系统状态空间模型，离散化后求解 LQR，并对不同腿长的反馈矩阵做三次多项式拟合。 |
| `out_leg_position.m` | 腿部正运动学输出。 |
| `out_leg_speed.m` | 腿部速度雅可比输出。 |
| `out_leg_vmc_conv.m` | 将虚拟腿推力和髋关节扭矩转换为两关节电机扭矩。 |
| `out_lqr_k.m` | 根据当前腿长输出 2x6 LQR 反馈矩阵。 |

`Matlab/codegen/` 中包含 MATLAB Coder 生成的 C 代码。ESP32 工程中已拷贝一份到：

```text
ESP32_WROOM/include/matlab_code/
ESP32_WROOM/src/matlab_code/
```

如果修改 MATLAB 模型，需要重新生成 C 代码，并同步更新 ESP32 工程中的 `matlab_code` 文件。

## 硬件资料

### 主控板

目录：`CTRL/`

- `SCH_ESP32_Control_2024-11-08.json`
- `PCB_PCB_ESP32_Control_2024-11-08.json`

这部分是 ESP32 主控板的立创 EDA 工程文件。

### FOC 板

目录：`FOC/`

- `SCH_FocNano_C6T6_AS5600_2024-11-08.json`
- `PCB_PCB_2024-11-08.json`

`FOC/PCB为立创EDA标准版绘制.txt` 表明该 PCB 使用立创 EDA 标准版绘制。

## 结构资料

目录：`SW/`

包含 SolidWorks 零件文件，例如：

- `车轮.SLDPRT`
- `小腿.SLDPRT`
- `适配4310大腿.SLDPRT`
- `重置底板.SLDPRT`
- `飞卡车皮.SLDPRT`
- `调参用相关参数.png`

## 调试和标定注意事项

- 上电前确认电机 ID 和 CAN ID 映射一致，否则主控会把电压发给错误电机。
- FOC 板超过 500 ms 没收到有效 CAN 控制帧会自动把目标电压清零，并点亮离线指示灯。
- ESP32 主控串口波特率为 `115200`，`serial.cpp` 中保留了多组调试输出，可按需要打开。
- MPU6050 的安装方向和偏置会影响姿态控制，`imu.cpp` 中已有一组偏置参数，换硬件后建议重新标定。
- `motor.cpp` 中的关节零位、方向、最大电压和扭矩系数依赖实物测量，换电机或机构后需要重新确认。
- `ctrl.cpp` 当前基础控制周期为 4 ms，CAN 接收和电机发送周期约为 2 ms。
- 开启 BLE 或 ADS1115 电池电压补偿时，需要在 `main.cpp` 中调用对应初始化函数。

## 推荐开发流程

1. 先单独烧录和调试每块 FOC 板，确认 `setid`、`vot`、`angle` 命令工作正常。
2. 用主控 CAN 测试任务或低电压指令确认 CAN 收发和电机 ID 映射。
3. 检查左右关节角度方向、轮电机方向和零位偏置。
4. 确认 MATLAB 生成的腿部运动学输出与实物姿态一致。
5. 再启用平衡控制和腿长/yaw/roll 相关控制。

## 许可

