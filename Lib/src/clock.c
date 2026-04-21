#include "clock.h"

void Clock_InitPll(PllRange pllRange)
{
    /* Enable HSI16 */
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U)
    {
    }

    /* Disable PLL */
    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0U)
    {
    }

    /* Configure PLL */
    RCC->PLLCFGR =
          RCC_PLLCFGR_PLLSRC_HSI
        | (uint32_t)pllRange
        | RCC_PLLCFGR_PLLREN;

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U)
    {
    }

    /* Flash latency */
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    /* Switch SYSCLK to PLL */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    /* Wait until PLL is used */
    while ((RCC->CFGR & RCC_CFGR_SWS) != (0x02U << RCC_CFGR_SWS_Pos))
    {
    }
}

void Clock_EnableOutput(MCO_Select sel, MCO_Div div)
{
    RCC->CFGR &= ~(RCC_CFGR_MCOSEL | RCC_CFGR_MCOPRE);
    RCC->CFGR |= ((uint32_t)sel | (uint32_t)div);
}