//
// Created by zhangyucong on 2023/11/4.
//

#ifndef SIMPLEFOC_BLDCMOTOR_H
#define SIMPLEFOC_BLDCMOTOR_H

#include "main.h"

// 驱动芯片使能脚。置高允许三相桥输出，置低关闭输出。
#define MOTOR_ENABLE HAL_GPIO_WritePin(DRV_EN_GPIO_Port,DRV_EN_Pin,GPIO_PIN_SET)
#define MOTOR_DISENABLE HAL_GPIO_WritePin(DRV_EN_GPIO_Port,DRV_EN_Pin,GPIO_PIN_RESET)

// TIM2 的 PWM 周期。必须与 Core/Src/tim.c 中 htim2.Init.Period + 1 保持一致。
#define PWM_Period 1280

// 编码器方向枚举。标定时会根据电机实际转动方向确定使用 CW 还是 CCW。
typedef enum
{
    CW      = 1,  //clockwise
    CCW     = -1, // counter clockwise
    UNKNOWN = 0   //not yet known or invalid state
} Direction;

extern int sensor_direction;           // 编码器方向修正，取值见 Direction。
extern float voltage_power_supply;     // 母线供电电压。
extern float voltage_limit;            // q/d 轴电压限幅。
extern float voltage_sensor_align;     // 标定时使用的开环电压。
extern int  pole_pairs;                // 电机极对数。

void Motor_Init(void);                                  // 启动 PWM、设置电压限制、使能驱动。
void FOC_Init(void);                                    // 读取或标定 FOC 所需参数。
uint8_t alignSensor(void);                              // 标定编码器方向和零电角度。
void setPhaseVoltage(float Uq, float Ud, float angle_el); // 把 d/q 轴电压转换成三相 PWM。
void loopFOC(void);                                     // FOC 主循环：读角度并输出相电压。
void setTargetVotage(float new_target);                 // 设置目标 q 轴电压。源码沿用 Votage 拼写。
float velocityOpenloop(float target_velocity);          // 开环速度接口声明，当前源码未实现。

#endif //SIMPLEFOC_BLDCMOTOR_H
