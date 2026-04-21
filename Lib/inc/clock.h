//******************
//Clock Library
//
// CREATED: Sept/24/2024, by Carlos Estay
//
// FILE: clock.h
//
//
#include "stm32g0b1xx.h"

#ifndef CLOCK_H
#define CLOCK_H

#define RCC_CFGR_SW_PLL RCC_CFGR_SW_1 //PLL as system clock
/*  
  PLL_CLK = PLL_IN x (N / M) / R 
  Minumum R = 2

  R and M factor must subtract 1 as 0 means setting 1
*/

#define PLL_CFG(N, M, R)   ( ((uint32_t)((R) & 0x3U) << RCC_PLLCFGR_PLLR_Pos) | \
                             ((uint32_t)(N) << RCC_PLLCFGR_PLLN_Pos)         | \
                             ((uint32_t)((M) & 0x7U) << RCC_PLLCFGR_PLLM_Pos) )


typedef enum PllRangeTypedef__
{
    /* 16 MHz * 8 / 1 / 8 = 16 MHz */
    PLL_16MHZ = PLL_CFG(8,  0, 3),

    /* 16 MHz * 10 / 1 / 8 = 20 MHz */
    PLL_20MHZ = PLL_CFG(10, 0, 3),

    /* 16 MHz * 12 / 1 / 8 = 24 MHz */
    PLL_24MHZ = PLL_CFG(12, 0, 3),

    /* 16 MHz * 8 / 1 / 4 = 32 MHz */
    PLL_32MHZ = PLL_CFG(8,  0, 1),

    /* 16 MHz * 10 / 1 / 4 = 40 MHz */
    PLL_40MHZ = PLL_CFG(10, 0, 1),

    /* 16 MHz * 12 / 1 / 4 = 48 MHz */
    PLL_48MHZ = PLL_CFG(12, 0, 1),

    /* 16 MHz * 25 / 1 / 8 = 50 MHz */
    PLL_50MHZ = PLL_CFG(25, 0, 3),

    /* 16 MHz * 15 / 1 / 4 = 60 MHz */
    PLL_60MHz = PLL_CFG(15, 0, 1),

    /* 16 MHz * 8 / 1 / 2 = 64 MHz */
    PLL_64MHZ = PLL_CFG(8,  0, 0)

}PllRange;


typedef enum MCO_DivTpedef__
{
    MCO_Div1 = 0U << 28,
    MCO_Div2 = 1U << 28,
    MCO_Div4 = 2U << 28,
    MCO_Div8 = 3U << 28,
    MCO_Div16 = 4U << 28,
    MCO_Div128 = 7U << 28
}MCO_Div;

typedef enum MCO_SelectTpedef__
{
    MCO_Sel_None = 0U << 24,
    MCO_Sel_SYSCLK = 1U << 24,
    MCO_Sel_HSI48 = 2U << 24,
    MCO_Sel_HSI16 = 3U << 24,
    MCO_Sel_HSE = 4U << 24,
    MCO_Sel_PLL = 5U << 24,
    MCO_Sel_LSI = 6U << 24,
    MCO_Sel_LSE = 7U << 24
}MCO_Select;


void Clock_InitPll(PllRange);
void Clock_EnableOutput(MCO_Select, MCO_Div);
  
#endif /* CLOCK_H */