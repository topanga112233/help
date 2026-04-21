That is a very smart strategy for a practical exam. Getting those incremental check-offs guarantees you secure points as you build up the complexity, rather than risking a broken final build. 

Here is the exact code you need for each step. For each part, you can completely overwrite your `main.c` with the provided block, compile, and call your instructor over. 

---

### Part 1 Check-off: Preparation & Question 1
**Goal:** 50MHz Clock + TIM15 Output Compare Toggle on PB14 (250µs) and PB15 (400µs).
*Show this on your oscilloscope.*

```c
#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"

int main(void) {
    // PREPARATION: Clock & Peripheral Clocks
    Clock_InitPll(PLL_50MHZ);
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    RCC->APBENR2 |= RCC_APBENR2_TIM15EN;

    // QUESTION 1: GPIO CONFIGURATIONS
    // TIM15_CH1 (PB14) and TIM15_CH2 (PB15) -> AF5
    GPIO_InitAlternateF(GPIOB, (1 << 14), 5);
    GPIO_InitAlternateF(GPIOB, (1 << 15), 5);
    
    // QUESTION 1: TIM15 Output Compare Toggle
    // 50MHz Clock. PSC = 49 yields a 1us timer tick. Free-running to 0xFFFF.
    Timer_Init(TIM15, 49, 0xFFFF);
    Timer_SetupChannel(TIM15, TimCCR1, OutputCompareToggle);
    Timer_SetupChannel(TIM15, TimCCR2, OutputCompareToggle);
    
    // Initial toggle limits
    Timer_WriteCCR(TIM15, TimCCR1, 250);
    Timer_WriteCCR(TIM15, TimCCR2, 400);
    
    // Enable Interrupts for CC1 and CC2
    Timer_EnableInterrupt(TIM15, TIM15_IRQn, TimCC1IE | TimCC2IE);
    Timer_SetEnable(TIM15, 1);

    while (1) {
        // Main loop empty for Part 1
    }
}

// TIM15 Interrupt Handler for Question 1
void TIM15_IRQHandler(void) {
    if (TIM15->SR & TIM_SR_CC1IF) {
        TIM15->SR &= ~TIM_SR_CC1IF; 
        TIM15->CCR1 += 250;         // Schedule next toggle in 250us
    }
    if (TIM15->SR & TIM_SR_CC2IF) {
        TIM15->SR &= ~TIM_SR_CC2IF; 
        TIM15->CCR2 += 400;         // Schedule next toggle in 400us
    }
}
```

---

### Part 2 Check-off: Adding Question 2
**Goal:** Keep Q1 working, add TIM16 PWM on PA6 (5kHz, 25% duty).
*Keep PB14/PB15 on the scope, add PA6 to another channel to show the 200µs period (5kHz).*

```c
#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"

int main(void) {
    // PREPARATION: Clocks
    Clock_InitPll(PLL_50MHZ);
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN;
    RCC->APBENR2 |= RCC_APBENR2_TIM15EN | RCC_APBENR2_TIM16EN;

    // Q1: GPIO
    GPIO_InitAlternateF(GPIOB, (1 << 14), 5);
    GPIO_InitAlternateF(GPIOB, (1 << 15), 5);
    
    // Q2: GPIO TIM16_CH1 (PA6) -> AF5
    GPIO_InitAlternateF(GPIOA, (1 << 6), 5);
    
    // Q1: TIM15 Config
    Timer_Init(TIM15, 49, 0xFFFF);
    Timer_SetupChannel(TIM15, TimCCR1, OutputCompareToggle);
    Timer_SetupChannel(TIM15, TimCCR2, OutputCompareToggle);
    Timer_WriteCCR(TIM15, TimCCR1, 250);
    Timer_WriteCCR(TIM15, TimCCR2, 400);
    Timer_EnableInterrupt(TIM15, TIM15_IRQn, TimCC1IE | TimCC2IE);
    Timer_SetEnable(TIM15, 1);

    // Q2: TIM16 PWM 5kHz, 25% Duty
    // 50MHz Clock. PSC = 49 yields 1us tick. 5kHz = 200us period -> ARR = 199.
    Timer_Init(TIM16, 49, 199);
    Timer_SetupChannel(TIM16, TimCCR1, Pwm1);
    Timer_WriteCCR(TIM16, TimCCR1, 50); // 25% of 200 = 50
    Timer_SetEnable(TIM16, 1);

    while (1) {
    }
}

// Q1 IRQ Handler
void TIM15_IRQHandler(void) {
    if (TIM15->SR & TIM_SR_CC1IF) {
        TIM15->SR &= ~TIM_SR_CC1IF; 
        TIM15->CCR1 += 250;         
    }
    if (TIM15->SR & TIM_SR_CC2IF) {
        TIM15->SR &= ~TIM_SR_CC2IF; 
        TIM15->CCR2 += 400;         
    }
}
```

