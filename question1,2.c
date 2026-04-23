#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include <stdio.h> // Required for sprintf

int main(void) {
    uint32_t counter = 0;
    char buffer[20]; // Buffer to hold the formatted string

    // 1. Initialize the System Clock (e.g., 64MHz)
    Init_SysClock_PLL(PLL_64MHZ);

    // 2. Enable Clocks for GPIOA, TIM6, and USART2
    Enable_Port_Clock(GPIOA);
    RCC->APBENR1 |= RCC_APBENR1_TIM6EN;
    Enable_Serial_Clock(USART2);

    // 3. Configure GPIOs
    // PA5 as Output (LED4 for Q1)
    Set_Pin_Out(GPIOA, 5);
    // PA2 as Alternate Function 1 (USART2_TX for Q2)
    Config_Pin_AF(GPIOA, 2, 1); 

    // 4. Configure USART2 (230,400 baud rate, no interrupts)
    Initialize_Serial(USART2, 230400, 0);
    
    // Clear the VT100 terminal screen at startup
    VT100_Clear(USART2);

    // 5. Configure TIM6 for a 100ms period (Q1 setup)
    // PSC: 1ms ticks, ARR: 100 ticks
    Setup_Tim_Base(TIM6, (SystemCoreClock / 1000) - 1, 100 - 1);

    // 6. Start the timer
    StartStop_Tim(TIM6, 1);

    while(1) {
        // 7. Poll the Update Interrupt Flag (UIF) every 100ms
        if (Check_Tim_Update(TIM6)) {
            
            // --- Q1 Action: Toggle LED ---
            Pin_Tgl(GPIOA, 5);
            
            // --- Q2 Action: Format and send the counter ---
            // %05lu formats the unsigned long integer to 5 digits, padded with leading zeros
            sprintf(buffer, "%05lu", counter);
            
            // Print the buffer at Col (X) = 20, Row (Y) = 10
            VT100_PrintAt(USART2, 20, 10, buffer);
            
            // Increment the counter for the next event
            counter++;
        }
    }
}