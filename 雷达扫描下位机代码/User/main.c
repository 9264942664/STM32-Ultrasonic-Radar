#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Servo.h"
#include "HCSR04.h"
#include "Interface.h"
#include "encoder.h"
#include "Serial.h"

int main(void)
{
	OLED_Init();
	Servo_Init();
	HCSR04_Init();
	Interface_Init();
	Serial_Init(115200);
	Serial_SendString("=== Radar System Start ===\r\n");
	
	while(1)
	{
		Interface_Run(0, 1);		//第一个参数设置扫描速度， 第二个参数设置雷达范围，由旋转编码器控制 测试
	}

}