---

### Part 3 Check-off: Adding Question 3
**Goal:** Pressing the Blue Button (PC13) exactly once toggles PA6 between PWM1 and PWM2.
*Show the instructor the oscilloscope inverting the duty cycle every time you press the button.*

```c
#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"

// Q3 Global State
ChannelMode_Typedef current_pwm_mode = Pwm1;
uint8_t prev_sw_blue = 1;

int main(void) {
    // PREPARATION: Clocks
    Clock_InitPll(PLL_50MHZ);
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN;
    RCC->APBENR2 |= RCC_APBENR2_TIM15EN | RCC_APBENR2_TIM16EN;

    // Q1 & Q2: GPIO
    GPIO_InitAlternateF(GPIOB, (1 << 14), 5);
    GPIO_InitAlternateF(GPIOB, (1 << 15), 5);
    GPIO_InitAlternateF(GPIOA, (1 << 6), 5);
    
    // Q3: Blue Button (PC13)
    GPIO_InitInput(GPIOC, (1 << 13));
    GPIO_SetPullMode(GPIOC, (1 << 13), PullMode_PullUp);
    
    // Q1: TIM15 Config
    Timer_Init(TIM15, 49, 0xFFFF);
    Timer_SetupChannel(TIM15, TimCCR1, OutputCompareToggle);
    Timer_SetupChannel(TIM15, TimCCR2, OutputCompareToggle);
    Timer_WriteCCR(TIM15, TimCCR1, 250);
    Timer_WriteCCR(TIM15, TimCCR2, 400);
    Timer_EnableInterrupt(TIM15, TIM15_IRQn, TimCC1IE | TimCC2IE);
    Timer_SetEnable(TIM15, 1);

    // Q2: TIM16 Config
    Timer_Init(TIM16, 49, 199);
    Timer_SetupChannel(TIM16, TimCCR1, current_pwm_mode);
    Timer_WriteCCR(TIM16, TimCCR1, 50); 
    Timer_SetEnable(TIM16, 1);

    while (1) {
        // Q3: Toggle PWM Mode on transition
        uint8_t sw_blue = GPIO_Read(GPIOC, (1 << 13));
        if (sw_blue == 0 && prev_sw_blue == 1) { 
            current_pwm_mode = (current_pwm_mode == Pwm1) ? Pwm2 : Pwm1;
            Timer_SetupChannel(TIM16, TimCCR1, current_pwm_mode);
        }
        prev_sw_blue = sw_blue;
        
        for(volatile int i = 0; i < 20000; i++); // basic debounce
    }
}

void TIM15_IRQHandler(void) {
    if (TIM15->SR & TIM_SR_CC1IF) {
        TIM15->SR &= ~TIM_SR_CC1IF; TIM15->CCR1 += 250;         
    }
    if (TIM15->SR & TIM_SR_CC2IF) {
        TIM15->SR &= ~TIM_SR_CC2IF; TIM15->CCR2 += 400;         
    }
}
```

---

### Part 4 Check-off: Adding Question 4
**Goal:** Use SW1 (PD8) to decrease and SW2 (PD9) to increase duty cycle by 5% (capped 5-95%).
*Show the instructor the pulse width changing on the scope when you tap the switches.*

