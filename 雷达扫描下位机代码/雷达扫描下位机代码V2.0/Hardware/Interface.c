#include "stm32f10x.h"                  // Device header
#include "Oled.h"
#include "Servo.h"
#include "HCSR04.h"
#include "delay.h"
#include "Serial.h"
#include <math.h>
#include <stdio.h>
#include "Encoder.h"

#define     OLED_WIDTH      128
#define     OLED_HEIGHT     64
#define     PI              3.14159


typedef struct
{
    int CenterX, CenterY;
    int Angle;
    int VertexX, VertexY;
    int Radius;
} Pointer;

float LastRange = 0;
uint8_t Obj[OLED_WIDTH][OLED_HEIGHT];
Pointer pointer;

// 辅助函数：发送整数到串口
static void Serial_SendNumber(uint16_t num)
{
    char str[6] = {0};
    sprintf(str, "%d", num);
    Serial_SendString(str);
}


void Interface_Init(void)
{
	Encoder_Init();
    OLED_ShowString(0, 0, "Deg:", OLED_6X8);
    OLED_ShowString(0, 8, "Dist:", OLED_6X8);
	OLED_ShowString(55, 8, "cm", OLED_6X8);

    pointer.Angle = 180;
    pointer.CenterX = OLED_WIDTH / 2- 1;
    pointer.CenterY = OLED_HEIGHT- 1;
    pointer.Radius = OLED_WIDTH / 2;
    pointer.VertexX = pointer.CenterX - (float)pointer.Radius * cos((float)pointer.Angle * PI / 180);
    pointer.VertexY = pointer.CenterY - (float)pointer.Radius * sin((float)pointer.Angle * PI / 180);

}

void Interface_Run(uint8_t scanTime, int8_t range)
{
	static uint8_t begin_flag = 0;
    int  objectx, objecty;
    float dist;
	
	if(begin_flag == 0)
	{
		begin_flag = 1;
	}
	
	else if(begin_flag != 0)
	{
	range = LastRange;
	}
	
    for (pointer.Angle = 180; pointer.Angle >= 0; pointer.Angle --)///////////0->180
    {
		OLED_ShowString(0, 0, "Deg:", OLED_6X8);
		OLED_ShowString(0, 8, "Dist:", OLED_6X8);
		OLED_ShowString(55, 8, "cm", OLED_6X8);
		Servo_SetAngle(pointer.Angle);
        pointer.VertexX = pointer.CenterX + (float)pointer.Radius * cos((float)pointer.Angle * PI / 180);
        pointer.VertexY = pointer.CenterY - (float)pointer.Radius * sin((float)pointer.Angle * PI / 180);
        OLED_DrawLine(pointer.CenterX, pointer.CenterY, pointer.VertexX, pointer.VertexY);
        OLED_Update();
		Delay_ms(scanTime);
        dist = HCSR04_GetDistance();
        Serial_SendString("Angle:");
        Serial_SendNumber(pointer.Angle);
        Serial_SendString(" Dist:");
        Serial_SendNumber((uint16_t)dist);
        Serial_SendString("\r\n");
		encoder_value = Encoder_GetValue();
		range = (range + encoder_value) > 10 ? 10 : (range + encoder_value);
		range = (range + encoder_value) < 1 ? 1 : (range + encoder_value);
		TIM_SetCounter(TIM3, 0);
		
        if (dist / range<= pointer.Radius)
        {
            objectx = pointer.CenterX + dist / range * cos((float)pointer.Angle * PI / 180);
            objecty = pointer.CenterY - dist / range * sin((float)pointer.Angle * PI / 180);
            Obj[objectx][objecty] = 1;
        }

        OLED_Clear();
		OLED_ShowString(0, 0, "Deg:", OLED_6X8);
		OLED_ShowString(0, 8, "Dist:", OLED_6X8);
		OLED_ShowString(55, 8, "cm", OLED_6X8);
		OLED_ShowNum(30, 0, pointer.Angle, 3, OLED_6X8);
		OLED_ShowNum(30, 8, dist, 3, OLED_6X8);
		OLED_ShowNum(30, 16, range, 2, OLED_6X8);
		OLED_ShowString(45, 16, "X", OLED_6X8);
        for (int x = 0; x < OLED_WIDTH; x ++)
        {
            for (int y = 0; y < OLED_HEIGHT; y++)
            {
                if (Obj[x][y] == 1)
                {
                    OLED_DrawPoint(x, y);
				
                }

            }
        }

        OLED_Update();
        Delay_ms(scanTime );

    }
			for(int x = 0; x < OLED_WIDTH; x++)
		{
			for(int y = 0; y < OLED_HEIGHT; y++)
			{
				Obj[x][y] = 0;
			}
		}
	    for (pointer.Angle = 0; pointer.Angle <= 180; pointer.Angle ++)////////180->0
    {
		OLED_ShowString(0, 0, "Deg:", OLED_6X8);
		OLED_ShowString(0, 8, "Dist:", OLED_6X8);
		Servo_SetAngle(pointer.Angle);
        pointer.VertexX = pointer.CenterX + (float)pointer.Radius * cos((float)pointer.Angle * PI / 180);
        pointer.VertexY = pointer.CenterY - (float)pointer.Radius * sin((float)pointer.Angle * PI / 180);
        OLED_DrawLine(pointer.CenterX, pointer.CenterY, pointer.VertexX, pointer.VertexY);
        OLED_Update();
		Delay_ms(scanTime);
        dist = HCSR04_GetDistance();
        Serial_SendString("Angle:");
        Serial_SendNumber(pointer.Angle);
        Serial_SendString(" Dist:");
        Serial_SendNumber((uint16_t)dist);
        Serial_SendString("\r\n");
		encoder_value = Encoder_GetValue();
		range = (range + encoder_value) > 10 ? 10 : (range + encoder_value);
		range = (range + encoder_value) < 1 ? 1 : (range + encoder_value);
		TIM_SetCounter(TIM3, 0);

        if (dist / range<= pointer.Radius)
        {
            objectx = pointer.CenterX + dist / range * cos((float)pointer.Angle * PI / 180);
            objecty = pointer.CenterY - dist / range * sin((float)pointer.Angle * PI / 180);
            Obj[objectx][objecty] = 1;
        }

        OLED_Clear();
		OLED_ShowString(0, 0, "Deg:         ", OLED_6X8);
        OLED_ShowString(0, 8, "Dist:    cm", OLED_6X8);
		OLED_ShowNum(30, 0, pointer.Angle, 3, OLED_6X8);
		OLED_ShowNum(30, 8, dist, 3, OLED_6X8);
		OLED_ShowNum(30, 16, range, 2, OLED_6X8);
		OLED_ShowString(45, 16, "X", OLED_6X8);
        for (int x = 0; x < OLED_WIDTH; x ++)
        {
            for (int y = 0; y < OLED_HEIGHT; y++)
            {
                if (Obj[x][y] == 1)
                {
                    OLED_DrawPoint(x, y);
                }

            }
        }

        OLED_Update();
        Delay_ms(scanTime);
    }
			for(int x = 0; x < OLED_WIDTH; x++)
		{
			for(int y = 0; y < OLED_HEIGHT; y++)
			{
				Obj[x][y] = 0;
			}
		}
		LastRange = range;
}
