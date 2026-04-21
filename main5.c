I can solve the parts that are actually visible in your photos: **Preparation, Question 1, and Question 5**. I do **not** have the full text for Questions 2–4, so I cannot honestly give a true full-exam solution for those pages yet.

What I *can* do right now is give you the **correct combined code** for the visible parts, plus the **Q4 hook** that Q5 depends on.

Two important details from your libraries:

* `PLL_50MHZ` already exists in your clock library, so you should use `Clock_InitPll(PLL_50MHZ);`  
* Your `UART_Init()` hardcodes the peripheral clock as `16000000U`, so after switching the MCU to **50 MHz**, you must fix `USART2->BRR` manually for 115200 baud. Otherwise the terminal baud will be wrong.  

Also, for **Q1**, the correct way to get **250 µs** and **400 µs** toggle intervals on two channels of the same timer is to use **output compare toggle mode with compare interrupts**, then add the interval to each CCR inside the ISR. A fixed CCR value alone is not enough.

---

## `main.c`

```c
#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include <stdio.h>

volatile uint8_t gDuty = 50;
volatile uint8_t gDutyChanged = 1;

/* ---------------- Q1 ----------------
   TIM15 output compare toggle mode
   PB14 -> TIM15_CH1 (AF5)
   PB15 -> TIM15_CH2 (AF5)

   Desired positive high times:
   CH1 = 250 us
   CH2 = 400 us

   In toggle mode, each compare event flips the output.
   So we reschedule the next compare by adding:
   +250 for CH1
   +400 for CH2
-------------------------------------*/

static void Q1_TIM15_Toggle_Init(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    RCC->APBENR2 |= RCC_APBENR2_TIM15EN;

    GPIO_InitAlternateF(GPIOB, (1U << 14), 5U);   // PB14 AF5 = TIM15_CH1
    GPIO_InitAlternateF(GPIOB, (1U << 15), 5U);   // PB15 AF5 = TIM15_CH2

    /* 50 MHz / (49+1) = 1 MHz -> 1 tick = 1 us */
    Timer_Init(TIM15, 49U, 0xFFFFU);

    Timer_SetupChannel(TIM15, TimCCR1, OutputCompareToggle);
    Timer_SetupChannel(TIM15, TimCCR2, OutputCompareToggle);

    /* TIM15 is an advanced timer, so MOE must be enabled */
    TIM15->BDTR |= TIM_BDTR_MOE;

    /* First compare events */
    Timer_WriteCCR(TIM15, TimCCR1, 250U);
    Timer_WriteCCR(TIM15, TimCCR2, 400U);

    Timer_EnableInterrupt(TIM15, TIM15_IRQn, (Timer_IE)(TimCC1IE | TimCC2IE));

    Timer_SetEnable(TIM15, 1U);
}

void TIM15_IRQHandler(void)
{
    if ((TIM15->SR & TIM_SR_CC1IF) != 0U)
    {
        TIM15->SR &= ~TIM_SR_CC1IF;
        TIM15->CCR1 += 250U;   // next toggle after 250 us
    }

    if ((TIM15->SR & TIM_SR_CC2IF) != 0U)
    {
        TIM15->SR &= ~TIM_SR_CC2IF;
        TIM15->CCR2 += 400U;   // next toggle after 400 us
    }
}

/* ---------------- Buttons ----------------
   From preparation:
   S1 -> PD8
   S4 -> PD9
------------------------------------------*/

static void Buttons_Init(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIODEN;

    GPIO_InitInput(GPIOD, (1U << 8));
    GPIO_InitInput(GPIOD, (1U << 9));

    /* Typical button wiring on these boards uses pull-up */
    GPIO_SetPullMode(GPIOD, (1U << 8), PullMode_PullUp);
    GPIO_SetPullMode(GPIOD, (1U << 9), PullMode_PullUp);
}

/* ---------------- Q5 ----------------
   USART2 @ 115200
   Display duty at (10,10) using VT100
-------------------------------------*/

static void USART2_Init_115200_At50MHz(void)
{
    RCC->IOPENR  |= RCC_IOPENR_GPIOAEN;
    RCC->APBENR1 |= RCC_APBENR1_USART2EN;

    /* PA2 = USART2_TX, PA3 = USART2_RX, AF1 */
    GPIO_InitAlternateF(GPIOA, (1U << 2), 1U);
    GPIO_InitAlternateF(GPIOA, (1U << 3), 1U);

    UART_Init(USART2, 115200U, 0);

    /* IMPORTANT:
       your UART_Init() assumes 16 MHz peripheral clock.
       But the exam says system clock = 50 MHz.
       So fix BRR manually here. */
    USART2->BRR = 50000000U / 115200U;
}

static void Duty_Display_Update(void)
{
    char buf[32];

    snprintf(buf, sizeof(buf), "DUTY: %u%%   ", gDuty);
    TERM_TxStringXY(USART2, 10, 10, buf);
}

/* ------------- Q4 hook -------------
   I do not have the actual Q4 page.
   Based on Q5 and the faint reverse text, this looks like:
   - duty changes with S1 / S4
   - step = 5%
   - clamp between 5% and 95%

   If your Q4 PWM output uses another timer/channel,
   put the CCR update inside ApplyDutyToQ4Pwm().
------------------------------------*/

static void ApplyDutyToQ4Pwm(uint8_t dutyPercent)
{
    /* Replace this with your actual Q4 PWM timer/channel update.
       Example only:
       CCR = ((ARR + 1) * dutyPercent) / 100;
    */
    (void)dutyPercent;
}

int main(void)
{
    uint8_t prevS1 = 1U;
    uint8_t prevS4 = 1U;

    /* Preparation: clock = 50 MHz using PLL */
    Clock_InitPll(PLL_50MHZ);

    Buttons_Init();
    Q1_TIM15_Toggle_Init();
    USART2_Init_115200_At50MHz();

    TERM_ClearScreen(USART2);
    Duty_Display_Update();

    while (1)
    {
        uint8_t s1 = (uint8_t)GPIO_Read(GPIOD, (1U << 8));
        uint8_t s4 = (uint8_t)GPIO_Read(GPIOD, (1U << 9));

        /* Falling-edge detect for active-low buttons */
        if ((prevS1 == 1U) && (s1 == 0U))
        {
            if (gDuty < 95U)
            {
                gDuty += 5U;
                ApplyDutyToQ4Pwm(gDuty);
                gDutyChanged = 1U;
            }
        }

        if ((prevS4 == 1U) && (s4 == 0U))
        {
            if (gDuty > 5U)
            {
                gDuty -= 5U;
                ApplyDutyToQ4Pwm(gDuty);
                gDutyChanged = 1U;
            }
        }

        prevS1 = s1;
        prevS4 = s4;

        if (gDutyChanged != 0U)
        {
            Duty_Display_Update();
            gDutyChanged = 0U;
        }
    }
}
```

---

## Why this matches your libraries

Your GPIO library gives you:

* alternate-function pin setup
* input setup
* pull-up/pull-down control
* pin reads  

Your timer library gives you:

* timer init
* channel setup in toggle mode
* CCR writes
* interrupt enable
* timer enable  

Your UART library gives you:

* UART init
* terminal cursor positioning
* VT100 screen clear and XY text output  

Your clock library already supports the exact PLL target required by the exam: `PLL_50MHZ`.  

---

## What to say for Q1 check-off

For **PB14** and **PB15** on the scope:

* PB14 high time = **250 µs**
* PB15 high time = **400 µs**

Because each channel toggles on its own compare event, and the ISR schedules the next toggle by adding the required delay each time.

---

## One thing you must not forget

For **TIM15**, this line matters:

```c
TIM15->BDTR |= TIM_BDTR_MOE;
```

Without that, advanced-timer outputs may not appear on the pins.

---

## What is still missing

I still need the actual photos/pages for:

* Question 2
* Question 3
* Question 4

Right now I only inferred the button-duty logic from Q5 and the faint text showing through the page. Upload those pages and I’ll turn this into the actual complete exam solution.
