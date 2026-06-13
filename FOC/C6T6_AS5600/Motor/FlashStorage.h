//
// Created by zhangyucong on 2023/11/22.
//

#ifndef JOINTMOTOR_FLASHSTORAGE_H
#define JOINTMOTOR_FLASHSTORAGE_H
#include "stm32f1xx_hal.h"
#include <stdint.h>

void Flash_SaveMotorParam(int poles, float zero_elec_angle, int dir);
void Flash_SaveMotorID(uint8_t id);
uint8_t Flash_ReadMotorID(void);
HAL_StatusTypeDef Flash_ReadMotorParam(int* poles, float* zero_elec_angle, int* dir);
void Flash_EraseAllPages(void);
#endif //JOINTMOTOR_FLASHSTORAGE_H
