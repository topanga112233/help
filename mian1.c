#include "stm32g0b1xx.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "uart.h"

#define SYSCLK_HZ           50000000UL

#define PWM_FREQ_HZ         5000UL
#define PWM_PERIOD_TICKS    1000UL     // 5kHz from 50MHz with PSC=9 => 5MHz tick, ARR=999
#define PWM_PSC             9U
#define PWM_ARR             999U

#define DUTY_MIN            5U
#define DUTY_MAX            95U
#define DUTY_STEP           5U

#define Q1_TIM15_PSC        49U        // 50MHz/(49+1)=1MHz => 1 tick = 1us
#define Q1_TIM15_ARR        649U       // full cycle = 650us
#define Q1_CH1_HIGH_US      250U       // PB14
#define Q1_CH2_HIGH_US      400U       // PB15

static volatile uint8_t g_dutyPercent = 25;
static volatile uint8_t g_pwmModeIsPwm1 = 1;

static void RCC_EnablePeripherals(void);
static void GPIO_Config(void);
static void TIM15_Config_Q1(void);
static void TIM16_Config_Q2_Q3_Q4(void);
static void USART2_Config_Q5(void);
static void UpdatePwmDutyAndMode(void);
static void UpdateTerminalDuty(void);
static void DelayMs(uint16_t ms);
static uint8_t ButtonPressedEdge(GPIO_TypeDef *port, uint16_t pin, uint8_t *state);
static void TIM15_SetupPseudoTogglePulse(void);

int main(void)
{
    uint8_t bluePrev = 0;
    uint8_t sw1Prev = 0;
    uint8_t sw2Prev = 0;

    Clock_InitPll(PLL_50MHZ);
    RCC_EnablePeripherals();
    GPIO_Config();
    TIM15_Config_Q1();
    TIM16_Config_Q2_Q3_Q4();
    USART2_Config_Q5();

    TERM_ClearScreen(USART2);
    UpdateTerminalDuty();

    while (1)
    {
        if (ButtonPressedEdge(GPIOC, GPIO_PIN_13, &bluePrev))
        {
            g_pwmModeIsPwm1 ^= 1U;
            UpdatePwmDutyAndMode();
            UpdateTerminalDuty();
        }

        if (ButtonPressedEdge(GPIOD, GPIO_PIN_8, &sw1Prev))
        {
            if (g_dutyPercent > DUTY_MIN)
            {
                g_dutyPercent -= DUTY_STEP;
            }
            UpdatePwmDutyAndMode();
            UpdateTerminalDuty();
        }

        if (ButtonPressedEdge(GPIOD, GPIO_PIN_9, &sw2Prev))
        {
            if (g_dutyPercent < DUTY_MAX)
            {
                g_dutyPercent += DUTY_STEP;
            }
            UpdatePwmDutyAndMode();
            UpdateTerminalDuty();
        }

        /*
         * Q1 visible square waves on PB14/PB15:
         * Re-arm compare points every update event so each channel
         * goes high at CNT=0 and toggles low at its compare value.
         */
        if (Timer_PollUIF(TIM15))
        {
            TIM15_SetupPseudoTogglePulse();
        }
    }
}

static void RCC_EnablePeripherals(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN
                |  RCC_IOPENR_GPIOBEN
                |  RCC_IOPENR_GPIOCEN
                |  RCC_IOPENR_GPIODEN;

    RCC->APBENR2 |= RCC_APBENR2_TIM15EN
                 |  RCC_APBENR2_TIM16EN
                 |  RCC_APBENR2_USART1EN; /* not used, harmless if left */

    RCC->APBENR1 |= RCC_APBENR1_USART2EN;
}

static void GPIO_Config(void)
{
    /* Q1: TIM15_CH1 on PB14, TIM15_CH2 on PB15, AF5 from exam sheet */
    GPIO_InitAlternateF(GPIOB, GPIO_PIN_14, 5);
    GPIO_InitAlternateF(GPIOB, GPIO_PIN_15, 5);
    GPIO_SetSpeed(GPIOB, GPIO_PIN_14, Speed_High);
    GPIO_SetSpeed(GPIOB, GPIO_PIN_15, Speed_High);

    /* Q2/Q3/Q4: TIM16_CH1 on PA6, AF5 from exam sheet */
    GPIO_InitAlternateF(GPIOA, GPIO_PIN_6, 5);
    GPIO_SetSpeed(GPIOA, GPIO_PIN_6, Speed_High);

    /* Q5: USART2
       Common Nucleo/STM32G0 mapping is PA2=TX, PA3=RX AF1.
       Change AF/pins here if your board uses different pins. */
    GPIO_InitAlternateF(GPIOA, GPIO_PIN_2, 1);
    GPIO_InitAlternateF(GPIOA, GPIO_PIN_3, 1);
    GPIO_SetSpeed(GPIOA, GPIO_PIN_2, Speed_High);
    GPIO_SetSpeed(GPIOA, GPIO_PIN_3, Speed_High);

    /* Buttons: PC13 blue button, PD8 SW1, PD9 SW2 */
    GPIO_InitInput(GPIOC, GPIO_PIN_13);
    GPIO_SetPullMode(GPIOC, GPIO_PIN_13, PullMode_PullUp);

    GPIO_InitInput(GPIOD, GPIO_PIN_8);
    GPIO_SetPullMode(GPIOD, GPIO_PIN_8, PullMode_PullUp);

    GPIO_InitInput(GPIOD, GPIO_PIN_9);
    GPIO_SetPullMode(GPIOD, GPIO_PIN_9, PullMode_PullUp);
}

