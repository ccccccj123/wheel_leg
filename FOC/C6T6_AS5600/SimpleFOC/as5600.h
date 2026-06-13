//
// Created by zhangyucong on 2023/11/3.
//

#ifndef MYMINIFOC_CLION_AS5600_H
#define MYMINIFOC_CLION_AS5600_H


#include "i2c.h"


// 本工程使用 I2C1 连接 AS5600，hi2c1 在 Core/Src/i2c.c 中定义。
#define AS5600_I2C_HANDLE (hi2c1)


/*
注意:AS5600的地址0x36是指的是原始7位设备地址,而ST I2C库中的设备地址是指原始设备地址左移一位得到的设备地址
*/
#define I2C_TIME_OUT_BASE   10
#define I2C_TIME_OUT_BYTE   1

/*
注意:AS5600的地址0x36是指的是原始7位设备地址,而ST I2C库中的设备地址是指原始设备地址左移一位得到的设备地址
*/

#define AS5600_RAW_ADDR    0x36              // AS5600 数据手册中的 7 bit 地址。
#define AS5600_ADDR        (AS5600_RAW_ADDR << 1) // STM32 HAL 使用左移后的地址。


#define AS5600_RESOLUTION 4096 // 12 bit 分辨率，一圈被分成 4096 份。

#define AS5600_RAW_ANGLE_REGISTER  0x0C // 原始角度寄存器地址。

extern float angle_prev; // 上一次角度值，供速度计算或初始化时使用。

void as5600Init(void); // 初始化 AS5600 角度读取相关的软件状态。

uint16_t as5600GetRawAngle(void); // 读取 0~4095 的原始角度计数。

float as5600getAngle(void); // 返回连续机械角度，单位 rad，可跨多圈。

#endif //MYMINIFOC_CLION_AS5600_H