```c
#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"

// Q3 & Q4 Global State
ChannelMode_Typedef current_pwm_mode = Pwm1;
uint8_t duty_cycle = 25;
uint8_t prev_sw_blue = 1;
uint8_t prev_sw1 = 1;
uint8_t prev_sw2 = 1;

int main(void) {
    // Clocks
    Clock_InitPll(PLL_50MHZ);
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN | RCC_IOPENR_GPIODEN;
    RCC->APBENR2 |= RCC_APBENR2_TIM15EN | RCC_APBENR2_TIM16EN;

    // GPIOs
    GPIO_InitAlternateF(GPIOB, (1 << 14), 5); // Q1
    GPIO_InitAlternateF(GPIOB, (1 << 15), 5); // Q1
    GPIO_InitAlternateF(GPIOA, (1 << 6), 5);  // Q2
    
    // Buttons Q3 & Q4
    GPIO_InitInput(GPIOC, (1 << 13));
    GPIO_SetPullMode(GPIOC, (1 << 13), PullMode_PullUp);
    GPIO_InitInput(GPIOD, (1 << 8));
    GPIO_SetPullMode(GPIOD, (1 << 8), PullMode_PullUp);
    GPIO_InitInput(GPIOD, (1 << 9));
    GPIO_SetPullMode(GPIOD, (1 << 9), PullMode_PullUp);
    
    // Q1: TIM15 Config
    Timer_Init(TIM15, 49, 0xFFFF);
    Timer_SetupChannel(TIM15, TimCCR1, OutputCompareToggle);
    Timer_SetupChannel(TIM15, TimCCR2, OutputCompareToggle);
    Timer_WriteCCR(TIM15, TimCCR1, 250);
    Timer_WriteCCR(TIM15, TimCCR2, 400);
    Timer_EnableInterrupt(TIM15, TIM15_IRQn, TimCC1IE | TimCC2IE);
    Timer_SetEnable(TIM15, 1);

    // Q2: TIM16 Config
    Timer_Init(TIM16, 49, 199);
    Timer_SetupChannel(TIM16, TimCCR1, current_pwm_mode);
    Timer_WriteCCR(TIM16, TimCCR1, duty_cycle * 2); 
    Timer_SetEnable(TIM16, 1);

    while (1) {
        // Q3: Toggle PWM Mode
        uint8_t sw_blue = GPIO_Read(GPIOC, (1 << 13));
        if (sw_blue == 0 && prev_sw_blue == 1) { 
            current_pwm_mode = (current_pwm_mode == Pwm1) ? Pwm2 : Pwm1;
            Timer_SetupChannel(TIM16, TimCCR1, current_pwm_mode);
        }
        prev_sw_blue = sw_blue;

        // Q4: Decrease Duty Cycle (SW1 - PD8)
        uint8_t sw1 = GPIO_Read(GPIOD, (1 << 8));
        if (sw1 == 0 && prev_sw1 == 1) { 
            if (duty_cycle > 5) duty_cycle -= 5;
            Timer_WriteCCR(TIM16, TimCCR1, duty_cycle * 2);
        }
        prev_sw1 = sw1;

        // Q4: Increase Duty Cycle (SW2 - PD9)
        uint8_t sw2 = GPIO_Read(GPIOD, (1 << 9));
        if (sw2 == 0 && prev_sw2 == 1) { 
            if (duty_cycle < 95) duty_cycle += 5;
            Timer_WriteCCR(TIM16, TimCCR1, duty_cycle * 2);
        }
        prev_sw2 = sw2;
        
        for(volatile int i = 0; i < 20000; i++); // basic debounce
    }
}

void TIM15_IRQHandler(void) {
    if (TIM15->SR & TIM_SR_CC1IF) {
        TIM15->SR &= ~TIM_SR_CC1IF; TIM15->CCR1 += 250;         
    }
    if (TIM15->SR & TIM_SR_CC2IF) {
        TIM15->SR &= ~TIM_SR_CC2IF; TIM15->CCR2 += 400;         
    }
}
```

---

### Part 5 Check-off: The Final Exam Code 
**Goal:** Add USART2 at 115200 baud to output the duty cycle using the `<stdio.h>` `sprintf()` function and your terminal library whenever a button is pressed.
*Show the instructor the duty cycle visually updating on Tera Term.*

