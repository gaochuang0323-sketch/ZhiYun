/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DAC_CS_Pin GPIO_PIN_4
#define DAC_CS_GPIO_Port GPIOA
#define temp6a_Pin GPIO_PIN_6
#define temp6a_GPIO_Port GPIOG
#define temp6b_Pin GPIO_PIN_7
#define temp6b_GPIO_Port GPIOG
#define temp5b_Pin GPIO_PIN_6
#define temp5b_GPIO_Port GPIOC
#define temp5a_Pin GPIO_PIN_8
#define temp5a_GPIO_Port GPIOC
#define temp4b_Pin GPIO_PIN_9
#define temp4b_GPIO_Port GPIOC
#define temp4a_Pin GPIO_PIN_8
#define temp4a_GPIO_Port GPIOA
#define DAC_ALMOUT_Pin GPIO_PIN_2
#define DAC_ALMOUT_GPIO_Port GPIOD
#define DAC_RESET_Pin GPIO_PIN_3
#define DAC_RESET_GPIO_Port GPIOD
#define DAC_nLDAC_Pin GPIO_PIN_6
#define DAC_nLDAC_GPIO_Port GPIOD
#define temp3a_Pin GPIO_PIN_9
#define temp3a_GPIO_Port GPIOG
#define temp3b_Pin GPIO_PIN_10
#define temp3b_GPIO_Port GPIOG
#define temp7a_Pin GPIO_PIN_11
#define temp7a_GPIO_Port GPIOG
#define temp7b_Pin GPIO_PIN_12
#define temp7b_GPIO_Port GPIOG
#define temp2a_Pin GPIO_PIN_13
#define temp2a_GPIO_Port GPIOG
#define temp2b_Pin GPIO_PIN_15
#define temp2b_GPIO_Port GPIOG
#define temp1a_Pin GPIO_PIN_3
#define temp1a_GPIO_Port GPIOB
#define temp1b_Pin GPIO_PIN_4
#define temp1b_GPIO_Port GPIOB
#define TOGGLE2_Pin GPIO_PIN_5
#define TOGGLE2_GPIO_Port GPIOB
#define TOGGLE1_Pin GPIO_PIN_6
#define TOGGLE1_GPIO_Port GPIOB
#define TOGGLE0_Pin GPIO_PIN_7
#define TOGGLE0_GPIO_Port GPIOB
#define LED_YL_Pin GPIO_PIN_0
#define LED_YL_GPIO_Port GPIOE
#define LED_RY_Pin GPIO_PIN_1
#define LED_RY_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
