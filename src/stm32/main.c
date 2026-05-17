/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Smart Home Menu + OLED + LEDs + Door Servo + Fan Speed
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ssd1306.h"
#include "fonts.h"
/* USER CODE END Includes */

I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim16;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t rx_char;
char rx_buffer[32];
int rx_index = 0;

int light1 = 0;
int light2 = 0;
int light3 = 0;

int door_open = 0;

int fan_speed = 0;
int fan_on = 0;
uint8_t fan_pwm_counter = 0;
uint32_t fan_pwm_last_tick = 0;

int current_screen = 0;   // 0=main menu, 1=lights, 2=fan, 3=door
int menu_index = 0;       // 0=lights, 1=fan, 2=door
int active_light = 1;
/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM16_Init(void);

/* USER CODE BEGIN PFP */
void OLED_ShowMainMenu(void);
void OLED_ShowLightsMenu(void);
void OLED_ShowFanScreen(void);
void OLED_ShowDoorScreen(void);
void Door_Open(void);
void Door_Close(void);
void Fan_SetSpeed(int speed);
void Fan_SoftPWM_Update(void);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

void Fan_SetSpeed(int speed)
{
  if (speed < 0) speed = 0;
  if (speed > 999) speed = 999;

  fan_speed = speed;
  fan_on = (fan_speed > 0) ? 1 : 0;

  if (fan_speed == 0)
  {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
  }

  if (current_screen == 2)
  {
    OLED_ShowFanScreen();
  }
}

void Fan_SoftPWM_Update(void)
{
  uint32_t now = HAL_GetTick();

  if ((now - fan_pwm_last_tick) >= 1)
  {
    fan_pwm_last_tick = now;

    fan_pwm_counter++;
    if (fan_pwm_counter >= 20)
    {
      fan_pwm_counter = 0;
    }

    int duty_steps = (fan_speed * 20) / 1000;

    if (fan_speed <= 0)
    {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    }
    else if (duty_steps >= 20)
    {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
    }
    else
    {
      if (fan_pwm_counter < duty_steps)
      {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
      }
      else
      {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
      }
    }
  }
}

void Door_Open(void)
{
  door_open = 1;

  /*
   * Servo open position.
   * 1000 gives about 30 degrees open in your setup.
   */
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1000);

  if (current_screen == 3)
  {
    OLED_ShowDoorScreen();
  }
}

void Door_Close(void)
{
  door_open = 0;

  /*
   * Servo closed position.
   */
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 500);

  if (current_screen == 3)
  {
    OLED_ShowDoorScreen();
  }
}

void OLED_ShowMainMenu(void)
{
  ssd1306_Fill(Black);

  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Smart Home", Font_7x10, White);

  ssd1306_SetCursor(0, 18);
  ssd1306_WriteString((menu_index == 0) ? "> Lights" : "  Lights", Font_7x10, White);

  ssd1306_SetCursor(0, 32);
  ssd1306_WriteString((menu_index == 1) ? "> Fan" : "  Fan", Font_7x10, White);

  ssd1306_SetCursor(0, 46);
  ssd1306_WriteString((menu_index == 2) ? "> Door" : "  Door", Font_7x10, White);

  ssd1306_UpdateScreen();
}

void OLED_ShowLightsMenu(void)
{
  char line[24];

  ssd1306_Fill(Black);

  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Lights", Font_7x10, White);

  ssd1306_SetCursor(0, 14);
  ssd1306_WriteString((active_light == 1) ? "> Light 1" : "  Light 1", Font_6x8, White);

  if (active_light == 1)
  {
    ssd1306_SetCursor(92, 14);
    ssd1306_WriteString("SEL", Font_6x8, White);
  }

  ssd1306_SetCursor(0, 26);
  ssd1306_WriteString((active_light == 2) ? "> Light 2" : "  Light 2", Font_6x8, White);

  if (active_light == 2)
  {
    ssd1306_SetCursor(92, 26);
    ssd1306_WriteString("SEL", Font_6x8, White);
  }

  ssd1306_SetCursor(0, 38);
  ssd1306_WriteString((active_light == 3) ? "> Light 3" : "  Light 3", Font_6x8, White);

  if (active_light == 3)
  {
    ssd1306_SetCursor(92, 38);
    ssd1306_WriteString("SEL", Font_6x8, White);
  }

  ssd1306_SetCursor(0, 52);
  snprintf(line, sizeof(line), "B:%d %d %d", light1, light2, light3);
  ssd1306_WriteString(line, Font_6x8, White);

  ssd1306_UpdateScreen();
}

void OLED_ShowFanScreen(void)
{
  char line[24];

  ssd1306_Fill(Black);

  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Fan Control", Font_7x10, White);

  ssd1306_SetCursor(0, 18);
  snprintf(line, sizeof(line), "Speed:%d", fan_speed);
  ssd1306_WriteString(line, Font_7x10, White);

  ssd1306_SetCursor(0, 36);
  if (fan_on)
  {
    ssd1306_WriteString("Status: ON", Font_6x8, White);
  }
  else
  {
    ssd1306_WriteString("Status: OFF", Font_6x8, White);
  }

  ssd1306_SetCursor(0, 52);
  ssd1306_WriteString("Pinch=Speed", Font_6x8, White);

  ssd1306_UpdateScreen();
}

