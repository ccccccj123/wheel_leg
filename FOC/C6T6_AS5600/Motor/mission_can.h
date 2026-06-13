//
// Created by zhangyucong on 2023/11/22.
//

#ifndef JOINTMOTOR_MISSION_CAN_H
#define JOINTMOTOR_MISSION_CAN_H

#include "main.h"


extern uint8_t motorID ;       //初始电机ID
extern uint32_t lastRecvTime ; //上次收到CAN数据帧的时间

void CAN_Init(void);
void CAN_offlineCheckProcess(void);
void CAN_SendState(const float angle, const float speed);
#endif //JOINTMOTOR_MISSION_CAN_H
