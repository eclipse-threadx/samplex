/* 
 * Copyright (c) 2026 STMicroelectronics
 * Copyright (c) 2026 Eclipse ThreadX contributors
 * 
 *  This program and the accompanying materials are made available 
 *  under the terms of the MIT license which is available at
 *  https://opensource.org/license/mit.
 * 
 *  SPDX-License-Identifier: MIT
 * 
 *  Contributors:
 *     Ali Eissa - 2026 version.
 */

#include "board_init.h"
#include "tx_api.h"
#include <string.h>
#include <stdio.h>

/* Global UART handler */
UART_HandleTypeDef huart3;

/* Global Ethernet handle */
ETH_HandleTypeDef heth;

/* Place the Ethernet DMA descriptors in the .nx_data section (uncacheable memory) */
__attribute__((section(".RxDecripSection"))) ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT];
__attribute__((section(".TxDecripSection"))) ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT];

void SystemClock_Config(void);
void MPU_Config(void);
void MX_GPIO_Init(void);
void MX_USART3_UART_Init(void);

/**
  * @brief  Initializes the board clocks, MPU, GPIOs, and serial console.
  *         Direct copy of the STM32CubeMX initialization sequence.
  * @retval None
  */
void board_init(void)
{
    /* Configure the Memory Protection Unit */
    MPU_Config();

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock to 216 MHz */
    SystemClock_Config();

    /* Initialize base GPIO pins */
    MX_GPIO_Init();

    /* Initialize USART3 console */
    MX_USART3_UART_Init();
}

/**
  * @brief System Clock Configuration (216 MHz using ST-LINK 8 MHz HSE bypass)
  *        Un-modified copy of STM32CubeMX generated Clock Configuration.
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART3 Initialization Function
  *
  * @retval None
  */
void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  *
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief MPU Configuration
  *
  * @retval None
  */
void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU for the NetXDuo/Ethernet DMA buffers in SRAM2 (0x20060000) */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x20060000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&heth);
}



void board_ethernet_init(void)
{
    /* Unique MAC address generation based on STM32 96-bit Unique ID (UID) */
    static uint8_t MACAddr[6];
    uint32_t uid0 = HAL_GetUIDw0();
    
    MACAddr[0] = 0x02; /* Locally administered unicast MAC address */
    MACAddr[1] = 0x00;
    MACAddr[2] = (uint8_t)(uid0 >> 24);
    MACAddr[3] = (uint8_t)(uid0 >> 16);
    MACAddr[4] = (uint8_t)(uid0 >> 8);
    MACAddr[5] = (uint8_t)(uid0);

    heth.Instance = ETH;
    heth.Init.MACAddr = MACAddr;
    heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
    heth.Init.TxDesc = DMATxDscrTab;
    heth.Init.RxDesc = DMARxDscrTab;
    heth.Init.RxBuffLen = 1524;

    printf("[HAL] Calling HAL_ETH_Init...\r\n");
    HAL_StatusTypeDef status = HAL_ETH_Init(&heth);
    printf("[HAL] HAL_ETH_Init returned status: %d\r\n", status);

    /* Initialize the PHY transceiver and wait for the link to be established.
       This ensures that when NetX Duo enables the interface during startup,
       the hardware link is already up, avoiding driver initialization failures. */
    extern int32_t nx_eth_phy_init(void);
    extern int32_t nx_eth_phy_get_link_state(void);
    printf("[NetX] Initializing PHY transceiver...\r\n");
    if (nx_eth_phy_init() == 0)
    {
        printf("[NetX] Waiting for Ethernet link (max 3s)...\r\n");
        uint32_t retries = 300; /* 300 * 10ms = 3 seconds */
        while (nx_eth_phy_get_link_state() <= 1)
        {
            HAL_Delay(10);
            retries--;
            if (retries == 0)
            {
                printf("[NetX] Ethernet link timeout! Cable connected?\r\n");
                break;
            }
        }
        if (nx_eth_phy_get_link_state() > 1)
        {
            printf("[NetX] Ethernet link up!\r\n");
        }
    }
    else
    {
        printf("[NetX] Failed to initialize PHY transceiver!\r\n");
    }
}

/* Override the weak HAL_GetTick function to provide a working tick source
   both before and after the ThreadX scheduler starts. */
uint32_t HAL_GetTick(void)
{
    /* If the ThreadX scheduler is running, use the ThreadX time */
    if (tx_thread_identify() != TX_NULL)
    {
        return (uint32_t)tx_time_get();
    }
    else
    {
        /* Return actual elapsed milliseconds based on SysTick hardware counter wrap-around.
           Since SysTick counts down from LOAD to 0, a wrap-around occurs when the current 
           value is greater than the last checked value, or if COUNTFLAG is set. */
        static uint32_t last_val = 0;
        static uint32_t ms_ticks = 0;
        
        uint32_t ctrl = SysTick->CTRL;
        uint32_t val = SysTick->VAL;
        
        if ((ctrl & SysTick_CTRL_COUNTFLAG_Msk) || (val > last_val))
        {
            ms_ticks++;
        }
        last_val = val;
        return ms_ticks;
    }
}
