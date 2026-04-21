Below is a **single `main.c`** that covers the questions shown in your photos using **your uploaded libraries**:

* Q1: TIM15 output compare toggle on **PB14** and **PB15**
* Q2: TIM16 PWM on **PA6**, **5 kHz**, **25%**
* Q3: **PC13** toggles PWM mode between **PWM1** and **PWM2**
* Q4: **PD8** decreases and **PD9** increases duty by **5%**, capped **5% to 95%**
* Q5: **USART2 @ 115200** shows duty at **(10,10)** using your VT100 helper functions

This code uses your clock, GPIO, timer, and UART libraries directly. Your clock library already includes `PLL_50MHZ`, your GPIO library supports alternate-function config, your timer library supports toggle/PWM modes plus timer interrupts, and your UART library includes `TERM_GotoXY` / `TERM_TxStringXY`.    

One important fix is included: your `UART_Init()` assumes a **16 MHz** peripheral clock, so this code explicitly selects **HSI16** as the USART2 clock source to keep **115200 baud** correct while the MCU itself runs at **50 MHz**.  

```c
#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"

/* -------------------- Pin definitions -------------------- */
#define Q1_CH1_PIN          (1U << 14)   /* PB14 -> TIM15_CH1 AF5 */
#define Q1_CH2_PIN          (1U << 15)   /* PB15 -> TIM15_CH2 AF5 */

#define PWM_OUT_PIN         (1U << 6)    /* PA6  -> TIM16_CH1 AF5 */

#define MODE_BTN_PIN        (1U << 13)   /* PC13 blue button */
#define DEC_BTN_PIN         (1U << 8)    /* PD8  decrease */
#define INC_BTN_PIN         (1U << 9)    /* PD9  increase */

#define UART_TX_PIN         (1U << 2)    /* PA2 -> USART2_TX AF1 */
#define UART_RX_PIN         (1U << 3)    /* PA3 -> USART2_RX AF1 */

/* -------------------- Timing constants -------------------- */
/* System clock = 50 MHz. Use PSC=49 for 1 MHz timer tick => 1 tick = 1 us */
#define TIMER_1US_PSC       49U

/* Q1 toggle intervals */
#define Q1_CH1_US           250U
#define Q1_CH2_US           400U

/* Q2/Q3/Q4 PWM settings */
#define PWM_FREQ_HZ         5000U
#define PWM_PERIOD_US       200U         /* 1 / 5000 = 200 us */
#define PWM_ARR             (PWM_PERIOD_US - 1U)

#define DUTY_MIN            5
#define DUTY_MAX            95
#define DUTY_STEP           5

/* -------------------- Globals -------------------- */
volatile uint32_t g_tim15_next_cc1 = Q1_CH1_US;
volatile uint32_t g_tim15_next_cc2 = Q1_CH2_US;

volatile int g_duty_percent = 25;
volatile ChannelMode_Typedef g_pwm_mode = Pwm1;
volatile int g_display_dirty = 1;

/* Button edge tracking */
static int prev_mode_btn = 0;
static int prev_dec_btn  = 0;
static int prev_inc_btn  = 0;

/* -------------------- Helpers -------------------- */
static int button_pressed(GPIO_TypeDef *port, uint16_t pin)
{
    /* Assumes active-low buttons */
    return (GPIO_Read(port, pin) == 0);
}

static void duty_to_string(char *buf, int duty)
{
    /* Output example: "DUTY: 25%   " */
    buf[0]  = 'D';
    buf[1]  = 'U';
    buf[2]  = 'T';
    buf[3]  = 'Y';
    buf[4]  = ':';
    buf[5]  = ' ';
    buf[6]  = (duty >= 10) ? (char)('0' + (duty / 10)) : ' ';
    buf[7]  = (char)('0' + (duty % 10));
    buf[8]  = '%';
    buf[9]  = ' ';
    buf[10] = ' ';
    buf[11] = ' ';
    buf[12] = '\0';
}

static void update_duty_display(void)
{
    char msg[13];

    duty_to_string(msg, g_duty_percent);
    TERM_TxStringXY(USART2, 10, 10, msg);
}

static void apply_pwm_settings(void)
{
    uint32_t ccr_ticks;

    /* 200 us period, duty from 5% to 95% */
    ccr_ticks = ((uint32_t)g_duty_percent * PWM_PERIOD_US) / 100U;

    Timer_SetupChannel(TIM16, TimCCR1, g_pwm_mode);
    Timer_WriteCCR(TIM16, TimCCR1, ccr_ticks);

    /* Advanced timer output enable */
    TIM16->BDTR |= TIM_BDTR_MOE;

    /* Load updated registers */
    TIM16->EGR |= TIM_EGR_UG;
}

static void init_clock_50mhz(void)
{
    Clock_InitPll(PLL_50MHZ);
}

static void init_buttons(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOCEN | RCC_IOPENR_GPIODEN;

    GPIO_InitInput(GPIOC, MODE_BTN_PIN);
    GPIO_SetPullMode(GPIOC, MODE_BTN_PIN, PullMode_PullUp);

    GPIO_InitInput(GPIOD, DEC_BTN_PIN);
    GPIO_SetPullMode(GPIOD, DEC_BTN_PIN, PullMode_PullUp);

    GPIO_InitInput(GPIOD, INC_BTN_PIN);
    GPIO_SetPullMode(GPIOD, INC_BTN_PIN, PullMode_PullUp);
}

static void init_usart2(void)
{
    RCC->IOPENR   |= RCC_IOPENR_GPIOAEN;
    RCC->APBENR1  |= RCC_APBENR1_USART2EN;

    GPIO_InitAlternateF(GPIOA, UART_TX_PIN, 1);
    GPIO_InitAlternateF(GPIOA, UART_RX_PIN, 1);

    /*
     * UART_Init() assumes 16 MHz peripheral clock.
     * Select HSI16 as USART2 clock source so 115200 is correct.
     */
    RCC->CCIPR &= ~RCC_CCIPR_USART2SEL;
    RCC->CCIPR |= (2U << RCC_CCIPR_USART2SEL_Pos);   /* 10: HSI16 */

    UART_Init(USART2, 115200, 0);
    TERM_ClearScreen(USART2);
    update_duty_display();
}

static void init_q1_tim15_toggle(void)
{
    RCC->IOPENR   |= RCC_IOPENR_GPIOBEN;
    RCC->APBENR2  |= RCC_APBENR2_TIM15EN;

    /* PB14 -> TIM15_CH1 AF5, PB15 -> TIM15_CH2 AF5 */
    GPIO_InitAlternateF(GPIOB, Q1_CH1_PIN, 5);
    GPIO_InitAlternateF(GPIOB, Q1_CH2_PIN, 5);

    /* Free-running timer, 1 us tick */
    Timer_Init(TIM15, TIMER_1US_PSC, 0xFFFFU);

    Timer_SetupChannel(TIM15, TimCCR1, OutputCompareToggle);
    Timer_SetupChannel(TIM15, TimCCR2, OutputCompareToggle);

    /*
     * To get independent square waves on the same timer with different
     * toggle intervals, reschedule CCR1/CCR2 in the interrupt.
     */
    Timer_WriteCCR(TIM15, TimCCR1, g_tim15_next_cc1);
    Timer_WriteCCR(TIM15, TimCCR2, g_tim15_next_cc2);

    TIM15->BDTR |= TIM_BDTR_MOE;

    Timer_EnableInterrupt(TIM15, TIM15_IRQn, (Timer_IE)(TimCC1IE | TimCC2IE));
    Timer_SetEnable(TIM15, 1);
}

static void init_q2_tim16_pwm(void)
{
    RCC->IOPENR   |= RCC_IOPENR_GPIOAEN;
    RCC->APBENR2  |= RCC_APBENR2_TIM16EN;

    /* PA6 -> TIM16_CH1 AF5 */
    GPIO_InitAlternateF(GPIOA, PWM_OUT_PIN, 5);

    /* 1 us tick, ARR = 199 -> 200 us period -> 5 kHz */
    Timer_Init(TIM16, TIMER_1US_PSC, PWM_ARR);

    apply_pwm_settings();
    Timer_SetEnable(TIM16, 1);
}

static void process_buttons(void)
{
    int mode_now = button_pressed(GPIOC, MODE_BTN_PIN);
    int dec_now  = button_pressed(GPIOD, DEC_BTN_PIN);
    int inc_now  = button_pressed(GPIOD, INC_BTN_PIN);

    /* Q3: PC13 toggles PWM1 <-> PWM2 once per press */
    if ((mode_now != 0) && (prev_mode_btn == 0))
    {
        if (g_pwm_mode == Pwm1)
        {
            g_pwm_mode = Pwm2;
        }
        else
        {
            g_pwm_mode = Pwm1;
        }

        apply_pwm_settings();
    }
    prev_mode_btn = mode_now;

    /* Q4: PD8 decrease duty by 5%, floor at 5% */
    if ((dec_now != 0) && (prev_dec_btn == 0))
    {
        if (g_duty_percent > DUTY_MIN)
        {
            g_duty_percent -= DUTY_STEP;
            if (g_duty_percent < DUTY_MIN)
            {
                g_duty_percent = DUTY_MIN;
            }

            apply_pwm_settings();
            g_display_dirty = 1;
        }
    }
    prev_dec_btn = dec_now;

    /* Q4: PD9 increase duty by 5%, cap at 95% */
    if ((inc_now != 0) && (prev_inc_btn == 0))
    {
        if (g_duty_percent < DUTY_MAX)
        {
            g_duty_percent += DUTY_STEP;
            if (g_duty_percent > DUTY_MAX)
            {
                g_duty_percent = DUTY_MAX;
            }

            apply_pwm_settings();
            g_display_dirty = 1;
        }
    }
    prev_inc_btn = inc_now;
}

/* -------------------- Interrupts -------------------- */
void TIM15_IRQHandler(void)
{
    if ((TIM15->SR & TIM_SR_CC1IF) != 0U)
    {
        TIM15->SR &= ~TIM_SR_CC1IF;
        g_tim15_next_cc1 += Q1_CH1_US;
        TIM15->CCR1 = g_tim15_next_cc1;
    }

    if ((TIM15->SR & TIM_SR_CC2IF) != 0U)
    {
        TIM15->SR &= ~TIM_SR_CC2IF;
        g_tim15_next_cc2 += Q1_CH2_US;
        TIM15->CCR2 = g_tim15_next_cc2;
    }
}

/* -------------------- main -------------------- */
int main(void)
{
    init_clock_50mhz();
    init_buttons();
    init_usart2();
    init_q1_tim15_toggle();
    init_q2_tim16_pwm();

    while (1)
    {
        process_buttons();

        if (g_display_dirty != 0)
        {
            update_duty_display();
            g_display_dirty = 0;
        }
    }
}
```

A few notes so you do not get stuck in lab:

* This code assumes the buttons are **active-low**. If your PD8/PD9 board reads the opposite way, change `button_pressed()` so pressed returns `GPIO_Read(port, pin) == 1`.
* Q1 is done with **TIM15 compare interrupts** because one timer has one ARR, so two different toggle intervals on two channels need CCR rescheduling.
* Q2-Q4 use **TIM16 CH1 on PA6** in PWM mode.
* Q5 uses your terminal helpers for cursor positioning and printing. Your uploaded UART helper functions already support that. 

If you want, I can also give you this split into the exact files your instructor will expect, like `main.c` plus a very short explanation of each question.
