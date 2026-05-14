#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "Timer.h"
#include "Oled.h"
#include "Encoder.h"

#define     HCSR04_PORT     GPIOB
#define     TRIG_PIN        GPIO_Pin_12
#define     ECHO_PIN        GPIO_Pin_13

#define     TIMEOUT         1000000

int8_t encoder_value = 0;

void HCSR04_Init(void)
{
    Timer_init();
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Pin = TRIG_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HCSR04_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = ECHO_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(HCSR04_PORT, &GPIO_InitStruct);

    GPIO_ResetBits(HCSR04_PORT, TRIG_PIN | ECHO_PIN);
}

uint16_t HCSR04_GetDistance(void)
{
    uint16_t Distance, sum_dist, avg_dist;
    uint16_t Count = 0;
    uint32_t busy = 0;;
    uint8_t Sample_Count = 0;


    for (int i = 0; i < 3; i ++)
    {
        TIM_SetCounter(TIM1, 0);
        GPIO_SetBits(HCSR04_PORT, TRIG_PIN);
        Delay_us(40);
        GPIO_ResetBits(HCSR04_PORT, TRIG_PIN);

        while (GPIO_ReadInputDataBit(HCSR04_PORT, ECHO_PIN) == 0)
        {

            if (++busy > TIMEOUT)
            {
                return 500;
            }
        }

        busy = 0;
        TIM_Cmd(TIM1, ENABLE);

        while (GPIO_ReadInputDataBit(HCSR04_PORT, ECHO_PIN) == 1)
        {

            if (++busy > TIMEOUT)
            {
                TIM_Cmd(TIM1, DISABLE);
                return 505;
            }
        }

        busy = 0;
        TIM_Cmd(TIM1, DISABLE);
        Count = TIM_GetCounter(TIM1);
        TIM_SetCounter(TIM1, 0);
        Distance = (float)Count * 0.00002 * 34000 / 2;          //距离单位为厘米

        if (Distance <= 490 && Distance > 2)
        {
            sum_dist += Distance;
            Sample_Count++;
        }

        Delay_ms(10);
    }
	if(Sample_Count == 0){return 510;}
    avg_dist = sum_dist / Sample_Count;

    return avg_dist;
}


void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
    {
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
    }
}
