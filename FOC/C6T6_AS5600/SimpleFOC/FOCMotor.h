//
// Created by zhangyucong on 2023/11/4.
//

#ifndef SIMPLEFOC_FOCMOTOR_H
#define SIMPLEFOC_FOCMOTOR_H

#include "foc_utils.h"

extern float shaft_angle;      // 当前机械角度，来自 AS5600，单位 rad。
extern float electrical_angle; // 当前电角度，FOC/SVPWM 输出需要用它判断转子磁场位置。

extern DQVoltage_s voltage;    // DQ 坐标系电压。voltage.q 是转矩方向电压，voltage.d 通常为 0。


extern float zero_electric_angle; // 电角度与机械角度的零点偏差，由 alignSensor() 得到。

float shaftAngle(void);       // 读取并修正机械角度。
float electricalAngle(void);  // 把机械角度换算成归一化电角度。

#endif //SIMPLEFOC_FOCMOTOR_H
