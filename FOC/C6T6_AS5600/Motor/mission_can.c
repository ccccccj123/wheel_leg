//
// Created by zhangyucong on 2023/11/22.
//

#include "mission_can.h"
#include "can.h"
#include <string.h>
#include "main.h"
#include "mission_uart.h"

/*
 * mission_can.c 负责通过 CAN 接收外部控制命令。
 *
 * 这份 FOC 程序的控制输入很简单：
 *   外部主控通过 CAN 发来目标电压，写入 targetVotage；
 *   main.c 循环中调用 setTargetVotage(targetVotage)；
 *   最终这个值进入 voltage.q，驱动 FOC 输出。
 */

uint8_t motorID = 1;       // 当前电机 ID，用来决定自己读取 CAN 帧中的哪两个字节。
uint32_t lastRecvTime = 0; // 上次收到有效 CAN 控制帧的时间，用于离线保护。

static void CAN_SetFilters(void) {
    // 设置 CAN 接收筛选器。
    CAN_FilterTypeDef filter;

    filter.FilterBank = 0;                        //筛选器组编号
    filter.FilterMode = CAN_FILTERMODE_IDMASK;    //ID掩码模式
    filter.FilterScale = CAN_FILTERSCALE_32BIT;   //32位
    // 不根据 ID 进行筛选：掩码全 0 表示所有 ID 都接收，再在回调里用代码判断。
    filter.FilterIdHigh = 0x0000;               //ID的高十六位
    filter.FilterIdLow = 0x0000;                //ID的低十六位，IDE=0 标准格式帧 RTR=0 数据帧
    filter.FilterMaskIdHigh = 0x0000;           //掩码的高十六位
    filter.FilterMaskIdLow = 0x0000;            //掩码的低十六位，均接受，所有位任意

    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0; //使用FIFO0接收数据
    //filter.SlaveStartFilterBank = 0;          //只有一个CAN模块，该参数无意义
    filter.FilterActivation = CAN_FILTER_ENABLE;  //激活过滤器
  
    HAL_CAN_ConfigFilter(&hcan, &filter);

}

// 发送一个反馈数据帧：把当前角度和速度发回主控。
void CAN_SendState(const float angle, const float speed) {
    CAN_TxHeaderTypeDef header;
    header.StdId = motorID + 0x100;     // 反馈帧 ID。例如 motorID=1 时反馈 ID 为 0x101。
    header.IDE = CAN_ID_STD;            //标准帧
    header.RTR = CAN_RTR_DATA;          //数据帧
    header.DLC = 8;                     //一共8个字节
    header.TransmitGlobalTime = DISABLE;  //不使用时间戳
    uint8_t data[8];

    /*
     * 数据缩放：
     *   angle 单位 rad，乘 1000 后转 int32_t，保留 0.001 rad 分辨率；
     *   speed 单位 rpm，乘 10 后转 int16_t，保留 0.1 rpm 分辨率。
     */
    memcpy(data, &(int32_t) {angle * 1000}, 4); //角度数据放在前四个字节
    memcpy(&data[4], &(int16_t) {speed * 10}, 2); //转速数据放在第5-6字节
    uint32_t mailbox;
    HAL_CAN_AddTxMessage(&hcan, &header, data, &mailbox);
}

    uint8_t rxData[8];
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    CAN_RxHeaderTypeDef header;
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, rxData) != HAL_OK) {
        //出现异常
        return;
    }

    /*
     * 控制协议：
     *   0x100 控制 ID 1~4 的电机；
     *   0x200 控制 ID 5~8 的电机；
     *   每台电机占 2 字节，数据类型 int16_t，单位 mV。
     *
     * 例如 motorID=2 时，读取 0x100 帧中的 byte2~byte3。
     */
    if (header.StdId == 0x100 && motorID <= 4) //ID=1~4接收0x100数据帧
    {
        targetVotage = *(int16_t*)&rxData[(motorID-1)*2] / 1000.0f;
        lastRecvTime = HAL_GetTick();
    } else if (header.StdId == 0x200 && motorID > 4) //ID=5~8接收0x200数据帧
    {
        targetVotage = *(int16_t*)&rxData[(motorID-5)*2] / 1000.0f;
        lastRecvTime = HAL_GetTick();
    }
}

void CAN_Init(void) {
    CAN_SetFilters();              //设置过滤器
    __HAL_CAN_ENABLE_IT(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);//启用FIFO0接收到新消息的中断事件
    HAL_CAN_Start(&hcan);                                  //启动CAN
}


////CAN离线检测，500ms没收到CAN信号则停机
void CAN_offlineCheckProcess(void)
{
    static uint8_t isOffline = 0;
    if(HAL_GetTick() - lastRecvTime > 500)
    {
        if(!isOffline)//离线后只进行一次target归零
        {
            HAL_GPIO_WritePin(ERROR_LED_GPIO_Port, ERROR_LED_Pin, GPIO_PIN_SET);
            targetVotage = 0; // 超过 500ms 没收到控制帧，清零 q 轴目标电压，避免电机失控。
            isOffline = 1;
        }
    }
    else
    {
        isOffline = 0;
        HAL_GPIO_WritePin(ERROR_LED_GPIO_Port, ERROR_LED_Pin, GPIO_PIN_RESET);
    } 
}


