#include "uart.h"
#include <stm32g0b1xx.h>
void UART_Init(USART_TypeDef* uart, uint32_t baudRate, char interrupt)
{
    uint32_t periphClk = 16000000U;
    uint32_t brrValue;

    uart->CR1 &= ~USART_CR1_UE;

    brrValue = periphClk / baudRate;
    uart->BRR = brrValue;

    uart->CR1 = USART_CR1_TE | USART_CR1_RE;

    if (interrupt != 0)
    {
        uart->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
    }

    uart->CR1 |= USART_CR1_UE;

    while ((uart->ISR & USART_ISR_TEACK) == 0U)
    {
    }

    while ((uart->ISR & USART_ISR_REACK) == 0U)
    {
    }
}

void UART_TxByte(USART_TypeDef* uart, uint8_t byte)
{
    while ((uart->ISR & USART_ISR_TXE_TXFNF) == 0U)
    {
    }

    uart->TDR = byte;
}

void UART_TxStr(USART_TypeDef* uart, const char* string)
{
    while (*string != '\0')
    {
        UART_TxByte(uart, (uint8_t)*string);
        string++;
    }
}

void UART_TxBuffer(USART_TypeDef* uart, uint8_t* buffer, uint16_t size)
{
    uint16_t i;

    for (i = 0; i < size; i++)
    {
        UART_TxByte(uart, buffer[i]);
    }
}

uint8_t UART_RxByte(USART_TypeDef* uart, uint8_t* byte)
{
    if ((uart->ISR & USART_ISR_RXNE_RXFNE) != 0U)
    {
        *byte = (uint8_t)uart->RDR;
        return 1U;
    }

    return 0U;
}

void TERM_ClearScreen (USART_TypeDef * pUSART)
{
    UART_TxStr(pUSART, "\x1B[2J");
    UART_TxStr(pUSART, "\x1B[H");
}

void TERM_GotoXY (USART_TypeDef * pUSART, int iCol, int iRow)
{
    char buf[20];
    int n = 0;
    int rowTens;
    int rowOnes;
    int colTens;
    int colOnes;

    rowTens = iRow / 10;
    rowOnes = iRow % 10;
    colTens = iCol / 10;
    colOnes = iCol % 10;

    buf[n++] = 0x1B;
    buf[n++] = '[';

    if (rowTens > 0)
    {
        buf[n++] = (char)('0' + rowTens);
    }
    buf[n++] = (char)('0' + rowOnes);
    buf[n++] = ';';

    if (colTens > 0)
    {
        buf[n++] = (char)('0' + colTens);
    }
    buf[n++] = (char)('0' + colOnes);
    buf[n++] = 'H';
    buf[n] = '\0';

    UART_TxStr(pUSART, buf);
}

void TERM_TxStringXY (USART_TypeDef * pUSART, int iCol, int iRow, char * pStr)
{
    TERM_GotoXY(pUSART, iCol, iRow);
    UART_TxStr(pUSART, pStr);
}