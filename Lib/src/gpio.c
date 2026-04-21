#include <stm32g0b1xx.h>
#include <gpio.h>
static uint8_t GPIO_GetPinNumber(uint16_t pin)
{
  uint8_t n = 0;

  while (((pin >> n) & 0x01U) == 0U && n < 16U)
  {
    n++;
  }

  return n;
}

void GPIO_InitInput(GPIO_TypeDef* port, uint16_t pin)
{
  uint8_t n = GPIO_GetPinNumber(pin);

  port->MODER &= ~(0x3U << (2U * n));
}

void GPIO_InitOutput(GPIO_TypeDef* port, uint16_t pin)
{
  uint8_t n = GPIO_GetPinNumber(pin);

  port->MODER &= ~(0x3U << (2U * n));
  port->MODER |=  (0x1U << (2U * n));
}

void GPIO_SetPullMode(GPIO_TypeDef* port, uint16_t pin, IO_PullMode mode)
{
  uint8_t n = GPIO_GetPinNumber(pin);

  port->PUPDR &= ~(0x3U << (2U * n));
  port->PUPDR |=  ((uint32_t)mode << (2U * n));
}

void GPIO_SetType(GPIO_TypeDef* port, uint16_t pin, IO_Type type)
{
  uint8_t n = GPIO_GetPinNumber(pin);

  port->OTYPER &= ~(0x1U << n);
  port->OTYPER |=  ((uint32_t)type << n);
}

void GPIO_SetSpeed(GPIO_TypeDef* port, uint16_t pin, IO_Speed speed)
{
  uint8_t n = GPIO_GetPinNumber(pin);

  port->OSPEEDR &= ~(0x3U << (2U * n));
  port->OSPEEDR |=  ((uint32_t)speed << (2U * n));
}

void GPIO_I2C_Config(GPIO_TypeDef* port, uint16_t pin)
{
  GPIO_InitAlternateF(port, pin, 6);          // AF6 is common for I2C on many G0 pins
  GPIO_SetType(port, pin, Type_OpenDrain);
  GPIO_SetPullMode(port, pin, PullMode_PullUp);
  GPIO_SetSpeed(port, pin, Speed_High);
}

void GPIO_InitAlternateF(GPIO_TypeDef* port, uint16_t pin, uint16_t AF)
{
  uint8_t n = GPIO_GetPinNumber(pin);

  port->MODER &= ~(0x3U << (2U * n));
  port->MODER |=  (0x2U << (2U * n));

  if (n < 8U)
  {
    port->AFR[0] &= ~(0xFU << (4U * n));
    port->AFR[0] |=  (((uint32_t)AF & 0xFU) << (4U * n));
  }
  else
  {
    port->AFR[1] &= ~(0xFU << (4U * (n - 8U)));
    port->AFR[1] |=  (((uint32_t)AF & 0xFU) << (4U * (n - 8U)));
  }
}

void GPIO_Set(GPIO_TypeDef* port, uint16_t pin)
{
  port->BSRR = pin;
}

void GPIO_Clear(GPIO_TypeDef* port, uint16_t pin)
{
  port->BRR = pin;
}

void GPIO_Toggle(GPIO_TypeDef* port, uint16_t pin)
{
  if ((port->ODR & pin) != 0U)
  {
    port->BRR = pin;
  }
  else
  {
    port->BSRR = pin;
  }
}

int GPIO_Read(GPIO_TypeDef* port, uint16_t pin)
{
  if ((port->IDR & pin) != 0U)
  {
    return 1;
  }

  return 0;
}