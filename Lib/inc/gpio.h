//******************
//GPIO Library
//
// CREATED: 10/27/2023, by Carlos Estay
//
// FILE: gpio.h
//
#include "stm32g0b1xx.h"

#ifndef GPIO_H
#define GPIO_H

typedef enum GPIO_IO_PullModeTypedef__
{
  PullMode_None = 0b00,
  PullMode_PullUp = 0b01,
  PullMode_PullDown = 0b10,
  PullMode_Mask = 0b11
}IO_PullMode;

typedef enum GPIO_IO_TypeTypedef__
{
  Type_PushPull = 0,
  Type_OpenDrain = 1,
}IO_Type;

typedef enum GPIO_IO_SpeedTypedef__
{
  Speed_VeryLow = 0b00,
  Speed_Low = 0b01,
  Speed_High = 0b10,
  Speed_VeryHigh = 0b11
}IO_Speed;

  /******Prototypes******/

  /// @brief Set GPIO pin as output
  /// @param  GPIO Port
  /// @param GPIO pin 
  void GPIO_InitInput(GPIO_TypeDef*, uint16_t);

  /// @brief Set GPIO pin as input
  /// @param  GPIO Port
  /// @param GPIO pin  
  void GPIO_InitOutput(GPIO_TypeDef*, uint16_t);

  /// @brief Set pull-up or pull-down
  /// @param  GPIO Port
  /// @param GPIO pin  
  /// @param  mode
  void GPIO_SetPullMode(GPIO_TypeDef*, uint16_t, IO_PullMode);

  /// @brief Set type of Output: Push-Pull or Open Collector
  /// @param  GPIO Port
  /// @param GPIO pin  
  /// @param  type
  void GPIO_SetType(GPIO_TypeDef*, uint16_t, IO_Type);
  
  /// @brief Set output speed for the Pin
  /// @param  GPIO Port
  /// @param  GPIO pin  
  /// @param  speed 
  void GPIO_SetSpeed(GPIO_TypeDef*, uint16_t, IO_Speed);

  /// @brief Configure Pin with all necessary settings for I2C
  /// @param  GPIO Port
  /// @param  GPIO Pin
  void GPIO_I2C_Config(GPIO_TypeDef*, uint16_t);
  
  /// @brief Set alternate function (AFx MUX)
  /// @param  GPIO Port
  /// @param GPIO pin   
  /// @param AF 
  void GPIO_InitAlternateF(GPIO_TypeDef*, uint16_t, uint16_t);
  
  /// @brief Set bit in Port
  /// @param Port 
  /// @param pin  
  void GPIO_Set(GPIO_TypeDef*, uint16_t);
  
  /// @brief Clear bit in Port
  /// @param Port 
  /// @param pin  
  void GPIO_Clear(GPIO_TypeDef*, uint16_t);
  
  /// @brief Toggle bit in Port
  /// @param Port 
  /// @param pin   
  void GPIO_Toggle(GPIO_TypeDef* , uint16_t);
  
  /// @brief Reads pin in Port
  /// @param Port 
  /// @param pin       
  int GPIO_Read(GPIO_TypeDef*, uint16_t);

#endif /* GPIO_H */