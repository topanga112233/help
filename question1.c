#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"

// Note: Ensure system_stm32g0xx.c is in your project so SystemCoreClock is defined.

int main(void) {
    // 1. Initialize the System Clock to a known speed (e.g., 64MHz)
    Init_SysClock_PLL(PLL_64MHZ);

    // 2. Configure PA5 (LED4) as an output
    Enable_Port_Clock(GPIOA);
    Set_Pin_Out(GPIOA, 5);

    // 3. Enable the peripheral clock for TIM6 (Standard on APB1 bus)
    RCC->APBENR1 |= RCC_APBENR1_TIM6EN;

    // 4. Configure TIM6 for a 100ms period
    // We use Setup_Tim_Base to set the Prescaler (PSC) and Auto-Reload Register (ARR).
    // PSC: (SystemCoreClock / 1000) - 1 gives us 1ms ticks.
    // ARR: 100 - 1 gives us exactly 100 ticks before the timer rolls over.
    Setup_Tim_Base(TIM6, (SystemCoreClock / 1000) - 1, 100 - 1);

    // 5. Start the timer
    StartStop_Tim(TIM6, 1);

    while(1) {
        // 6. Poll the Update Interrupt Flag (UIF)
        if (Check_Tim_Update(TIM6)) {
            // Toggle the LED. 
            // Scoping PA5 will show a high state for 100ms and a low state for 100ms.
            Pin_Tgl(GPIOA, 5);
        }
    }
}