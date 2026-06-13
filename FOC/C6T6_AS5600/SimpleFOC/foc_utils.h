//
// Created by zhangyucong on 2023/11/4.
//

#ifndef SIMPLEFOC_FOC_UTILS_H
#define SIMPLEFOC_FOC_UTILS_H


#include <math.h>

/******************************************************************************/
// 四舍五入到 long。查正弦表时用它把浮点下标变成整数下标。
#define _round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

// 限幅宏：把 amt 限制在 [low, high] 之间。电压限幅会频繁用到。
#define _constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

// sqrt 使用本工程自己的快速近似实现。
#define _sqrt(a) (_sqrtApprox(a))

// 常用数学常数，FOC/SVPWM 里会反复使用。
#define _SQRT3 1.73205080757   // sqrt(3)
#define _PI 3.14159265359      // pi
#define _PI_2 1.57079632679    // pi / 2，即 90 度
#define _PI_3 1.0471975512     // pi / 3，即 60 度，SVPWM 一个扇区
#define _2PI 6.28318530718     // 2 * pi，一整圈
#define _3PI_2 4.71238898038   // 3 * pi / 2，即 270 度

// DQ 坐标系电压。
// d 轴：沿转子磁链方向；q 轴：垂直于磁链方向，主要产生转矩。
typedef struct
{
    float d; // d 轴电压，本工程通常为 0。
    float q; // q 轴电压，由 CAN/串口目标电压间接设置。
} DQVoltage_s;
/******************************************************************************/
float _sin(float a);                                      // 查表近似 sin(a)，输入应为 0~2PI。
float _normalizeAngle(float angle);                       // 把角度归一化到 0~2PI。
float _electricalAngle(float shaft_angle, int pole_pairs); // 机械角度乘极对数得到电角度。
float _sqrtApprox(float number);                          // 快速平方根近似。
/******************************************************************************/

#endif //SIMPLEFOC_FOC_UTILS_H
