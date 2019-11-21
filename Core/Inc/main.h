/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

	/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

	
	extern	SPI_HandleTypeDef HSPI_SDCARD;
	extern  uint8_t SPI_Complete;
	
	
	/* Exported functions prototypes ---------------------------------------------*/
	void Error_Handler(void);
	void UART_Printf(const char* fmt, ...);
	int getButtonNumber(int xInput, int yInput);
	void save();
	void load();
	/* USER CODE BEGIN EFP */

	/* USER CODE END EFP */

	/* Private defines -----------------------------------------------------------*/
#define Giro_Block_Pin GPIO_PIN_3
#define Giro_Block_GPIO_Port GPIOE
#define ILI9341_RS_Pin GPIO_PIN_4
#define ILI9341_RS_GPIO_Port GPIOC
#define ILI9341_RST_Pin GPIO_PIN_5
#define ILI9341_RST_GPIO_Port GPIOC
#define TOUCH_SCK_Pin GPIO_PIN_13
#define TOUCH_SCK_GPIO_Port GPIOB
#define TOUCH_MISO_Pin GPIO_PIN_14
#define TOUCH_MISO_GPIO_Port GPIOB
#define TOUCH_MOSI_Pin GPIO_PIN_15
#define TOUCH_MOSI_GPIO_Port GPIOB
#define ILI9341_CS_Pin GPIO_PIN_6
#define ILI9341_CS_GPIO_Port GPIOC
#define SD_CS_Pin GPIO_PIN_7
#define SD_CS_GPIO_Port GPIOC
#define TOUCH_CS_Pin GPIO_PIN_8
#define TOUCH_CS_GPIO_Port GPIOC
#define TOUCH_IRQ_Pin GPIO_PIN_9
#define TOUCH_IRQ_GPIO_Port GPIOC
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define SD_SCK_Pin GPIO_PIN_10
#define SD_SCK_GPIO_Port GPIOC
#define SD_MISO_Pin GPIO_PIN_11
#define SD_MISO_GPIO_Port GPIOC
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define SD_MOSI_Pin GPIO_PIN_5
#define SD_MOSI_GPIO_Port GPIOB
	/* USER CODE BEGIN Private defines */

	/* USER CODE END Private defines */
	
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
