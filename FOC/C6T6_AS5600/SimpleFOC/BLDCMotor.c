//
// Created by zhangyucong on 2023/11/4.
//

#include "BLDCMotor.h"
#include "main.h"
#include "foc_utils.h"
#include "FOCMotor.h"
#include "as5600.h"
#include "tim.h"
#include "Beep.h"
#include "mission_uart.h"
#include "FlashStorage.h"
#include "stdio.h"

/*
 * BLDCMotor.c 是这份工程里最核心的 FOC 输出文件。
 *
 * 初学者可以先抓住一句话：
 *   外部给一个 q 轴目标电压 voltage.q，
 *   程序读取 AS5600 得到转子角度，
 *   再用 setPhaseVoltage() 把这个电压换算成三相 PWM 占空比。
 *
 * 这份代码是“电压模式 FOC”：
 *   - 没有采样相电流；
 *   - 没有电流环 PI；
 *   - Uq 直接表示希望施加到 q 轴上的电压。
 */

int pole_pairs = 14;               // 电机极对数。2805 电机一般为 7，4310 电机一般为 14。

//下面按照极对数设置 下面的数不需要调整！！！！
float voltage_power_supply = 16.8f;         // V，母线供电电压 Udc。4S 电池满电约 16.8V。
float voltage_limit = 0;                    // V，运行时 Uq/Ud 的最大允许值，防止输出过大。
float voltage_sensor_align = 0;             // V，标定时使用的开环电压：太小电机不动，太大容易发热。
int sensor_direction = UNKNOWN;             // 编码器方向。标定后变成 CW(1) 或 CCW(-1)。

void Motor_Init(void) {

    /*
     * 根据电机极对数选择一组保守电压参数。
     * pole_pairs == 7 对应较小电机，所以限幅低一些。
     * 其它情况默认按 4310 类电机处理，限幅高一些。
     */
    if (pole_pairs == 7)
    {
      voltage_limit = 4.5f;
      voltage_sensor_align = 2.0f;
    }
    else 
    {
      voltage_limit = 9.69f;
      voltage_sensor_align = 6.0f;
    }
    
    if (voltage_sensor_align > voltage_limit) // 标定电压不能超过运行电压上限。
        voltage_sensor_align = voltage_limit;

    /*
     * TIM2 的 CH1/CH2/CH3 分别输出三路 PWM。
     * 这些 PWM 进入三相驱动桥，最终控制电机 A/B/C 三相。
     */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

    MOTOR_ENABLE; // 使能驱动芯片，让功率级真正允许输出。

}

void FOC_Init(void) {

    /*
     * FOC 必须知道两个关键参数：
     *   1. sensor_direction：编码器角度方向；
     *   2. zero_electric_angle：机械角度与电角度之间的零点偏差。
     *
     * 如果 Flash 里已经保存过，就直接读取；
     * 如果没有保存过，就执行 alignSensor() 现场标定。
     */
    if(Flash_ReadMotorParam(&pole_pairs, &zero_electric_angle,  &sensor_direction) != HAL_OK)
    {
        if (alignSensor())    // 标定传感器方向和电角度零点。
        {
          // 标定成功后保存到 Flash，下次上电可跳过标定。
          Flash_SaveMotorParam(pole_pairs,zero_electric_angle,sensor_direction);
          printf("Save:\r\n");
          printf("pairs:%d\t,zero_angle:%.4f\t,dir:%d\r\n",pole_pairs,zero_electric_angle,sensor_direction);
        }
    }
    else
    {
      printf("Read:\r\n");
      printf("pairs:%d\t,zero_angle:%.4f\t,dir:%d\r\n",pole_pairs,zero_electric_angle,sensor_direction);
    }

    // 上电后先读一次角度，给后续角度/速度计算一个稳定初值。
    angle_prev = as5600getAngle();
    HAL_Delay(5);
    shaft_angle = shaftAngle(); // 更新当前机械角度。

    HAL_Delay(100);
}

