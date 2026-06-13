//
// Created by zhangyucong on 2023/11/22.
//

#include "FlashStorage.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_flash_ex.h"

/*
 * FlashStorage.c 负责保存 FOC 标定参数。
 *
 * 为什么要保存？
 *   第一次上电时 alignSensor() 会标定编码器方向和零电角度；
 *   如果每次上电都标定，电机会抖动/旋转，使用体验不好；
 *   所以标定成功后写入 Flash，下次开机直接读取。
 */

// STM32F103C6T6 Flash 大小为 32K，RAM 为 10K。
#define FLASH_STORAGE_PARAM_ADDR 0x08007800 // 第 30 页：电机结构参数，极对数/零电角度/方向。
#define FLASH_STORAGE_ID_ADDR 0x08007C00    // 第 31 页：电机 ID。
#define FLASH_STORAGE_AVAIL_FLAG 0xA5A5A5A5 // Flash 数据有效标志，用来判断这一页是否保存过参数。

static void Flash_ErasePage(uint32_t PageAddress)
{
    HAL_FLASH_Unlock();                           // 写 Flash 前必须先解锁。
    FLASH_EraseInitTypeDef erase;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;      // 按页擦除。
    erase.PageAddress = PageAddress;              // 要擦除的页地址。
    erase.NbPages = 1;                            // 只擦除一页。
    uint32_t error = 0;
    HAL_StatusTypeDef result;
    result=HAL_FLASHEx_Erase(&erase, &error);
    if((result!=HAL_OK )||(error!=0xFFFFFFFF))
    {
       
    }
    HAL_FLASH_Lock();

}

void Flash_EraseAllPages(void)
{
    HAL_FLASH_Unlock();                           // 解锁 Flash。
    FLASH_EraseInitTypeDef erase;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;      // 按页擦除。
    erase.PageAddress = FLASH_STORAGE_PARAM_ADDR; // 从第 30 页开始。
    erase.NbPages = 2;                            // 擦除第 30、31 两页。
    uint32_t error = 0;
    HAL_StatusTypeDef result;
    result=HAL_FLASHEx_Erase(&erase, &error);
    if((result!=HAL_OK )||(error!=0xFFFFFFFF))
    {
       
    }
    HAL_FLASH_Lock();
}

// 将电机 ID 存入 Flash。
void Flash_SaveMotorID(uint8_t id)
{
    // Flash 只能从 1 写成 0，不能直接从 0 写回 1，所以写入前必须擦页。
    Flash_ErasePage(FLASH_STORAGE_ID_ADDR);
    HAL_FLASH_Unlock();

    // 先写有效标志，再写 ID。
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_ID_ADDR, FLASH_STORAGE_AVAIL_FLAG);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_ID_ADDR+4, id);

    HAL_FLASH_Lock();
}


// 将电机结构参数存入 Flash。
void Flash_SaveMotorParam(int poles, float zero_elec_angle, int dir)
{
    Flash_ErasePage(FLASH_STORAGE_PARAM_ADDR);
    HAL_FLASH_Unlock();
    uint32_t buf[4];
    buf[0] = FLASH_STORAGE_AVAIL_FLAG;       // 有效标志。
    buf[1] = *((uint32_t*)&poles);           // 极对数按原始 32 位写入。
    buf[2] = *((uint32_t*)&zero_elec_angle); // float 零电角度按原始 32 位写入。
    buf[3] = *((uint32_t*)&dir);             // 编码器方向。

    // STM32F1 按 word，也就是 32 bit 写入。
    for(uint8_t i=0; i<4; i++)
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_PARAM_ADDR+i*4, buf[i]);

    HAL_FLASH_Lock();
}


// 从 Flash 中读取电机 ID。
uint8_t Flash_ReadMotorID(void)
{
    uint32_t *buf = (uint32_t*)FLASH_STORAGE_ID_ADDR;
    // 没有有效标志，说明还没保存过 ID，返回 0，让上层使用默认 ID。
    if(buf[0] != FLASH_STORAGE_AVAIL_FLAG)
        return 0;
    return buf[1];
}

// 从 Flash 中读取电机结构参数。
HAL_StatusTypeDef Flash_ReadMotorParam(int* poles, float* zero_elec_angle, int* dir)
{

    uint32_t *buf = (uint32_t*)FLASH_STORAGE_PARAM_ADDR;
    // 没有有效标志，说明还没有标定数据，需要重新 alignSensor()。
    if(buf[0] != FLASH_STORAGE_AVAIL_FLAG)
        return HAL_ERROR;

    // 按保存时的原始位模式转换回来。
    *poles = *((int*)&buf[1]);
    *zero_elec_angle = *((float*)&buf[2]);
    *dir = *((int*)&buf[3]);
     
    return HAL_OK;
}