void OLED_ShowDoorScreen(void)
{
  ssd1306_Fill(Black);

  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString("Door Control", Font_7x10, White);

  ssd1306_SetCursor(0, 20);

  /*
   * UI matches your current gesture setup:
   * 1 finger = close
   * 2 fingers = open
   */
  if (door_open)
  {
    ssd1306_WriteString("Status: OPEN", Font_7x10, White);
  }
  else
  {
    ssd1306_WriteString("Status: CLOSED", Font_7x10, White);
  }

  ssd1306_SetCursor(0, 42);
  ssd1306_WriteString("1=Close 2=Open", Font_6x8, White);

  ssd1306_UpdateScreen();
}

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_TIM16_Init();

  /*
   * Start PWM channels.
   * TIM1_CH1 = PA8 / D9 = door servo
   * TIM3_CH1 = PB4 = Light 1
   * TIM3_CH2 = PB5 = Light 2
   * TIM2_CH2 = PB3 = Light 3
   */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  __HAL_TIM_MOE_ENABLE(&htim1);

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

  Door_Close();

  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);

  Fan_SetSpeed(0);

  HAL_Delay(200);

  ssd1306_Init();
  HAL_Delay(100);

  ssd1306_Fill(Black);
  ssd1306_UpdateScreen();
  HAL_Delay(50);

  ssd1306_Fill(Black);
  ssd1306_UpdateScreen();
  HAL_Delay(50);

  current_screen = 0;
  menu_index = 0;
  OLED_ShowMainMenu();

  while (1)
  {
    Fan_SoftPWM_Update();

    if (HAL_UART_Receive(&huart2, &rx_char, 1, 1) == HAL_OK)
    {
      if (rx_char == '\n' || rx_char == '\r')
      {
        rx_buffer[rx_index] = '\0';

        int value = 0;

        if (strcmp(rx_buffer, "UP") == 0)
        {
          if (current_screen == 0)
          {
            menu_index--;

            if (menu_index < 0)
            {
              menu_index = 2;
            }

            OLED_ShowMainMenu();
          }
        }
        else if (strcmp(rx_buffer, "DOWN") == 0)
        {
          if (current_screen == 0)
          {
            menu_index++;

            if (menu_index > 2)
            {
              menu_index = 0;
            }

            OLED_ShowMainMenu();
          }
        }
        else if (strcmp(rx_buffer, "SELECT") == 0)
        {
          if (current_screen == 0)
          {
            if (menu_index == 0)
            {
              current_screen = 1;
              OLED_ShowLightsMenu();
            }
            else if (menu_index == 1)
            {
              current_screen = 2;
              OLED_ShowFanScreen();
            }
            else if (menu_index == 2)
            {
              current_screen = 3;
              OLED_ShowDoorScreen();
            }
          }
        }
        else if (strcmp(rx_buffer, "BACK") == 0)
        {
          if (current_screen == 1 || current_screen == 2 || current_screen == 3)
          {
            current_screen = 0;
            menu_index = 0;
            OLED_ShowMainMenu();
          }
        }
        else if (sscanf(rx_buffer, "FAN:%d", &value) == 1)
        {
          Fan_SetSpeed(value);
        }
        else if (strcmp(rx_buffer, "DOOR:1") == 0 || strcmp(rx_buffer, "1") == 0)
        {
          Door_Close();
        }
        else if (strcmp(rx_buffer, "DOOR:0") == 0 || strcmp(rx_buffer, "2") == 0)
        {
          Door_Open();
        }
        else if (sscanf(rx_buffer, "LIGHT:%d", &value) == 1)
        {
          if (current_screen == 1 && value >= 1 && value <= 3)
          {
            active_light = value;
            OLED_ShowLightsMenu();
          }
        }
        else if (sscanf(rx_buffer, "L1:%d", &value) == 1)
        {
          if (value < 0)
          {
            value = 0;
          }

          if (value > 999)
          {
            value = 999;
          }

          light1 = value;
          __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, light1);
        }
        else if (sscanf(rx_buffer, "L2:%d", &value) == 1)
        {
          if (value < 0)
          {
            value = 0;
          }

          if (value > 999)
          {
            value = 999;
          }

          light2 = value;
          __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, light2);
        }
        else if (sscanf(rx_buffer, "L3:%d", &value) == 1)
        {
          if (value < 0)
          {
            value = 0;
          }

          if (value > 999)
          {
            value = 999;
          }

          light3 = value;
          __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, light3);
        }

        rx_index = 0;
        memset(rx_buffer, 0, sizeof(rx_buffer));
      }
      else
      {
        if (rx_index < (int)sizeof(rx_buffer) - 1)
        {
          rx_buffer[rx_index++] = rx_char;
        }
      }
    }
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  __HAL_RCC_TIM1_CLK_ENABLE();

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 7;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 19999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_ENABLE;

  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim1);
}

static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 79;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim2);
}

static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 79;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim3);
}

static void MX_TIM16_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 0;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 65535;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim16, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;

  if (HAL_TIMEx_ConfigBreakDeadTime(&htim16, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*
   * OLED I2C pins:
   * PB6 = I2C1_SCL
   * PB7 = I2C1_SDA
   */
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*
   * Door servo PWM pin:
   * D9 = PA8 = TIM1_CH1
   */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_TIM1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*
   * Fan control pin:
   * A6 = PA7
   */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
}

void Error_Handler(void)
{
  __disable_irq();

  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
