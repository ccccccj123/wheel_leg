//
// Created by zhangyucong on 2023/11/22.
//

#include "mission_uart.h"
#include "usart.h"
#include "mission_can.h"
#include "FlashStorage.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_armcc.h"
#include "can.h"
#include "foc_utils.h"
#include "BLDCMotor.h"
#include "FOCMotor.h"
char rxProBuf[16];

/*
 * mission_uart.c 是串口调试入口。
 *
 * 初学时很有用：
 *   - 可以设置电机 ID；
 *   - 可以擦除 Flash，让电机重新标定；
 *   - 可以直接发送 vot:xxx 设置 q 轴目标电压；
 *   - 可以查看当前电角度。
 *
 * 串口命令最后都会影响 targetVotage 或 Flash 参数，
 * 再由 main.c 的 FOC 主循环使用。
 */

// 单片机软件复位。
void System_Reset(void)
{
    __set_FAULTMASK(1);
    NVIC_SystemReset();
}


void bufCommandCheck(const char buf[])
{

    if(strstr(buf,"setid"))
    {
        int tempid;
        sscanf(buf,"setid:%d\r\n",&tempid); // 命令格式示例：setid:3
        Flash_SaveMotorID(tempid);          // 保存到 Flash，断电后仍然保留。
        System_Reset();                     // 复位后重新读取新的 ID。
    } else if (strstr(buf,"erase"))
    {
        Flash_EraseAllPages();              // 擦除 ID 和电机标定参数。
        System_Reset();
    }
    else if (strstr(buf,"reboot"))
    {
        System_Reset();
    }
    if (strstr(buf,"vot:"))
    {
      int millivolt;
      sscanf(buf,"vot:%d\r\n",&millivolt); // 命令格式示例：vot:1000，表示 1000mV。
      targetVotage=millivolt/1000.0f;      // 内部使用 V，所以 mV / 1000。
      targetVotage = _constrain(targetVotage, -voltage_limit, voltage_limit); // 防止串口输入过大。
      printf("votage is %.3f\r\n",targetVotage);
    }
    if (strstr(buf,"angle"))
    {
      printf("e_angle is %.3f\r\n",electrical_angle);
    }
}


void USART_DMA_StartReceive(void)
{
    /*
     * 使用 ReceiveToIdle DMA：
     *   串口收到一段数据后，如果总线空闲，会触发 HAL_UARTEx_RxEventCallback。
     * 这比固定长度接收更适合简单命令行。
     */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1,(uint8_t*)rxProBuf,16);
    // 关闭半传输中断，只关心“接收完成/空闲”事件，减少无用中断。
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx,DMA_IT_HT);
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance==USART1)
    {
        HAL_UART_DMAStop(huart);  //DMA通道被配置为环模式，每次接收完都需要关闭，防止数据错误
        bufCommandCheck(rxProBuf); // 解析本次收到的命令。
        memset(rxProBuf,0,16);      // 清理数组，避免旧数据影响下一条命令。
        USART_DMA_StartReceive();   // 重新打开下一次 DMA 接收。
    }
}


