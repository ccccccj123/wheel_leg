//
// Created by zhangyucong on 2023/11/23.
//
#include "tim.h"
#include "Beep.h"
#include "BLDCMotor.h"
#include "foc_utils.h"

/*
 * Beep.c 利用电机本体发声。
 *
 * 原理很朴素：
 *   定时器 TIM1 周期性触发中断；
 *   中断里让电机磁场在两个角度之间来回跳；
 *   电机绕组和结构会产生可听振动，于是听起来像提示音。
 *
 * 播放提示音时，loopFOC() 会暂停正常电压输出，避免两边同时写 PWM。
 */

uint8_t beepPlaying = 0;   // 当前是否在蜂鸣状态。1 表示蜂鸣中，0 表示正常 FOC 输出。


// 根据蜂鸣周期配置并触发定时器。
void Beep_Play(uint16_t period)
{
    if(period!=0)
    {
        __HAL_TIM_SetAutoreload(&htim1,period/2); // 设置 TIM1 自动重装值，决定中断频率。
        HAL_TIM_Base_Start_IT(&htim1);            // 开启 TIM1 更新中断。
        beepPlaying = 1;                          // 通知 loopFOC() 暂停正常输出。
    }
    else
    {
        HAL_TIM_Base_Stop_IT(&htim1);             // period 为 0 表示停止播放。
        beepPlaying = 0;                          // 恢复正常 FOC 输出。
    }
}

// 播放一串音符，notes[i][0] 是音调周期，notes[i][1] 是持续时间 ms。
void Beep_PlayNotes(uint8_t num, uint16_t notes[][2])
{
    for(uint8_t i=0; i<num; i++)
    {
        Beep_Play(notes[i][0]);
        HAL_Delay(notes[i][1]);
    }
    Beep_Play(0);
}

// 蜂鸣中断处理，在定时器中断回调中调用。
void Beep_IRQHandler(void)
{
    static uint8_t flipFlag = 0;
    if(targetVotage == 0)
    {
        /*
         * 只在目标电压为 0 时发声，避免电机正在受控运动时强行改变相电压。
         * 这里让磁场角度在 0 和 PI/3 间来回切换，形成振动。
         */
        setPhaseVoltage(voltage_limit/2, 0, _PI/3 * flipFlag); //使磁场方向在0-PI/3间震荡
        flipFlag = !flipFlag;
    }
}

void Motor_PlaySong(void)
{
  Beep_PlayNotes(3,(uint16_t[][2]){{T_H1,200},{T_H3,200},{T_H5,500}}); //播放开机音效
}
