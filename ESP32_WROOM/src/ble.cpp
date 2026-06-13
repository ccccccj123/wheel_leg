
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "ctrl.h"
//每个服务、特征和描述符都有一个UUID(通用唯一标识符)，UUID用于唯一标识信息，它可以识别蓝牙设备提供的特定服务。
// See the following for generating UUIDs: https://www.uuidgenerator.net/
//
#define BLE_NAME "Bot"
#define SERVICE_UUID  "ba0d1b7e-7ad8-11ef-b864-0242ac120002"
#define CHARACTERISTIC_UUID_RX "c7ebcf24-7ad8-11ef-b864-0242ac120002"
#define CHARACTERISTIC_UUID_TX "d3c5baf8-7ad8-11ef-b864-0242ac120002"

#define MAX_SPEED           (0.6F)
#define MAX_YAWSPEED        (0.5F)
#define MAX_LEG_LENGTH      (0.115F)
#define MIN_LEG_LENGTH      (0.068F)

#define _constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

//蓝牙全局指针捏 先置空防止意外
BLEServer *pServer = NULL;
BLEService *pService =NULL;
BLECharacteristic *pTxCharacteristic =NULL;
BLECharacteristic *pRxCharacteristic =NULL;

bool deviceConnected = false;                //本次连接状态

class MyServerCallbacks : public BLEServerCallbacks
{
public:
	void onConnect(BLEServer *server)
	{
		Serial.println("onConnect");
		server->getAdvertising()->stop();
        deviceConnected = true;
	}
	void onDisconnect(BLEServer *server)
	{
		Serial.println("onDisconnect");
		server->getAdvertising()->start();
        deviceConnected = false;
	}
};


class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)//特征写入事件，收到蓝牙数据
    {
        uint8_t *data = pCharacteristic->getData(); //接收信息 //getData 返回的是uint8 getValue 返回的是string
        uint32_t len = pCharacteristic->getLength();
        int8_t speed_t,yawsp_t,length_t;

		if(data[0] == 0xA5 && data[6] == 0x5A && len == 7) //合法报文
		{
            //Serial.println("Get Data!");
            if (data[1] == 88 ) //第一次接到起立命令
            {
                if(standupState == StandupState_None)
                {
                    standupState = StandupState_Standup;
                }
                else if (standupState == StandupState_Standup) //遥控器开始接管 
                {
                    // speed_t = data[2];
                    // yawsp_t = data[3];
                    // length_t = data[4]; // 6.8cm - 12cm
                    //Serial.printf("%d,%d\r\n", speed_t,yawsp_t);
                    target.speedCmd = ((float)((signed char)data[2]) / 100.0f) * MAX_SPEED / 1.27f;//signed char 然后到 float
                    target.yawSpeedCmd = ((float)((signed char)data[3]) / -100.0f) * MAX_YAWSPEED / 1.27f; // 同上
                    target.legLength = _constrain(((float)((signed char)data[4])/1000.0f),MIN_LEG_LENGTH,MAX_LEG_LENGTH);
                    //Serial.printf("%.4f,%.4f,%.4f\r\n", target.speedCmd,target.yawSpeedCmd,target.legLength);
                }
            }

		}


        
    }
};


void BLE_Init(void)
{
    BLEDevice::init(BLE_NAME);      //创建BLE设备

    pServer = BLEDevice::createServer();   //创建BLE服务器
    pServer->setCallbacks(new MyServerCallbacks());   //设置回调函数
    pService = pServer->createService(SERVICE_UUID);   //创建服务

    pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,BLECharacteristic::PROPERTY_NOTIFY);
    pTxCharacteristic->addDescriptor(new BLE2902());    //创建并添加描述符
    pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,BLECharacteristic::PROPERTY_WRITE_NR);   //与 WRITE 属性类似，不同之处在于它不等待服务器的响应
    pRxCharacteristic->setCallbacks(new MyCallbacks()); //设置回调

    pService->start();                  // 开始服务
    pServer->getAdvertising()->start(); // 开始广播
    Serial.println("Waiting for connection \r\n");

}

uint8_t txValue = 0;     //后面需要发送的值
void BLE_TestTask(void *arg)
{

	TickType_t xLastWakeTime = xTaskGetTickCount();
	while(1)
	{
        if (deviceConnected)
        {
            pTxCharacteristic->setValue(&txValue, 1); // 设置要发送的值为1
            pTxCharacteristic->notify();              // 广播
            txValue++;                                // 指针数值自加1
        }
            vTaskDelayUntil(&xLastWakeTime, 100); //100ms轮询
    }
}

void BLE_Test(void)
{
    xTaskCreate(BLE_TestTask, "BLE_TestTask", 4096, NULL, 1, NULL);
}

