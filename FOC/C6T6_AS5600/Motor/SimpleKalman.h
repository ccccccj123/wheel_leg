//
// Created by zhangyucong on 2023/11/25.
//

#ifndef JOINTMOTOR_SIMPLEKALMAN_H
#define JOINTMOTOR_SIMPLEKALMAN_H

typedef struct{
    float X_last;
    float X_mid;
    float X_now;
    float P_mid;
    float P_now;
    float P_last;
    float kg;
    float A;
    float Q;
    float R;
    float H;
} Kalman;

extern Kalman angleFilter; //卡尔曼滤波结构体

void Kalman_Init(Kalman* p,float T_Q,float T_R);
float Kalman_Filter(Kalman* p,float dat);

#endif //JOINTMOTOR_SIMPLEKALMAN_H
