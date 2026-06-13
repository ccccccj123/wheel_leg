//
// Created by zhangyucong on 2023/11/23.
//

#ifndef JOINTMOTOR_BEEP_H
#define JOINTMOTOR_BEEP_H
enum
{
    //不响
    T_None=0,
    //高八度
    T_H1=956,
    T_H2=851,
    T_H3=758,
    T_H4=716,
    T_H5=638,
    T_H6=568,
    T_H7=506
};

extern uint8_t beepPlaying;

void Beep_IRQHandler(void);
void Motor_PlaySong(void);
#endif //JOINTMOTOR_BEEP_H