uint8_t alignSensor(void) {
    long i;
    float angle;
    float mid_angle, end_angle;
    float moved;

    printf("Align sensor.\r\n");

    /*
     * 第 1 步：让电机磁场按一个方向转过一圈电角度。
     *
     * setPhaseVoltage(voltage_sensor_align, 0, angle)
     * 表示给定一个固定大小的 q 轴电压，并慢慢改变电角度 angle。
     * 转子会被这个旋转磁场拖着走。
     */
    for (i = 0; i <= 500; i++) {
        angle = _3PI_2 + _2PI * i / 500.0;
        setPhaseVoltage(voltage_sensor_align, 0, angle);
        HAL_Delay(2);
    }
    mid_angle = as5600getAngle(); // 记录正向拖动后的机械角度。

    /*
     * 第 2 步：再把磁场反方向转回来。
     * 通过比较 mid_angle 和 end_angle，可以判断编码器方向。
     */
    for (i = 500; i >= 0; i--)
    {
        angle = _3PI_2 + _2PI * i / 500.0;
        setPhaseVoltage(voltage_sensor_align, 0, angle);
        HAL_Delay(2);
    }
    end_angle = as5600getAngle(); // 记录反向拖动后的机械角度。
    setPhaseVoltage(0, 0, 0);     // 标定阶段临时关闭输出。
    HAL_Delay(200);

    printf("mid_angle=%.4f\r\n", mid_angle);
    printf("end_angle=%.4f\r\n", end_angle);

    moved = fabs(mid_angle - end_angle);
    if ((mid_angle == end_angle) || (moved < 0.02))  // 角度几乎没变，说明电机/编码器/驱动可能异常。
    {
        printf("Failed");
        MOTOR_DISENABLE;    // 电机检测不正常，关闭驱动，避免继续输出。
        return 0;
    } else if (mid_angle < end_angle) {
        // 反向拖动后角度变大，说明传感器方向与设定方向相反。
        sensor_direction = CCW;
    } else {
        // 反向拖动后角度变小，说明传感器方向与设定方向一致。
        sensor_direction = CW;
    }

    /*
     * 第 3 步：把电角度固定在 270 度，让转子停在一个已知磁场方向。
     * 此时读取到的机械角度乘以极对数，就能得到电角度零点偏移。
     */
    setPhaseVoltage(voltage_sensor_align, 0, _3PI_2);
    HAL_Delay(700);
    zero_electric_angle = _normalizeAngle(_electricalAngle(sensor_direction * as5600getAngle(), pole_pairs));
    HAL_Delay(20);
    printf("zero_angle:");
    printf("%.4f\r\n", zero_electric_angle);

    setPhaseVoltage(0, 0, 0);
    HAL_Delay(100);

    return 1;
}

/******************************************************************************/
/*
 * setPhaseVoltage()：FOC/SVPWM 的核心函数。
 *
 * 输入：
 *   Uq       q 轴电压。q 轴近似对应“产生转矩”的方向。
 *   Ud       d 轴电压。d 轴近似对应“转子磁链”的方向，本工程通常传 0。
 *   angle_el 当前电角度，单位 rad。
 *
 * 输出：
 *   通过 TIM2 的三个 CCR 比较值，生成三相 PWM 占空比。
 *
 * 初学者理解路线：
 *   1. 先把 Uq/Ud 合成一个空间电压矢量；
 *   2. 判断这个矢量落在 SVPWM 的哪一个 60 度扇区；
 *   3. 算出相邻两个有效矢量 T1/T2 和零矢量 T0 的时间；
 *   4. 映射成三相占空比 Ta/Tb/Tc；
 *   5. 写进定时器比较寄存器，驱动三相桥。
 */
