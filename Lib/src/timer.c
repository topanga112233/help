#include "timer.h"

static volatile uint32_t * Timer_GetCCR(TIM_TypeDef * timer, CCR_Typedef ccr)
{
    return (volatile uint32_t *)((uint8_t *)timer + 0x34U + (uint32_t)ccr);
}

static uint8_t Timer_GetChannelIndex(CCR_Typedef ccr)
{
    if (ccr == TimCCR1) return 1U;
    if (ccr == TimCCR2) return 2U;
    if (ccr == TimCCR3) return 3U;
    if (ccr == TimCCR4) return 4U;
    return 0U;
}

void Timer_Init(TIM_TypeDef * timer, uint16_t psc, uint16_t period)
{
    timer->CR1 &= ~TIM_CR1_CEN;
    timer->PSC = psc;
    timer->ARR = period;
    timer->CNT = 0U;
    timer->EGR |= TIM_EGR_UG;
}

void Timer_SetupChannel(TIM_TypeDef * timer, CCR_Typedef ccr, ChannelMode_Typedef chMode)
{
    uint8_t ch = Timer_GetChannelIndex(ccr);

    if (ch == 1U)
    {
        timer->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);
        if (chMode == InputCapture)
        {
            timer->CCMR1 |= 0x01U;
        }
        else if (chMode == OutputCompareToggle)
        {
            timer->CCMR1 |= (0x3U << TIM_CCMR1_OC1M_Pos);
        }
        else if (chMode == Pwm1)
        {
            timer->CCMR1 |= (0x6U << TIM_CCMR1_OC1M_Pos);
            timer->CCMR1 |= TIM_CCMR1_OC1PE;
        }
        else if (chMode == Pwm2)
        {
            timer->CCMR1 |= (0x7U << TIM_CCMR1_OC1M_Pos);
            timer->CCMR1 |= TIM_CCMR1_OC1PE;
        }

        timer->CCER |= TIM_CCER_CC1E;
    }
    else if (ch == 2U)
    {
        timer->CCMR1 &= ~(TIM_CCMR1_CC2S | TIM_CCMR1_OC2M);
        if (chMode == InputCapture)
        {
            timer->CCMR1 |= (0x01U << TIM_CCMR1_CC2S_Pos);
        }
        else if (chMode == OutputCompareToggle)
        {
            timer->CCMR1 |= (0x3U << TIM_CCMR1_OC2M_Pos);
        }
        else if (chMode == Pwm1)
        {
            timer->CCMR1 |= (0x6U << TIM_CCMR1_OC2M_Pos);
            timer->CCMR1 |= TIM_CCMR1_OC2PE;
        }
        else if (chMode == Pwm2)
        {
            timer->CCMR1 |= (0x7U << TIM_CCMR1_OC2M_Pos);
            timer->CCMR1 |= TIM_CCMR1_OC2PE;
        }

        timer->CCER |= TIM_CCER_CC2E;
    }
    else if (ch == 3U)
    {
        timer->CCMR2 &= ~(TIM_CCMR2_CC3S | TIM_CCMR2_OC3M);
        if (chMode == InputCapture)
        {
            timer->CCMR2 |= 0x01U;
        }
        else if (chMode == OutputCompareToggle)
        {
            timer->CCMR2 |= (0x3U << TIM_CCMR2_OC3M_Pos);
        }
        else if (chMode == Pwm1)
        {
            timer->CCMR2 |= (0x6U << TIM_CCMR2_OC3M_Pos);
            timer->CCMR2 |= TIM_CCMR2_OC3PE;
        }
        else if (chMode == Pwm2)
        {
            timer->CCMR2 |= (0x7U << TIM_CCMR2_OC3M_Pos);
            timer->CCMR2 |= TIM_CCMR2_OC3PE;
        }

        timer->CCER |= TIM_CCER_CC3E;
    }
    else if (ch == 4U)
    {
        timer->CCMR2 &= ~(TIM_CCMR2_CC4S | TIM_CCMR2_OC4M);
        if (chMode == InputCapture)
        {
            timer->CCMR2 |= (0x01U << TIM_CCMR2_CC4S_Pos);
        }
        else if (chMode == OutputCompareToggle)
        {
            timer->CCMR2 |= (0x3U << TIM_CCMR2_OC4M_Pos);
        }
        else if (chMode == Pwm1)
        {
            timer->CCMR2 |= (0x6U << TIM_CCMR2_OC4M_Pos);
            timer->CCMR2 |= TIM_CCMR2_OC4PE;
        }
        else if (chMode == Pwm2)
        {
            timer->CCMR2 |= (0x7U << TIM_CCMR2_OC4M_Pos);
            timer->CCMR2 |= TIM_CCMR2_OC4PE;
        }

        timer->CCER |= TIM_CCER_CC4E;
    }
}

void Timer_WriteCCR(TIM_TypeDef * timer, CCR_Typedef ccr, uint32_t ccrTicks)
{
    *Timer_GetCCR(timer, ccr) = ccrTicks;
}

void Timer_EnableInterrupt(TIM_TypeDef * timer, IRQn_Type timerIRQn, Timer_IE interruptMask)
{
    timer->DIER |= (uint32_t)interruptMask;
    NVIC_ClearPendingIRQ(timerIRQn);
    NVIC_EnableIRQ(timerIRQn);
}

void Timer_SetEnable(TIM_TypeDef * timer, uint16_t en)
{
    if (en != 0U)
    {
        timer->CR1 |= TIM_CR1_CEN;
    }
    else
    {
        timer->CR1 &= ~TIM_CR1_CEN;
    }
}

void Timer_SetmsTick(TIM_TypeDef * pTimer)
{
    /* Assumes timer input clock = 16 MHz */
    pTimer->PSC = 16000U - 1U;
    pTimer->ARR = 1U - 1U;
    pTimer->CNT = 0U;
    pTimer->EGR |= TIM_EGR_UG;
}

void Timer_SetusTick(TIM_TypeDef * pTimer)
{
    /* Assumes timer input clock = 16 MHz */
    pTimer->PSC = 16U - 1U;
    pTimer->ARR = 1U - 1U;
    pTimer->CNT = 0U;
    pTimer->EGR |= TIM_EGR_UG;
}

void Timer_DelayTicks(TIM_TypeDef * pTimer, uint16_t ticks)
{
    uint16_t i;

    for (i = 0; i < ticks; i++)
    {
        pTimer->SR &= ~TIM_SR_UIF;
        pTimer->CNT = 0U;
        pTimer->CR1 |= TIM_CR1_CEN;

        while ((pTimer->SR & TIM_SR_UIF) == 0U)
        {
        }

        pTimer->CR1 &= ~TIM_CR1_CEN;
    }
}

uint8_t Timer_PollUIF(TIM_TypeDef * pTimer)
{
    if ((pTimer->SR & TIM_SR_UIF) != 0U)
    {
        pTimer->SR &= ~TIM_SR_UIF;
        return 1U;
    }

    return 0U;
}