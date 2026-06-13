//
// Created by zhangyucong on 2023/11/25.
//

#include "mission_loop.h"
#include "stdint.h"
//电机转速计算
#include "foc_utils.h"
#include "stm32f1xx_hal.h"
#include "mission_can.h"
#include "FOCMotor.h"
#include "SimpleKalman.h"
#include "mission_uart.h"
#include "as5600.h"
#include "stdio.h"

/*
 * mission_loop.c 是辅助周期任务。
 *
 * 它不直接产生三相 PWM，真正的 PWM 输出在 loopFOC() 中完成。
 * 这里主要做：
 *   - 心跳灯；
 *   - CAN 离线保护；
 *   - 角度滤波；
 *   - 根据角度差估算转速；
 *   - 通过 CAN 回传状态。
 */

#define speedLpfLen 5
float speed;         // 电机转速，单位 RPM。
float filteredAngle; // 经过卡尔曼滤波后的机械角度，单位 rad。

float Motor_SpeedCalcProcess(float angle_now) {
    // 保存最近 5 次角度，用“当前角度 - 最早角度”估算平均速度。
    static float speedLpfBuf[speedLpfLen] = {0};

    /*
     * 速度估算公式：
     *   角速度 rad/s = 角度差 / 时间差
     *   RPM = rad/s / 2PI * 60
     *
     * 这里默认 mission_loop() 大约 1ms 调用一次，
     * 所以 5 个样本间隔约为 5ms，公式中使用 1000/speedLpfLen。
     */
    float curSpeed = (angle_now - speedLpfBuf[0]) * 1000 / speedLpfLen / _2PI * 60;//1000是1000ms == 1s

    // 左移数组，丢掉最旧角度，把当前角度放到末尾。
    for (uint8_t i = 0; i < speedLpfLen - 1; i++)
        speedLpfBuf[i] = speedLpfBuf[i + 1];
    speedLpfBuf[speedLpfLen - 1] = angle_now;

    return  curSpeed;
}

void LED_Blink(void) {
    // 每隔约 200ms 翻转一次 LED，作为程序仍在运行的心跳指示。
    if (HAL_GetTick() % 200 == 0) {
      HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
    }
}

void mission_loop(void) {
    LED_Blink();                 // 心跳灯。
    CAN_offlineCheckProcess();   // 500ms 没收到 CAN 控制帧则清零目标电压。

    // 根据滤波后的角度估算转速。
    speed=Motor_SpeedCalcProcess(filteredAngle);

    /*
     * 发送状态反馈。
     * 注意：HAL_GetTick()%2 为真时，在同一个奇数毫秒内可能执行多次；
     * 如果需要严格 2ms 一次，建议改成“记录上次发送时间”的写法。
     */
    if (HAL_GetTick() % 2)
        CAN_SendState(shaft_angle, speed);

    // 用当前机械角度更新滤波器，供下一轮速度估算使用。
    filteredAngle = Kalman_Filter(&angleFilter, shaft_angle);

}
