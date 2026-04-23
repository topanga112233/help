#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include "delay.h" 
#include <stdio.h> 

int main(void) {
    // --- Q1 & Q2 Variables ---
    uint32_t counter = 0;
    char buffer[20]; 
    
    // --- Q4 Variables ---
    uint8_t multiplier = 1;      // Starts at 1x
    int current_btn = 1;         // Assuming button is Active Low (default 1)
    int prev_btn = 1;

    // --- Q6 Variables ---
    uint8_t duty_cycle_percent = 75; // Starts at 75% to satisfy Q5 initially

    // ==========================================
    // 1. Initialize the System Clock (64MHz)
    // ==========================================
    Init_SysClock_PLL(PLL_64MHZ);

    // ==========================================
    // 2. Enable Peripheral Clocks
    // ==========================================
    Enable_Port_Clock(GPIOA);
    Enable_Port_Clock(GPIOC);
    RCC->APBENR1 |= RCC_APBENR1_TIM6EN;    // Q1 & Q2 (TIM6)
    RCC->APBENR2 |= RCC_APBENR2_TIM14EN;   // Q3 & Q4 (TIM14)
    RCC->APBENR2 |= RCC_APBENR2_TIM16EN;   // Q5 & Q6 (TIM16)
    Enable_Serial_Clock(USART2);           // Q2 (USART2)

    // ==========================================
    // 3. GPIO Configuration
    // ==========================================
    Set_Pin_Out(GPIOA, 5);          // Q1: PA5 as Output (LED4)
    Config_Pin_AF(GPIOA, 2, 1);     // Q2: PA2 as AF1 (USART2_TX)
    Config_Pin_AF(GPIOC, 12, 2);    // Q3: PC12 as AF2 (TIM14_CH1)
    Set_Pin_In(GPIOC, 13);          // Q4: PC13 as Input (Blue Button)
    Config_Pin_AF(GPIOA, 6, 5);     // Q5: PA6 as AF5 (TIM16_CH1)

    // ==========================================
    // 4. USART2 Setup (Q2)
    // ==========================================
    Initialize_Serial(USART2, 230400, 0);
    VT100_Clear(USART2);

    // ==========================================
    // 5. TIM16 Setup for PWM (Q5 & Q6)
    // ==========================================
    // 1 kHz Frequency = 1ms Period.
    // PSC: 1us ticks -> (SystemCoreClock / 1000000) - 1
    // ARR: 1000 ticks -> 1ms period
    Setup_Tim_Base(TIM16, (SystemCoreClock / 1000000) - 1, 1000 - 1);
    
    // Initial Duty Cycle set by variable (75% = 750 ticks)
    Set_Tim_Compare(TIM16, TimCCR1, duty_cycle_percent * 10);
    
    // Configure CH1 for PWM Mode 1
    Config_Tim_Chan(TIM16, TimCCR1, Pwm1);
    
    // Start TIM16
    StartStop_Tim(TIM16, 1);

    // ==========================================
    // 6. TIM14 Setup for Output Compare (Q3 & Q4)
    // ==========================================
    // Base setup: 1us ticks, 1ms interval (1000 ticks)
    Setup_Tim_Base(TIM14, (SystemCoreClock / 1000000) - 1, 1000 - 1);
    Set_Tim_Compare(TIM14, TimCCR1, 0);
    Config_Tim_Chan(TIM14, TimCCR1, OutputCompareToggle);
    StartStop_Tim(TIM14, 1);

    // ==========================================
    // 7. TIM6 Setup for Polling (Q1 & Q2)
    // ==========================================
    // Base setup: 1ms ticks, 100ms interval (100 ticks)
    Setup_Tim_Base(TIM6, (SystemCoreClock / 1000) - 1, 100 - 1);
    StartStop_Tim(TIM6, 1);

    // ==========================================
    // 8. Main Loop
    // ==========================================
    while(1) {
        
        // -----------------------------------------------------
        // Q4 Action: Button Transition Detection
        // -----------------------------------------------------
        current_btn = Pin_Get(GPIOC, 13);
        
        if (current_btn == 0 && prev_btn == 1) { 
            Wait_Millis(20); // Debounce
            
            if (Pin_Get(GPIOC, 13) == 0) { 
                multiplier++;
                if (multiplier > 5) {
                    multiplier = 1;
                }
                // Update TIM14 ARR dynamically
                TIM14->ARR = (multiplier * 1000) - 1;
            }
        }
        prev_btn = current_btn;


        // -----------------------------------------------------
        // Q1, Q2 & Q6 Action: 100ms Polling
        // -----------------------------------------------------
        if (Check_Tim_Update(TIM6)) {
            
            // --- Q1: Toggle LED4 ---
            Pin_Tgl(GPIOA, 5);
            
            // --- Q2: Send the padded counter ---
            sprintf(buffer, "%05lu", counter);
            VT100_PrintAt(USART2, 20, 10, buffer);
            counter++;
            
            // --- Q6: Adjust PWM Duty Cycle dynamically ---
            duty_cycle_percent++;
            
            if (duty_cycle_percent > 99) {
                duty_cycle_percent = 1; // Roll back to 1%
            }
            
            // Multiply by 10 because our total ARR period is 1000 ticks
            Set_Tim_Compare(TIM16, TimCCR1, duty_cycle_percent * 10);
        }
    }
}