static void TIM15_Config_Q1(void)
{
    /* 1 MHz timer tick => 1us resolution */
    Timer_Init(TIM15, Q1_TIM15_PSC, Q1_TIM15_ARR);

    /* Output compare toggle on CH1 and CH2 */
    Timer_SetupChannel(TIM15, TimCCR1, OutputCompareToggle);
    Timer_SetupChannel(TIM15, TimCCR2, OutputCompareToggle);

    /*
     * To create measurable positive width:
     * - force outputs high at start of each cycle
     * - toggle low at CCR1=250 and CCR2=400
     * - period = 650us, then repeated
     *
     * This gives positive pulses 250us and 400us wide.
     */
    TIM15->BDTR |= TIM_BDTR_MOE;
    TIM15_SetupPseudoTogglePulse();
    Timer_SetEnable(TIM15, 1);
}

static void TIM15_SetupPseudoTogglePulse(void)
{
    /* Start of new cycle: drive channels active/high */
    TIM15->CNT = 0U;

    /* Force OC1/OC2 active briefly by mode rewrite */
    TIM15->CCMR1 &= ~((0x7U << TIM_CCMR1_OC1M_Pos) | (0x7U << TIM_CCMR1_OC2M_Pos));
    TIM15->CCMR1 |=  ((0x5U << TIM_CCMR1_OC1M_Pos) | (0x5U << TIM_CCMR1_OC2M_Pos)); /* force active */

    /* Load compare times */
    Timer_WriteCCR(TIM15, TimCCR1, Q1_CH1_HIGH_US);
    Timer_WriteCCR(TIM15, TimCCR2, Q1_CH2_HIGH_US);

    /* Restore toggle mode so match toggles low */
    TIM15->CCMR1 &= ~((0x7U << TIM_CCMR1_OC1M_Pos) | (0x7U << TIM_CCMR1_OC2M_Pos));
    TIM15->CCMR1 |=  ((0x3U << TIM_CCMR1_OC1M_Pos) | (0x3U << TIM_CCMR1_OC2M_Pos));
}

static void TIM16_Config_Q2_Q3_Q4(void)
{
    Timer_Init(TIM16, PWM_PSC, PWM_ARR);
    TIM16->BDTR |= TIM_BDTR_MOE;

    UpdatePwmDutyAndMode();

    Timer_SetEnable(TIM16, 1);
}

static void UpdatePwmDutyAndMode(void)
{
    uint32_t ccr;

    ccr = ((uint32_t)g_dutyPercent * (PWM_ARR + 1U)) / 100U;
    if (ccr > 0U)
    {
        ccr -= 1U;
    }

    if (g_pwmModeIsPwm1 != 0U)
    {
        Timer_SetupChannel(TIM16, TimCCR1, Pwm1);
    }
    else
    {
        Timer_SetupChannel(TIM16, TimCCR1, Pwm2);
    }

    Timer_WriteCCR(TIM16, TimCCR1, ccr);
    TIM16->EGR |= TIM_EGR_UG;
}

static void USART2_Config_Q5(void)
{
    UART_Init(USART2, 115200, 0);
}

static void UpdateTerminalDuty(void)
{
    char msg[20];
    uint8_t tens = g_dutyPercent / 10U;
    uint8_t ones = g_dutyPercent % 10U;
    uint8_t n = 0;

    msg[n++] = 'D';
    msg[n++] = 'U';
    msg[n++] = 'T';
    msg[n++] = 'Y';
    msg[n++] = ':';
    msg[n++] = ' ';

    if (tens > 0U)
    {
        msg[n++] = (char)('0' + tens);
    }
    else
    {
        msg[n++] = '0';
    }

    msg[n++] = (char)('0' + ones);
    msg[n++] = '%';
    msg[n++] = ' ';
    msg[n++] = ' ';
    msg[n++] = ' ';
    msg[n++] = '\0';

    TERM_TxStringXY(USART2, 10, 10, msg);
}

static uint8_t ButtonPressedEdge(GPIO_TypeDef *port, uint16_t pin, uint8_t *state)
{
    uint8_t nowPressed;

    /* Assumes active-low buttons with pull-up */
    nowPressed = (GPIO_Read(port, pin) == 0) ? 1U : 0U;

    if ((nowPressed != 0U) && (*state == 0U))
    {
        DelayMs(20);
        nowPressed = (GPIO_Read(port, pin) == 0) ? 1U : 0U;
        if (nowPressed != 0U)
        {
            *state = 1U;
            return 1U;
        }
    }
    else if (nowPressed == 0U)
    {
        *state = 0U;
    }

    return 0U;
}

static void DelayMs(uint16_t ms)
{
    static uint8_t initDone = 0;

    if (initDone == 0U)
    {
        Timer_SetmsTick(TIM14);
        initDone = 1U;
    }

    RCC->APBENR2 |= RCC_APBENR2_TIM14EN;
    Timer_DelayTicks(TIM14, ms);
}