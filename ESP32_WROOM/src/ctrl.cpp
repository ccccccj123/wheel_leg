#include <Arduino.h>
#include "pid.h"
#include "legs.h"
#include "motor.h"
#include "../include/matlab_code/leg_vmc_conv.h"
#include "../include/matlab_code/lqr_k.h"
#include "ctrl.h"
#include "imu.h"
#include <esp_task_wdt.h>
#include <math.h>

CascadePID legAnglePID, legLengthPID; //腿部角度和长度控制PID
CascadePID yawPID, rollPID; //机身yaw和roll控制PID

Target target = {0, 0, 0, 0, 0, 0, 0.07f};
StateVar stateVar;
StandupState standupState = StandupState_Standup;
GroundDetector groundDetector = {10, 10, true, false};	//离地检测器 默认状态为触地
//测试用
#define SIN_FREQUENCY_MS 5000 // 周期，单位毫秒
#define SIN_AMPLITUDE 0.04      // 幅度
#define SIN_OFFSET 0.1         // 偏移量

void vSinGeneratorTask(void *arg) {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(SIN_FREQUENCY_MS);

    // 获取当前系统时间
    xLastWakeTime = xTaskGetTickCount();

    while (1) {
        // 计算当前时刻的正弦函数值
        float sin_value = SIN_AMPLITUDE * sin((2 * M_PI * xTaskGetTickCount() / xFrequency))+SIN_OFFSET;
		target.legLength = sin_value; // 假设虚拟腿0的腿部长度为正弦函数值
        // 在这里可以将 sin_value 用于其他操作，比如输出到外设

        // 任务挂起，直到下一个周期
        vTaskDelayUntil(&xLastWakeTime, 5);
    }
}

