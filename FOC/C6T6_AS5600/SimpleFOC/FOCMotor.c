//
// Created by zhangyucong on 2023/11/4.
//

#include "FOCMotor.h"
#include "foc_utils.h"
#include "as5600.h"
#include "BLDCMotor.h"

/*
 * FOCMotor.c 负责“角度坐标转换”。
 *
 * AS5600 读到的是机械角度：转子真实转过的角度。
 * FOC 需要的是电角度：电机磁场周期中的角度。
 *
 * 二者关系：
 *   电角度 = 机械角度 * 极对数 - 零电角度偏移
 *
 * 例如极对数为 14 时，转子机械转 1 圈，电角度会走 14 圈。
 */

float shaft_angle;      // 当前机械角度，单位 rad。这个值可以跨越多圈，不一定限制在 0~2PI。
float electrical_angle; // 当前电角度，单位 rad。通常会归一化到 0~2PI。

DQVoltage_s voltage;    // DQ 坐标系电压：d 轴一般为 0，q 轴用于产生转矩。

float zero_electric_angle; // 电角度与机械角度之间的零点偏差，由 alignSensor() 标定得到。

/******************************************************************************/
// 计算机械角度。
float shaftAngle(void)
{
    /*
     * as5600getAngle() 返回编码器累计机械角度。
     * sensor_direction 是标定出来的方向修正：
     *   CW  =  1，角度保持原方向；
     *   CCW = -1，角度取反。
     */
    return sensor_direction*as5600getAngle();
}

/******************************************************************************/
// 计算电角度。
float electricalAngle(void)
{
    /*
     * FOC 输出三相电压时必须知道转子磁场的电角度。
     * _normalizeAngle() 把结果限制到 0~2PI，方便后续 SVPWM 判断扇区。
     */
    return _normalizeAngle((shaft_angle) * pole_pairs - zero_electric_angle);
}
/******************************************************************************/


