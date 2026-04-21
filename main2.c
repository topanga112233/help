#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include <stdio.h>

// --- Global State Variables ---
uint8_t duty_cycle = 25;                       // Initial duty cycle: 25%
ChannelMode_Typedef current_pwm_mode = Pwm1;   // Initial mode: PWM1

// Button state variables for edge-detection (acting only on transition)
uint8_t prev_sw_blue = 1;
uint8_t prev_sw1 = 1;
uint8_t prev_sw2 = 1;

// Helper to update the TIM16 PWM and redraw the UART terminal
void update_pwm_and_display(void) {
    // Update TIM16 PWM duty cycle 
    // (ARR is 199, so period is 200 ticks. 1% duty = 2 ticks)
    Timer_WriteCCR(TIM16, TimCCR1, duty_cycle * 2);
    
    // Update UART Terminal at (10, 10)
    char buf[32];
    sprintf(buf, "DUTY: %d%%  ", duty_cycle); // Extra spaces clear leftover characters
    TERM_TxStringXY(USART2, 10, 10, buf);
}

int main(void) {
    // ==========================================
    // PREPARATION: Clock & Peripheral Clocks
    // ==========================================
    // Configure system clock to 50MHz using PLL
    Clock_InitPll(PLL_50MHZ);
    
    // Enable clocks for GPIOA, GPIOB, GPIOC, GPIOD, TIM15, TIM16, and USART2
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN | RCC_IOPENR_GPIODEN;
    RCC->APBENR2 |= RCC_APBENR2_TIM15EN | RCC_APBENR2_TIM16EN;
    RCC->APBENR1 |= RCC_APBENR1_USART2EN;

    // ==========================================
    // GPIO PIN CONFIGURATIONS
    // ==========================================
    // Q1: TIM15_CH1 (PB14) and TIM15_CH2 (PB15) -> AF5
    GPIO_InitAlternateF(GPIOB, (1 << 14), 5);
    GPIO_InitAlternateF(GPIOB, (1 << 15), 5);
    
    // Q2: TIM16_CH1 (PA6) -> AF5
    GPIO_InitAlternateF(GPIOA, (1 << 6), 5);
    
    // Q5: USART2 TX (PA2) -> AF1 (RX not strictly needed for transmitting but good practice)
    GPIO_InitAlternateF(GPIOA, (1 << 2), 1);
    
    // Q3 & Q4: Button Inputs (PC13, PD8, PD9)
    GPIO_InitInput(GPIOC, (1 << 13));
    GPIO_SetPullMode(GPIOC, (1 << 13), PullMode_PullUp);
    
    GPIO_InitInput(GPIOD, (1 << 8));
    GPIO_SetPullMode(GPIOD, (1 << 8), PullMode_PullUp);
    
    GPIO_InitInput(GPIOD, (1 << 9));
    GPIO_SetPullMode(GPIOD, (1 << 9), PullMode_PullUp);

    // ==========================================
    // QUESTION 1: TIM15 Output Compare Toggle
    // ==========================================
    // 50MHz Clock. PSC = 49 yields a 1us timer tick. Free-running to 0xFFFF.
    Timer_Init(TIM15, 49, 0xFFFF);
    Timer_SetupChannel(TIM15, TimCCR1, OutputCompareToggle);
    Timer_SetupChannel(TIM15, TimCCR2, OutputCompareToggle);
    
    // Initial toggle limits (250us and 400us pulses = 250 and 400 tick intervals)
    Timer_WriteCCR(TIM15, TimCCR1, 250);
    Timer_WriteCCR(TIM15, TimCCR2, 400);
    
    // Enable Interrupts for CC1 and CC2
    Timer_EnableInterrupt(TIM15, TIM15_IRQn, TimCC1IE | TimCC2IE);
    Timer_SetEnable(TIM15, 1);

    // ==========================================
    // QUESTION 2: TIM16 PWM 5kHz, 25% Duty
    // ==========================================
    // 50MHz Clock. PSC = 49 yields 1us tick. 5kHz = 200us period -> ARR = 199.
    Timer_Init(TIM16, 49, 199);
    Timer_SetupChannel(TIM16, TimCCR1, current_pwm_mode);
    Timer_WriteCCR(TIM16, TimCCR1, 50); // 25% of 200 = 50
    Timer_SetEnable(TIM16, 1);

    // ==========================================
    // QUESTION 5: UART2 Initialization
    // ==========================================
    UART_Init(USART2, 115200, 0);
    TERM_ClearScreen(USART2);
    update_pwm_and_display();

    // ==========================================
    // MAIN LOOP (Polling and Debouncing)
    // ==========================================
    while (1) {
        // Q3: Blue Button (PC13) - Toggle PWM Mode
        uint8_t sw_blue = GPIO_Read(GPIOC, (1 << 13));
        if (sw_blue == 0 && prev_sw_blue == 1) { // Falling edge transition
            current_pwm_mode = (current_pwm_mode == Pwm1) ? Pwm2 : Pwm1;
            Timer_SetupChannel(TIM16, TimCCR1, current_pwm_mode);
        }
        prev_sw_blue = sw_blue;

        // Q4: SW1 (PD8) - Decrease Duty Cycle by 5%
        uint8_t sw1 = GPIO_Read(GPIOD, (1 << 8));
        if (sw1 == 0 && prev_sw1 == 1) { // Falling edge transition
            if (duty_cycle > 5) { // Enforce 5% minimum
                duty_cycle -= 5;
            }
            update_pwm_and_display();
        }
        prev_sw1 = sw1;

        // Q4: SW2 (PD9) - Increase Duty Cycle by 5%
        uint8_t sw2 = GPIO_Read(GPIOD, (1 << 9));
        if (sw2 == 0 && prev_sw2 == 1) { // Falling edge transition
            if (duty_cycle < 95) { // Enforce 95% maximum
                duty_cycle += 5;
            }
            update_pwm_and_display();
        }
        prev_sw2 = sw2;
        
        // Software delay for basic switch debouncing
        for(volatile int i = 0; i < 20000; i++);
    }
}

// ==========================================
// QUESTION 1: TIM15 Interrupt Handler
// ==========================================
void TIM15_IRQHandler(void) {
    // If Channel 1 Compare match fired
    if (TIM15->SR & TIM_SR_CC1IF) {
        TIM15->SR &= ~TIM_SR_CC1IF; // Clear flag
        // Schedule next toggle in 250us
        TIM15->CCR1 += 250;         
    }
    // If Channel 2 Compare match fired
    if (TIM15->SR & TIM_SR_CC2IF) {
        TIM15->SR &= ~TIM_SR_CC2IF; // Clear flag
        // Schedule next toggle in 400us
        TIM15->CCR2 += 400;         
    }
}