#define STEP_INTERVAL_MS 1000 // 跃变间隔，单位毫秒
#define STEP_AMPLITUDE 5      // 幅度
// 阶跃函数生成任务
void vStepGeneratorTask(void *pvParameters) {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(STEP_INTERVAL_MS);
    int step_value = 0;

    // 获取当前系统时间
    xLastWakeTime = xTaskGetTickCount();

    while (1) {
        // 每隔一定时间改变阶跃函数值
        step_value += STEP_AMPLITUDE;

        // 在这里可以将 step_value 用于其他操作，比如输出到外设

        // 任务挂起，直到下一个跃变间隔
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void VMC_TestTask(void *arg)
{
	float targetLegLength = 0.1f; //m，目标虚拟腿腿部长度 
    float targetLegAngle = 1.57f; //rad，目标虚拟腿腿部角度 1.57 = pai/2  居中

	TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
{
	float targetLegLength = target.legLength;
	// //测试左腿
	// float legLength = leftLegPos.length;
	// float dLegLength = leftLegPos.dLength;

	// PID_CascadeCalc(&legLengthPID,targetLegLength,legLength, dLegLength);
	// PID_CascadeCalc(&legAnglePID,targetLegAngle, leftLegPos.angle, leftLegPos.dAngle);
		
	// float leftJointTorque[2]={0};
	// float leftForce = legLengthPID.output;
	// float leftTp =  - legAnglePID.output;
	// leg_vmc_conv(leftForce, leftTp, leftJoint[1].angle, leftJoint[0].angle, leftJointTorque);
	// Motor_SetTorque(&leftJoint[0], -leftJointTorque[0]);
	// Motor_SetTorque(&leftJoint[1], -leftJointTorque[1]);

	//测试右腿
	float legLength = rightLegPos.length;
	float dLegLength = rightLegPos.dLength;

	PID_CascadeCalc(&legLengthPID,targetLegLength,legLength, dLegLength);
	PID_CascadeCalc(&legAnglePID,targetLegAngle, rightLegPos.angle, rightLegPos.dAngle);
		
	float rightJointTorque[2]={0};
	float rightForce = legLengthPID.output;
	float rightTp =   -legAnglePID.output;
	leg_vmc_conv(rightForce, rightTp, rightJoint[1].angle, rightJoint[0].angle, rightJointTorque);
	Motor_SetTorque(&rightJoint[0], -rightJointTorque[0]);
	Motor_SetTorque(&rightJoint[1], -rightJointTorque[1]);

	//测试两条腿捏
	// float legLength = (leftLegPos.length + rightLegPos.length) / 2;
	// float dLegLength = (leftLegPos.dLength + rightLegPos.dLength) / 2;

	// PID_CascadeCalc(&legLengthPID,targetLegLength,legLength, dLegLength);
	// PID_CascadeCalc(&legAnglePID,0, leftLegPos.angle - rightLegPos.angle,  leftLegPos.dAngle - rightLegPos.dAngle);
		
	// float leftJointTorque[2]={0};
	// float leftForce = legLengthPID.output;
	// float leftTp =  - legAnglePID.output;
	// leg_vmc_conv(leftForce, leftTp, leftJoint[1].angle, leftJoint[0].angle, leftJointTorque);

	// float rightJointTorque[2]={0};
	// float rightForce = legLengthPID.output;
	// float rightTp =   + legAnglePID.output;
	// leg_vmc_conv(rightForce, rightTp, rightJoint[1].angle, rightJoint[0].angle, rightJointTorque);

	// Motor_SetTorque(&leftJoint[0], -leftJointTorque[0]);
	// Motor_SetTorque(&leftJoint[1], -leftJointTorque[1]);
	// Motor_SetTorque(&rightJoint[0], -rightJointTorque[0]);
	// Motor_SetTorque(&rightJoint[1], -rightJointTorque[1]);

	vTaskDelayUntil(&xLastWakeTime, 4); //4ms控制周期
}
}

//目标量更新任务(根据蓝牙收到的目标量计算实际控制算法的给定量)
void Ctrl_TargetUpdateTask(void *arg)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();
	float speedSlopeStep = 0.003f;
	while (1)
	{
		//根据当前腿长计算速度斜坡步长(腿越短越稳定，加减速斜率越大)
		float legLength = (leftLegPos.length + rightLegPos.length) / 2;
		speedSlopeStep = -(legLength - 0.07f) * 0.02f + 0.002f;

		//计算速度斜坡，斜坡值更新到target.speed
		if(fabs(target.speedCmd - target.speed) < speedSlopeStep)
			target.speed = target.speedCmd;
		else
		{
			if(target.speedCmd - target.speed > 0)
				target.speed += speedSlopeStep;
			else
				target.speed -= speedSlopeStep;
		}

		//计算位置目标，并限制在当前位置的±0.1m内
		target.position += target.speed * 0.004f;
		if(target.position - stateVar.x > 0.1f)
			target.position = stateVar.x + 0.1f; 
		else if(target.position - stateVar.x < -0.1f)
			target.position = stateVar.x - 0.1f;

		//限制速度目标在当前速度的±0.3m/s内
		if(target.speed - stateVar.dx > 0.3f)
			target.speed = stateVar.dx + 0.3f;
		else if(target.speed - stateVar.dx < -0.3f)
			target.speed = stateVar.dx - 0.3f;

		//计算yaw方位角目标
		target.yawAngle += target.yawSpeedCmd * 0.004f;
		
		vTaskDelayUntil(&xLastWakeTime, 4); //每4ms更新一次
	}
}


//没有起立
void CtrlBasic_Task(void *arg)
{
	const float wheelRadius = 0.0325f; 	//m，车轮半径
	const float legMass = 0.052f; 		//kg，腿部质量

	TickType_t xLastWakeTime = xTaskGetTickCount();

	//手动为反馈矩阵和输出叠加一个系数，用于手动优化控制效果
	//                     theta dTheta  x,   dx,   phi, dPhi
	float kRatio[2][6] = {
		{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},	//lqrOutT		轮子
		{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}	//lqrOutTp		髋关节
		};			//基本不用调

	float lqrTRatio = 0.35f,				    //轮子
		  lqrTpRatio = 0.75f;					//髋关节

	target.rollAngle = 0.0f;
	target.legLength = 0.065f;
	target.speed = 0.0f;
	target.position = (leftWheel.angle + rightWheel.angle) / 2 * wheelRadius;

	while (1)
	{
		//计算状态变量
		stateVar.phi = imuData.pitch;
		stateVar.dPhi = imuData.pitchSpd;
		stateVar.x = (leftWheel.angle + rightWheel.angle) / 2 * wheelRadius;
		stateVar.dx = (leftWheel.speed + rightWheel.speed) / 2 * wheelRadius;
		stateVar.theta = (leftLegPos.angle + rightLegPos.angle) / 2 - M_PI_2 - imuData.pitch;
		stateVar.dTheta = (leftLegPos.dAngle + rightLegPos.dAngle) / 2 - imuData.pitchSpd;

		float legLength = (leftLegPos.length + rightLegPos.length) / 2;
		float dLegLength = (leftLegPos.dLength + rightLegPos.dLength) / 2;	

		//如果正在站立准备状态，则不进行后续控制
		if(standupState == StandupState_Prepare)
		{
			vTaskDelayUntil(&xLastWakeTime, 4);
			continue;
		}


		//计算LQR反馈矩阵
		float kRes[12] = {0}, k[2][6] = {0};
		lqr_k(legLength, kRes);	
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 2; j++)
				k[j][i] = kRes[i * 2 + j] * kRatio[j][i];
		}

		//准备状态变量
		float x[6] = {stateVar.theta, stateVar.dTheta, stateVar.x, stateVar.dx, stateVar.phi, stateVar.dPhi};
		//与给定量作差
		x[2] -= target.position;
		x[3] -= target.speed;

		//矩阵相乘，计算LQR输出
		float lqrOutT = k[0][0] * x[0] + k[0][1] * x[1] + k[0][2] * x[2] + k[0][3] * x[3] + k[0][4] * x[4] + k[0][5] * x[5];
		float lqrOutTp = k[1][0] * x[0] + k[1][1] * x[1] + k[1][2] * x[2] + k[1][3] * x[3] + k[1][4] * x[4] + k[1][5] * x[5];

		PID_CascadeCalc(&yawPID, target.yawAngle, imuData.yaw, imuData.yawSpd);

		Motor_SetTorque(&leftWheel, -lqrOutT * lqrTRatio - yawPID.output);
		Motor_SetTorque(&rightWheel, -lqrOutT * lqrTRatio + yawPID.output);

		PID_CascadeCalc(&legLengthPID,target.legLength, legLength, dLegLength);

		//计算左右腿角度差PID输出
		PID_CascadeCalc(&legAnglePID, 0, leftLegPos.angle - rightLegPos.angle, leftLegPos.dAngle - rightLegPos.dAngle);
		PID_CascadeCalc(&rollPID, target.rollAngle, imuData.roll, imuData.rollSpd);

		float leftForce = legLengthPID.output+6-rollPID.output ;
		//float leftTp = lqrOutTp * lqrTpRatio - legAnglePID.output;
		float leftTp = lqrOutTp * lqrTpRatio - legAnglePID.output * (leftLegPos.length / 0.07f);

		float leftJointTorque[2]={0};
		leg_vmc_conv(leftForce, leftTp, leftJoint[1].angle, leftJoint[0].angle, leftJointTorque);

		float rightForce = legLengthPID.output+6+rollPID.output ;	
		//float rightTp = lqrOutTp * lqrTpRatio + legAnglePID.output;
		float rightTp = lqrOutTp * lqrTpRatio + legAnglePID.output * (rightLegPos.length / 0.07f);
		float rightJointTorque[2]={0};
		leg_vmc_conv(rightForce, rightTp, rightJoint[1].angle, rightJoint[0].angle, rightJointTorque);

		//设定关节电机输出扭矩
		Motor_SetTorque(&leftJoint[0], -leftJointTorque[0]);
		Motor_SetTorque(&leftJoint[1], -leftJointTorque[1]);
		Motor_SetTorque(&rightJoint[0], -rightJointTorque[0]);
		Motor_SetTorque(&rightJoint[1], -rightJointTorque[1]);

		vTaskDelayUntil(&xLastWakeTime, 4); //4ms控制周期
	}
	
}

