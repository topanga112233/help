#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include "delay.h" 
#include <stdio.h> 

int main(void) {
    // --- Q1 & Q2 Variables ---
    uint32_t sys_counter = 0;
    char sys_buffer[20]; 
    
    // --- Q4 Variables ---
    uint8_t multiplier = 1;      
    int current_btn_blue = 1;         
    int prev_btn_blue = 1;

    // --- Q6 Variables ---
    uint8_t duty_cycle_percent = 75; 

    // --- Q7 Variables ---
    int16_t seg_counter = 0; // Using signed int to easily catch < 0 for wrap-around
    char seg_buffer[10];
    int current_sw4 = 1, prev_sw4 = 1;
    int current_sw1 = 1, prev_sw1 = 1; // Assuming SW1 is Active Low

    // ==========================================
    // 1. Initialize the System Clock (64MHz)
    // ==========================================
    Init_SysClock_PLL(PLL_64MHZ);

    // ==========================================
    // 2. Enable Peripheral Clocks
    // ==========================================
    Enable_Port_Clock(GPIOA);
    Enable_Port_Clock(GPIOC);
    Enable_Port_Clock(GPIOD);              // Q7: Clocks for SW1 & SW4
    
    RCC->APBENR1 |= RCC_APBENR1_TIM6EN;    // Q1, Q2, Q6 (TIM6)
    RCC->APBENR2 |= RCC_APBENR2_TIM14EN;   // Q3, Q4 (TIM14)
    RCC->APBENR2 |= RCC_APBENR2_TIM16EN;   // Q5, Q6 (TIM16)
    
    Enable_Serial_Clock(USART1);           // Q7 (USART1)
    Enable_Serial_Clock(USART2);           // Q2 (USART2)

    // ==========================================
    // 3. GPIO Configuration
    // ==========================================
    Set_Pin_Out(GPIOA, 5);          // Q1: PA5 as Output (LED4)
    Config_Pin_AF(GPIOA, 2, 1);     // Q2: PA2 as AF1 (USART2_TX)
    Config_Pin_AF(GPIOC, 12, 2);    // Q3: PC12 as AF2 (TIM14_CH1)
    Set_Pin_In(GPIOC, 13);          // Q4: PC13 as Input (Blue Button)
    Config_Pin_AF(GPIOA, 6, 5);     // Q5: PA6 as AF5 (TIM16_CH1)
    
    // Q7: USART1 & Switches
    Config_Pin_AF(GPIOC, 4, 1);     // PC4 as AF1 (USART1_TX)
    
    Set_Pin_In(GPIOD, 9);           // PD9 as Input (SW4)
    Set_Pin_Pull(GPIOD, 9, PullMode_PullUp); // Internal pull-up to ensure stable high
    
    Set_Pin_In(GPIOD, 8);           // PD8 as Input (Assumed SW1)
    Set_Pin_Pull(GPIOD, 8, PullMode_PullUp); 

    // ==========================================
    // 4. USART Setup (Q2 & Q7)
    // ==========================================
    Initialize_Serial(USART2, 230400, 0); // Q2 Termianl
    VT100_Clear(USART2);

    // Assuming standard 9600 baud for a generic serial 7-segment display
    Initialize_Serial(USART1, 9600, 0);   
    
    // Q7: Display "0000" at the beginning of the program
    sprintf(seg_buffer, "%04d", seg_counter);
    Serial_SendText(USART1, seg_buffer);

    // ==========================================
    // 5. TIM16 Setup for PWM (Q5 & Q6)
    // ==========================================
    Setup_Tim_Base(TIM16, (SystemCoreClock / 1000000) - 1, 1000 - 1);
    Set_Tim_Compare(TIM16, TimCCR1, duty_cycle_percent * 10);
    Config_Tim_Chan(TIM16, TimCCR1, Pwm1);
    StartStop_Tim(TIM16, 1);

    // ==========================================
    // 6. TIM14 Setup for Output Compare (Q3 & Q4)
    // ==========================================
    Setup_Tim_Base(TIM14, (SystemCoreClock / 1000000) - 1, 1000 - 1);
    Set_Tim_Compare(TIM14, TimCCR1, 0);
    Config_Tim_Chan(TIM14, TimCCR1, OutputCompareToggle);
    StartStop_Tim(TIM14, 1);

    // ==========================================
    // 7. TIM6 Setup for Polling (Q1, Q2 & Q6)
    // ==========================================
    Setup_Tim_Base(TIM6, (SystemCoreClock / 1000) - 1, 100 - 1);
    StartStop_Tim(TIM6, 1);

    // ==========================================
    // 8. Main Loop
    // ==========================================
    while(1) {
        
        // -----------------------------------------------------
        // Q4 Action: Blue Button Transition Detection
        // -----------------------------------------------------
        current_btn_blue = Pin_Get(GPIOC, 13);
        if (current_btn_blue == 0 && prev_btn_blue == 1) { 
            Wait_Millis(20); 
            if (Pin_Get(GPIOC, 13) == 0) { 
                multiplier++;
                if (multiplier > 5) multiplier = 1;
                TIM14->ARR = (multiplier * 1000) - 1;
            }
        }
        prev_btn_blue = current_btn_blue;


        // -----------------------------------------------------
        // Q7 Action: 7-Segment Increment (SW4 on PD9)
        // -----------------------------------------------------
        current_sw4 = Pin_Get(GPIOD, 9);
        if (current_sw4 == 0 && prev_sw4 == 1) { // Edge detection
            Wait_Millis(20); // Debounce
            if (Pin_Get(GPIOD, 9) == 0) { 
                seg_counter++;
                if (seg_counter > 9999) {
                    seg_counter = 0; // Wrap around to 0
                }
                // Send updated value to display
                sprintf(seg_buffer, "%04d", seg_counter);
                Serial_SendText(USART1, seg_buffer);
            }
        }
        prev_sw4 = current_sw4;


        // -----------------------------------------------------
        // Q7 Action: 7-Segment Decrement (SW1 on Assumed PD8)
        // -----------------------------------------------------
        current_sw1 = Pin_Get(GPIOD, 8);
        if (current_sw1 == 0 && prev_sw1 == 1) { // Edge detection
            Wait_Millis(20); // Debounce
            if (Pin_Get(GPIOD, 8) == 0) { 
                seg_counter--;
                if (seg_counter < 0) {
                    seg_counter = 9999; // Wrap around to max
                }
                // Send updated value to display
                sprintf(seg_buffer, "%04d", seg_counter);
                Serial_SendText(USART1, seg_buffer);
            }
        }
        prev_sw1 = current_sw1;


        // -----------------------------------------------------
        // Q1, Q2 & Q6 Action: 100ms Polling
        // -----------------------------------------------------
        if (Check_Tim_Update(TIM6)) {
            
            // --- Q1: Toggle LED4 ---
            Pin_Tgl(GPIOA, 5);
            
            // --- Q2: Send the padded counter via USART2 ---
            sprintf(sys_buffer, "%05lu", sys_counter);
            VT100_PrintAt(USART2, 20, 10, sys_buffer);
            sys_counter++;
            
            // --- Q6: Adjust PWM Duty Cycle dynamically ---
            duty_cycle_percent++;
            if (duty_cycle_percent > 99) {
                duty_cycle_percent = 1; 
            }
            Set_Tim_Compare(TIM16, TimCCR1, duty_cycle_percent * 10);
        }
    }
}