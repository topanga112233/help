//******************
//Timer / PWM Library
//
// CREATED: 10/04/2024, by Carlos Estay
//
// FILE: timer.h
//

#ifndef TIMER_H
#define TIMER_H

#include "stm32g0b1xx.h"

typedef enum CCR_Typedef__
{//Offset from CCR1
    TimCCR1 = 0x00,   //0x34 -  Offset from Base Timer Addr
    TimCCR2 = 0x04,   //0x38 -  Offset from Base Timer Addr
    TimCCR3 = 0x08,   //0x3C -  Offset from Base Timer Addr
    TimCCR4 = 0x0C,   //0x40 -  Offset from Base Timer Addr
    TimCCR5 = 0x24,   //0x58 -  Offset from Base Timer Addr
    TimCCR6 = 0x28,   //0x5C -  Offset from Base Timer Addr
}CCR_Typedef;

typedef enum ChannelMode_Typedef__
{
    OutputCompareToggle,
    InputCapture,
    Pwm1,       //Positive polarity
    Pwm2,       //NegativePolarity
}ChannelMode_Typedef;


typedef enum Timer_IE_Typedef__
{

    TimUIE = 0x01 << 0,         //Interrupt for Counter Overflow event
    TimCC1IE = 0x01 << 1,       //Interrupt for Capture/Compare Register 1
    TimCC2IE = 0x01 << 2,       //Interrupt for Capture/Compare Register 2
    TimCC3IE = 0x01 << 3,       //Interrupt for Capture/Compare Register 3
    TimCC4IE = 0x01 << 4,       //Interrupt for Capture/Compare Register 4
}Timer_IE;

/// @brief This funtion enables a timer, using a prescaler(psc) from
///        the BUS speed, a period for reload(ARR) in ticks.
/// @param timer        Timer module
/// @param psc          Prescaler
/// @param period       Auto reload register (modulus counter)
void Timer_Init(TIM_TypeDef * timer, uint16_t psc, uint16_t period);

/// @brief Enables a channel according to the mode described in ChannelMode_Typedef enum
/// @param timer    Timer module 
/// @param ccr      Capture/compre register
/// @param chMode   Mode
void Timer_SetupChannel(TIM_TypeDef * timer, CCR_Typedef ccr, ChannelMode_Typedef chMode);

/// @brief Writes a value directly into the Output compare register CCRx
/// @param timer        Timer module 
/// @param ccr          Capture/compre register    
/// @param ccrTicks     Number to write in timer ticks 
void Timer_WriteCCR(TIM_TypeDef * timer, CCR_Typedef ccr, uint32_t ccrTicks);

/// @brief Enables a specific interrupt for the timer
/// @param timer                Timer module  
/// @param timerIRQn            IRQn (could be shred), check IRQn_Type list in stm32g031xx.h
/// @param interruptMask        Interrupt mask according to Timer_IE enum        
void Timer_EnableInterrupt(TIM_TypeDef * timer, IRQn_Type timerIRQn, Timer_IE interruptMask);

/// @brief Starts the timer, call this fucntion after all setting are ready
/// @param timer                Timer module 
/// @param en                   !0: enable / 0: disable 
void Timer_SetEnable(TIM_TypeDef * timer, uint16_t en);

void Timer_SetmsTick(TIM_TypeDef * pTimer);
void Timer_SetusTick(TIM_TypeDef * pTimer);
void Timer_DelayTicks(TIM_TypeDef * pTimer, uint16_t ticks);
uint8_t Timer_PollUIF(TIM_TypeDef * pTimer);

#endif /* TIMER_H */