void Ctrl_Init(void)
{
	PID_Init(&rollPID.inner, 0.5, 0, 2.5, 0, 5);
	PID_Init(&rollPID.outer, 10, 0, 0, 0, 3);
	PID_SetErrLpfRatio(&rollPID.inner, 0.1f);

	PID_Init(&yawPID.inner, 0.01, 0, 0, 0, 0.1);
	PID_Init(&yawPID.outer, 10, 0, 0, 0, 2);

	PID_Init(&legLengthPID.inner, 10.0f, 1, 30.0f, 2.0f, 10.0f);
	PID_Init(&legLengthPID.outer, 5.0f, 0, 0.0f, 0.0f, 0.5f);
	PID_SetErrLpfRatio(&legLengthPID.inner, 0.5f);
	PID_Init(&legAnglePID.inner, 0.04, 0, 0, 0, 1);
	PID_Init(&legAnglePID.outer, 12, 0, 0, 0, 20);
	PID_SetErrLpfRatio(&legAnglePID.outer, 0.5f);

	xTaskCreate(Ctrl_TargetUpdateTask, "Ctrl_TargetUpdateTask", 4096, NULL, 3, NULL);
	vTaskDelay(2);
	//xTaskCreate(Ctrl_StandupPrepareTask, "StandupPrepare_Task", 4096, NULL, 1, NULL);
	//xTaskCreate(vSinGeneratorTask, "vSinGeneratorTask", 4096, NULL, 1, NULL);
	//xTaskCreate(VMC_TestTask, "VMC_TestTask", 4096, NULL, 1, NULL);
	xTaskCreate(CtrlBasic_Task, "CtrlBasic_Task", 4096, NULL, 1, NULL);//没有离地检测器

}