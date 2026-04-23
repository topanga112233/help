#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include <stdio.h> // Required for sprintf

int main(void) {
    uint32_t counter = 0;
    char buffer[20]; // Buffer to hold the formatted string

    // ==========================================
    // 1. Initialize the System Clock (e.g., 64MHz)
    // ==========================================
    Init_SysClock_PLL(PLL_64MHZ);

    // ==========================================
    // 2. Enable Peripheral Clocks
    // ==========================================
    Enable_Port_Clock(GPIOA);
    Enable_Port_Clock(GPIOC);
    RCC->APBENR1 |= RCC_APBENR1_TIM6EN;    // Clock for TIM6 (Q1 & Q2)
    RCC->APBENR2 |= RCC_APBENR2_TIM14EN;   // Clock for TIM14 (Q3)
    Enable_Serial_Clock(USART2);           // Clock for USART2 (Q2)

    // ==========================================
    // 3. GPIO Configuration
    // ==========================================
    // Q1: PA5 as Output (LED4)
    Set_Pin_Out(GPIOA, 5);
    
    // Q2: PA2 as Alternate Function 1 (USART2_TX)
    Config_Pin_AF(GPIOA, 2, 1);
    
    // Q3: PC12 as Alternate Function 2 (TIM14_CH1)
    // AF2 maps the hardware timer directly to the pin.
    Config_Pin_AF(GPIOC, 12, 2);

    // ==========================================
    // 4. USART2 Setup (Q2)
    // ==========================================
    // 230,400 baud rate, no interrupts
    Initialize_Serial(USART2, 230400, 0);
    VT100_Clear(USART2);

    // ==========================================
    // 5. TIM14 Setup for Output Compare (Q3)
    // ==========================================
    // We want a 1ms interval between toggles. 
    // PSC: 1us ticks -> (SystemCoreClock / 1000000) - 1
    // ARR: 1000 ticks -> 1ms period before reloading
    Setup_Tim_Base(TIM14, (SystemCoreClock / 1000000) - 1, 1000 - 1);
    
    // Set CCR1 to 0 so the compare matches exactly when the timer resets
    Set_Tim_Compare(TIM14, TimCCR1, 0);
    
    // Configure CH1 for Output Compare Toggle mode
    Config_Tim_Chan(TIM14, TimCCR1, OutputCompareToggle);
    
    // Start TIM14 (It will now run independently in the background)
    StartStop_Tim(TIM14, 1);

    // ==========================================
    // 6. TIM6 Setup for Polling (Q1 & Q2)
    // ==========================================
    // PSC: 1ms ticks, ARR: 100 ticks (100ms)
    Setup_Tim_Base(TIM6, (SystemCoreClock / 1000) - 1, 100 - 1);
    StartStop_Tim(TIM6, 1);

    // ==========================================
    // 7. Main Loop
    // ==========================================
    while(1) {
        // Poll the Update Interrupt Flag (UIF) every 100ms (Q1 & Q2)
        if (Check_Tim_Update(TIM6)) {
            
            // --- Q1 Action: Toggle LED4 ---
            Pin_Tgl(GPIOA, 5);
            
            // --- Q2 Action: Format and send the counter ---
            // %05lu formats the unsigned long to 5 digits, padded with leading zeros
            sprintf(buffer, "%05lu", counter);
            
            // Print the buffer at Col (X) = 20, Row (Y) = 10
            VT100_PrintAt(USART2, 20, 10, buffer);
            
            // Increment the counter for the next event
            counter++;
        }
        
        // Note: Q3 requires NO code inside the while loop!
        // TIM14 handles the 1ms PC12 toggle entirely in hardware via Output Compare.
    }
}