```c
#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include <stdio.h>

// Global State
uint8_t duty_cycle = 25;
ChannelMode_Typedef current_pwm_mode = Pwm1;
uint8_t prev_sw_blue = 1;
uint8_t prev_sw1 = 1;
uint8_t prev_sw2 = 1;

// Q5 Helper: Updates TIM16 and re-draws Tera Term
void update_pwm_and_display(void) {
    Timer_WriteCCR(TIM16, TimCCR1, duty_cycle * 2);
    char buf[32];
    sprintf(buf, "DUTY: %d%%  ", duty_cycle); 
    TERM_TxStringXY(USART2, 10, 10, buf);
}

int main(void) {
    // PREPARATION Clocks
    Clock_InitPll(PLL_50MHZ);
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN | RCC_IOPENR_GPIODEN;
    RCC->APBENR2 |= RCC_APBENR2_TIM15EN | RCC_APBENR2_TIM16EN;
    RCC->APBENR1 |= RCC_APBENR1_USART2EN;

    // GPIOs
    GPIO_InitAlternateF(GPIOB, (1 << 14), 5); // Q1
    GPIO_InitAlternateF(GPIOB, (1 << 15), 5); // Q1
    GPIO_InitAlternateF(GPIOA, (1 << 6), 5);  // Q2
    GPIO_InitAlternateF(GPIOA, (1 << 2), 1);  // Q5: USART2 TX
    
    // Buttons Q3 & Q4
    GPIO_InitInput(GPIOC, (1 << 13));
    GPIO_SetPullMode(GPIOC, (1 << 13), PullMode_PullUp);
    GPIO_InitInput(GPIOD, (1 << 8));
    GPIO_SetPullMode(GPIOD, (1 << 8), PullMode_PullUp);
    GPIO_InitInput(GPIOD, (1 << 9));
    GPIO_SetPullMode(GPIOD, (1 << 9), PullMode_PullUp);

    // Q1: TIM15 Config
    Timer_Init(TIM15, 49, 0xFFFF);
    Timer_SetupChannel(TIM15, TimCCR1, OutputCompareToggle);
    Timer_SetupChannel(TIM15, TimCCR2, OutputCompareToggle);
    Timer_WriteCCR(TIM15, TimCCR1, 250);
    Timer_WriteCCR(TIM15, TimCCR2, 400);
    Timer_EnableInterrupt(TIM15, TIM15_IRQn, TimCC1IE | TimCC2IE);
    Timer_SetEnable(TIM15, 1);

    // Q2: TIM16 Config
    Timer_Init(TIM16, 49, 199);
    Timer_SetupChannel(TIM16, TimCCR1, current_pwm_mode);
    Timer_SetEnable(TIM16, 1);

    // Q5: UART Config
    UART_Init(USART2, 115200, 0);
    TERM_ClearScreen(USART2);
    
    // Push initial values to both Timer and Screen
    update_pwm_and_display();

    while (1) {
        // Q3: Toggle PWM Mode
        uint8_t sw_blue = GPIO_Read(GPIOC, (1 << 13));
        if (sw_blue == 0 && prev_sw_blue == 1) { 
            current_pwm_mode = (current_pwm_mode == Pwm1) ? Pwm2 : Pwm1;
            Timer_SetupChannel(TIM16, TimCCR1, current_pwm_mode);
        }
        prev_sw_blue = sw_blue;

        // Q4 & Q5: Decrease Duty Cycle (SW1 - PD8)
        uint8_t sw1 = GPIO_Read(GPIOD, (1 << 8));
        if (sw1 == 0 && prev_sw1 == 1) { 
            if (duty_cycle > 5) {
                duty_cycle -= 5;
                update_pwm_and_display();
            }
        }
        prev_sw1 = sw1;

        // Q4 & Q5: Increase Duty Cycle (SW2 - PD9)
        uint8_t sw2 = GPIO_Read(GPIOD, (1 << 9));
        if (sw2 == 0 && prev_sw2 == 1) { 
            if (duty_cycle < 95) {
                duty_cycle += 5;
                update_pwm_and_display();
            }
        }
        prev_sw2 = sw2;
        
        for(volatile int i = 0; i < 20000; i++); // basic debounce
    }
}

void TIM15_IRQHandler(void) {
    if (TIM15->SR & TIM_SR_CC1IF) {
        TIM15->SR &= ~TIM_SR_CC1IF; TIM15->CCR1 += 250;         
    }
    if (TIM15->SR & TIM_SR_CC2IF) {
        TIM15->SR &= ~TIM_SR_CC2IF; TIM15->CCR2 += 400;         
    }
}
```