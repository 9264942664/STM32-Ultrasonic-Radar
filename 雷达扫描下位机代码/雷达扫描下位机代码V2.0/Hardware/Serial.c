#include "stm32f10x.h"
#include "Serial.h"

#define SERIAL_USART            USART1
#define SERIAL_USART_IRQn       USART1_IRQn
#define SERIAL_TX_GPIO          GPIOA
#define SERIAL_TX_PIN           GPIO_Pin_9
#define SERIAL_RX_PIN           GPIO_Pin_10
#define SERIAL_RX_BUFFER_SIZE   128

static volatile uint8_t Serial_RxBuffer[SERIAL_RX_BUFFER_SIZE];
static volatile uint16_t Serial_RxHead = 0;
static volatile uint16_t Serial_RxTail = 0;

static uint16_t Serial_NextIndex(uint16_t index)
{
    return (index + 1) % SERIAL_RX_BUFFER_SIZE;
}

void Serial_Init(uint32_t baudrate)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = SERIAL_TX_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(SERIAL_TX_GPIO, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = SERIAL_RX_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(SERIAL_TX_GPIO, &GPIO_InitStruct);

    USART_InitTypeDef USART_InitStruct;
    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(SERIAL_USART, &USART_InitStruct);

    USART_ITConfig(SERIAL_USART, USART_IT_RXNE, ENABLE);
    USART_Cmd(SERIAL_USART, ENABLE);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = SERIAL_USART_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    Serial_RxHead = 0;
    Serial_RxTail = 0;
}

void Serial_SendByte(uint8_t byte)
{
    USART_SendData(SERIAL_USART, byte);
    while (USART_GetFlagStatus(SERIAL_USART, USART_FLAG_TC) == RESET)
    {
    }
}

void Serial_SendData(const uint8_t *data, uint16_t length)
{
    for (uint16_t i = 0; i < length; i++)
    {
        Serial_SendByte(data[i]);
    }
}

void Serial_SendString(const char *str)
{
    while (*str != '\0')
    {
        Serial_SendByte((uint8_t)*str);
        str++;
    }
}

uint8_t Serial_Available(void)
{
    return (uint8_t)((SERIAL_RX_BUFFER_SIZE + Serial_RxHead - Serial_RxTail) % SERIAL_RX_BUFFER_SIZE);
}

uint8_t Serial_Read(void)
{
    if (Serial_RxHead == Serial_RxTail)
    {
        return 0;
    }

    uint8_t value = Serial_RxBuffer[Serial_RxTail];
    Serial_RxTail = Serial_NextIndex(Serial_RxTail);
    return value;
}

uint8_t Serial_Peek(void)
{
    if (Serial_RxHead == Serial_RxTail)
    {
        return 0;
    }

    return Serial_RxBuffer[Serial_RxTail];
}

void Serial_Flush(void)
{
    Serial_RxHead = Serial_RxTail = 0;
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(SERIAL_USART, USART_IT_RXNE) != RESET)
    {
        uint8_t data = (uint8_t)USART_ReceiveData(SERIAL_USART);
        uint16_t nextHead = Serial_NextIndex(Serial_RxHead);

        if (nextHead != Serial_RxTail)
        {
            Serial_RxBuffer[Serial_RxHead] = data;
            Serial_RxHead = nextHead;
        }

        USART_ClearITPendingBit(SERIAL_USART, USART_IT_RXNE);
    }
}
