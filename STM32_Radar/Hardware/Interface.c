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

static void Serial_SendNumber(uint16_t num)
{
    char str[6] = {0};
    sprintf(str, "%d", num);
    Serial_SendString(str);
}


void Interface_Init(void)
{
    Encoder_Init();
    OLED_ShowString(0, 0, "Dist:", OLED_6X8);
    OLED_ShowString(55, 0, "cm", OLED_6X8);

    pointer.Angle = 180;
    pointer.CenterX = OLED_WIDTH / 2 - 1;
    pointer.CenterY = OLED_HEIGHT - 1;
    pointer.Radius = OLED_WIDTH / 2;
    pointer.VertexX = pointer.CenterX - (float)pointer.Radius * cos((float)pointer.Angle * PI / 180);
    pointer.VertexY = pointer.CenterY - (float)pointer.Radius * sin((float)pointer.Angle * PI / 180);
}

void Interface_Run(uint8_t scanTime, int8_t range)
{
    static uint8_t begin_flag = 0;
    int  objectx, objecty;
    float dist;

    if (begin_flag == 0) {
        begin_flag = 1;
    } else {
        range = LastRange;
    }

    // ── Sweep: 180° → 0° ──
    for (pointer.Angle = 180; pointer.Angle >= 0; pointer.Angle--)
    {
        Servo_SetAngle(pointer.Angle);
        Delay_ms(scanTime);
        dist = HCSR04_GetDistance();

        Serial_SendNumber(pointer.Angle);
        Serial_SendString(",");
        Serial_SendNumber((uint16_t)dist);
        Serial_SendString(",");
        Serial_SendNumber(range);
        Serial_SendString("\r\n");

        encoder_value = Encoder_GetValue();
        {
            int r = range + encoder_value;
            if (r > 10) r = 10;
            if (r < 1)  r = 1;
            range = r;
        }
        TIM_SetCounter(TIM3, 0);

        if (dist / range <= pointer.Radius) {
            objectx = pointer.CenterX + dist / range * cos((float)pointer.Angle * PI / 180);
            objecty = pointer.CenterY - dist / range * sin((float)pointer.Angle * PI / 180);
            Obj[objectx][objecty] = 1;
        }

        pointer.VertexX = pointer.CenterX + (float)pointer.Radius * cos((float)pointer.Angle * PI / 180);
        pointer.VertexY = pointer.CenterY - (float)pointer.Radius * sin((float)pointer.Angle * PI / 180);

        OLED_Clear();
        OLED_ShowString(0, 0, "Dist:", OLED_6X8);
        OLED_ShowString(55, 0, "cm", OLED_6X8);
        OLED_ShowNum(30, 0, dist, 3, OLED_6X8);
        OLED_ShowNum(108, 0, range, 2, OLED_6X8);
        OLED_ShowString(120, 0, "X", OLED_6X8);
        OLED_DrawLine(pointer.CenterX, pointer.CenterY, pointer.VertexX, pointer.VertexY);

        for (int x = 0; x < OLED_WIDTH; x++) {
            for (int y = 0; y < OLED_HEIGHT; y++) {
                if (Obj[x][y] == 1)
                    OLED_DrawPoint(x, y);
            }
        }
        OLED_Update();
    }

    // Clear Obj for next sweep
    for (int x = 0; x < OLED_WIDTH; x++)
        for (int y = 0; y < OLED_HEIGHT; y++)
            Obj[x][y] = 0;

    // ── Sweep: 0° → 180° ──
    for (pointer.Angle = 0; pointer.Angle <= 180; pointer.Angle++)
    {
        Servo_SetAngle(pointer.Angle);
        Delay_ms(scanTime);
        dist = HCSR04_GetDistance();

        Serial_SendNumber(pointer.Angle);
        Serial_SendString(",");
        Serial_SendNumber((uint16_t)dist);
        Serial_SendString(",");
        Serial_SendNumber(range);
        Serial_SendString("\r\n");

        encoder_value = Encoder_GetValue();
        {
            int r = range + encoder_value;
            if (r > 10) r = 10;
            if (r < 1)  r = 1;
            range = r;
        }
        TIM_SetCounter(TIM3, 0);

        if (dist / range <= pointer.Radius) {
            objectx = pointer.CenterX + dist / range * cos((float)pointer.Angle * PI / 180);
            objecty = pointer.CenterY - dist / range * sin((float)pointer.Angle * PI / 180);
            Obj[objectx][objecty] = 1;
        }

        pointer.VertexX = pointer.CenterX + (float)pointer.Radius * cos((float)pointer.Angle * PI / 180);
        pointer.VertexY = pointer.CenterY - (float)pointer.Radius * sin((float)pointer.Angle * PI / 180);

        OLED_Clear();
        OLED_ShowString(0, 0, "Dist:", OLED_6X8);
        OLED_ShowString(55, 0, "cm", OLED_6X8);
        OLED_ShowNum(30, 0, dist, 3, OLED_6X8);
        OLED_ShowNum(108, 0, range, 2, OLED_6X8);
        OLED_ShowString(120, 0, "X", OLED_6X8);
        OLED_DrawLine(pointer.CenterX, pointer.CenterY, pointer.VertexX, pointer.VertexY);

        for (int x = 0; x < OLED_WIDTH; x++) {
            for (int y = 0; y < OLED_HEIGHT; y++) {
                if (Obj[x][y] == 1)
                    OLED_DrawPoint(x, y);
            }
        }
        OLED_Update();
    }

    // Clear Obj
    for (int x = 0; x < OLED_WIDTH; x++)
        for (int y = 0; y < OLED_HEIGHT; y++)
            Obj[x][y] = 0;

    LastRange = range;
}