void setPhaseVoltage(float Uq, float Ud, float angle_el) {
    float Uout;             // 归一化后的电压幅值，单位是“占母线电压的比例”。
    uint32_t sector;        // 当前电压矢量所在的 SVPWM 扇区，范围理论上是 1~6。
    float T0, T1, T2;       // T1/T2 是相邻有效矢量作用时间，T0 是零矢量作用时间。
    float Ta, Tb, Tc;       // 三相最终占空比，范围通常在 0~1。

    // 先对输入电压限幅，防止外部命令给出过大的电压。
    Uq = _constrain(Uq, -voltage_limit, voltage_limit);
    Ud = _constrain(Ud, -voltage_limit, voltage_limit);

    if (Ud) // 如果 d 轴和 q 轴都给了电压，需要做一次矢量合成。
    {
        // _sqrt 是近似平方根，速度快但有少量误差。
        Uout = _sqrt(Ud * Ud + Uq * Uq) / voltage_power_supply;
        // atan2(Uq, Ud) 得到合成电压矢量相对 d 轴的角度。
        angle_el = _normalizeAngle(angle_el + atan2(Uq, Ud));
    } else {
        // 本工程常见情况：Ud=0，只给 Uq，因此不需要平方根和 atan2。
        Uout = Uq / voltage_power_supply;
        // q 轴相对 d 轴相差 90 度，所以电角度加 PI/2。
        angle_el = _normalizeAngle(angle_el + _PI_2);
    }

    /*
     * SVPWM 在线性调制区的最大比例约为 1/sqrt(3)=0.577。
     * 超过这个值会进入过调制，波形失真，所以这里强行限制。
     */
    if (Uout > 0.577) Uout = 0.577;
    if (Uout < -0.577) Uout = -0.577;

    sector = (angle_el / _PI_3) + 1; // 每 PI/3，也就是 60 度，划分为一个扇区。

    // 计算当前扇区内两个相邻有效矢量的作用时间比例。
    T1 = _SQRT3 * _sin(sector * _PI_3 - angle_el) * Uout;
    T2 = _SQRT3 * _sin(angle_el - (sector - 1.0) * _PI_3) * Uout;
    T0 = 1 - T1 - T2; // 一个 PWM 周期内剩余时间给零矢量。

    /*
     * 根据扇区把 T1/T2/T0 排列到 A/B/C 三相。
     * T0/2 放在两边，是典型的对称 SVPWM 写法，中心对齐 PWM 下谐波更好。
     */
    switch (sector) {
        case 1:
            Ta = T1 + T2 + T0 / 2;
            Tb = T2 + T0 / 2;
            Tc = T0 / 2;
            break;
        case 2:
            Ta = T1 + T0 / 2;
            Tb = T1 + T2 + T0 / 2;
            Tc = T0 / 2;
            break;
        case 3:
            Ta = T0 / 2;
            Tb = T1 + T2 + T0 / 2;
            Tc = T2 + T0 / 2;
            break;
        case 4:
            Ta = T0 / 2;
            Tb = T1 + T0 / 2;
            Tc = T1 + T2 + T0 / 2;
            break;
        case 5:
            Ta = T2 + T0 / 2;
            Tb = T0 / 2;
            Tc = T1 + T2 + T0 / 2;
            break;
        case 6:
            Ta = T1 + T2 + T0 / 2;
            Tb = T0 / 2;
            Tc = T1 + T0 / 2;
            break;
        default:  // possible error state
            // 理论上 angle_el 已归一化，不应进入这里；异常时关闭三相输出。
            Ta = 0;
            Tb = 0;
            Tc = 0;
    }

    /*
     * 把 0~1 的占空比乘以 PWM 周期，写入定时器比较寄存器。
     * 注意：这里硬件接线/驱动顺序对应 CH1=Tc、CH2=Tb、CH3=Ta。
     * 如果你以后换了三相接线顺序，这里可能需要一起调整。
     */
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, Tc * PWM_Period);
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, Tb * PWM_Period);
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_3, Ta * PWM_Period);
}

/******************************************************************************/
void loopFOC(void) {
    // 先读当前机械角度。shaftAngle() 内部会读取 AS5600，并乘上传感器方向。
    shaft_angle = shaftAngle();

    // 再由机械角度计算电角度。注意必须先更新 shaft_angle。
    electrical_angle = electricalAngle();

    // 没在蜂鸣状态时才正常输出 FOC 电压；蜂鸣时 Beep.c 会临时接管相电压。
    if(beepPlaying == 0)
    {
        setPhaseVoltage(voltage.q, voltage.d, electrical_angle);
    }
}

void setTargetVotage(float new_target) {
    // 外部控制量最终写到 q 轴电压。源码拼写是 Votage，实际含义是 Voltage。
    voltage.q = new_target;
}

