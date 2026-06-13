//
// Created by zhangyucong on 2023/11/3.
//

#include "as5600.h"

#define abs(x) ((x)>0?(x):-(x))
#define _2PI 6.28318530718
//参考资料：https://blog.csdn.net/xiaoyuanwuhui/article/details/118970127
/*
 * as5600.c 负责读取磁编码器 AS5600。
 *
 * AS5600 一圈输出 0~4095，也就是 12 bit 分辨率。
 * 但电机可能连续转很多圈，所以这里额外用 full_rotation_offset
 * 记录“已经跨过了多少整圈”，最终返回一个可连续增减的弧度角。
 */

// static 修饰的全局变量只在本文件可见，外部文件不能直接访问。
static float angle_data_prev = 0;    // 上一次读取到的 AS5600 原始值，范围 0~4095。
static float full_rotation_offset;   // 跨圈累计偏移，单位 rad。
float angle_prev;                    // 上一次连续角度，供其它模块需要时使用。

static int i2cWrite(uint8_t dev_addr, uint8_t *pData, uint32_t count) {
    int status;
    // 根据读写字节数给一个简单超时时间，避免 I2C 异常时一直卡死。
    int i2c_time_out = I2C_TIME_OUT_BASE + count * I2C_TIME_OUT_BYTE;

    // STM32 HAL 的 I2C 发送：向设备地址 dev_addr 写 count 个字节。
    status = HAL_I2C_Master_Transmit(&AS5600_I2C_HANDLE, dev_addr, pData, count, i2c_time_out);
    return status;
}

static int i2cRead(uint8_t dev_addr, uint8_t *pData, uint32_t count) {
    int status;
    int i2c_time_out = I2C_TIME_OUT_BASE + count * I2C_TIME_OUT_BYTE;

    /*
     * 读操作使用 dev_addr | 1。
     * 注意 HAL 通常要求传入左移一位后的 8 bit 地址，本工程 AS5600_ADDR 已经左移。
     */
    status = HAL_I2C_Master_Receive(&AS5600_I2C_HANDLE, (dev_addr | 1), pData, count, i2c_time_out);
    return status;
}

uint16_t as5600GetRawAngle(void) {
    uint16_t raw_angle;
    uint8_t buffer[2] = {0};
    uint8_t raw_angle_register = AS5600_RAW_ANGLE_REGISTER;

    // 先告诉 AS5600：接下来要读取 RAW_ANGLE 寄存器。
    i2cWrite(AS5600_ADDR, &raw_angle_register, 1);

    // RAW_ANGLE 是两个字节：高 8 位在 buffer[0]，低 8 位在 buffer[1]。
    i2cRead(AS5600_ADDR, buffer, 2);

    // 合并两个字节。AS5600 实际有效位为 12 bit，范围 0~4095。
    raw_angle = ((uint16_t) buffer[0] << 8) | (uint16_t) buffer[1];
    return raw_angle;
}

// 返回连续机械角度，单位 rad。
float as5600getAngle(void) {
    // 当前原始角度，仍然是 0~4095 的计数值。
    float angle_data = as5600GetRawAngle();

    /*
     * 计算这次和上次读取值的差。
     * 正常小幅运动时差值不会太大；
     * 如果从 4095 跳到 0，或者从 0 跳到 4095，差值会接近一整圈。
     */
    float d_angle = angle_data - angle_data_prev;
    if (abs(d_angle) > (0.8 * AS5600_RESOLUTION)) {
        /*
         * 发生跨圈：
         *   d_angle > 0 通常表示从小值跳到大值，比如 10 -> 4090，
         *   实际是反向跨过 0 点，所以累计角度减一圈。
         *
         *   d_angle < 0 通常表示从大值跳到小值，比如 4090 -> 10，
         *   实际是正向跨过 0 点，所以累计角度加一圈。
         */
        full_rotation_offset += (d_angle > 0 ? -_2PI : _2PI);
    }
    angle_data_prev = angle_data;

    if (full_rotation_offset >= (_2PI * 1000)) //转动圈数过多后浮点数精度下降，并导致堵转，每隔一千圈归零一次
    {                                        //这个问题针对电机长时间连续转动；如果不是长时间一个方向转动也可以屏蔽掉这几句
        full_rotation_offset = 0;
        angle_prev = angle_prev - _2PI * 1000;
    }
    if (full_rotation_offset <= (-_2PI * 1000)) {
        full_rotation_offset = 0;
        angle_prev = angle_prev + _2PI * 1000;
    }

    // 原始值 / 4096 得到一圈内比例，再乘 2PI 变成弧度，最后加上跨圈累计值。
    return (full_rotation_offset + (angle_data / (float) AS5600_RESOLUTION) * _2PI);
}

void as5600Init(void) {
    /*
     * 这里不初始化 I2C 外设本身，I2C 已经在 MX_I2C1_Init() 中完成。
     * 本函数只初始化 AS5600 读角度所需的软件状态。
     */

    full_rotation_offset = 0;            // 上电时从 0 圈累计开始。
    angle_data_prev = as5600GetRawAngle(); // 先记录当前原始角度，作为跨圈判断的基准。
    HAL_Delay(5);
    angle_prev = as5600getAngle();       // 再读一次连续角度，给其它模块初值。
}

