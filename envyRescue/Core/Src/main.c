/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

const uint8_t ID_READ[3] = {0x9F, 0x00, 0x00};
const uint8_t READ_STATUS = 0x05;
const uint8_t READ_DATA = 0x03;
const uint8_t WRITE_EN = 0x06;
const uint8_t WRITE_DIS = 0x04;
const uint8_t CHIP_ERASE = 0x60;
const uint8_t PAGE_PROG = 0x02;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
char uart_buf[0x100] = {0x00};
uint16_t uart_len = 0;
uint8_t cdc_recv_flag = 0;


void cdc_recv()
{
	while (cdc_recv_flag != 1);
	cdc_recv_flag = 0;
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */
	char uart_sel = '0';
	int uart_buf_len;
	char spi_buf[0x100] = {0};
	uint8_t addr;
	uint8_t wip;
	uint8_t usb_res = 0;

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_SPI1_Init();
	MX_USB_DEVICE_Init();
	/* USER CODE BEGIN 2 */

	// CS pin should default high
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		cdc_recv();
		uart_sel = uart_buf[0];

		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

		switch(uart_sel) {
		case 'i': // Read RDID register
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
			HAL_SPI_Transmit(&hspi1, (uint8_t *)&ID_READ, 1, 100);
			HAL_SPI_Receive(&hspi1, (uint8_t *)spi_buf, 3, 100);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

			usb_res = CDC_Transmit_FS((uint8_t *)spi_buf, 3);
			//	      HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
			break;
		case 's': // Read STATUS register
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
			HAL_SPI_Transmit(&hspi1, (uint8_t *)&READ_STATUS, 1, 100);
			HAL_SPI_Receive(&hspi1, (uint8_t *)spi_buf, 1, 100);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

			uart_buf_len = sprintf(uart_buf, "%c", spi_buf[0]);
			usb_res = CDC_Transmit_FS((uint8_t *)uart_buf, uart_buf_len);
			//			  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
			break;
		case 'd': // Dump all data
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
			uint8_t cmd[4] = {0x00};
			cmd[0] = READ_DATA;
			HAL_SPI_Transmit(&hspi1, (uint8_t *)&cmd, 4, 100);
			unsigned char next_in;

			//			HAL_UART_Receive(&huart1, &next_in, 1, 300);
			uart_buf_len = sprintf(uart_buf, "d");
			usb_res = CDC_Transmit_FS((uint8_t *)uart_buf, uart_buf_len);
			cdc_recv();
			next_in = uart_buf[0];

			if (next_in == 'n') {
				while(next_in == 'n')
				{
					next_in = 'z'; // Ensure that the next command actually arrives, instead of being left over.
					HAL_SPI_Receive(&hspi1, (uint8_t *)spi_buf, 0x1, 800);
					//		uart_buf_len = sprintf(uart_buf, "DATA: %d \t%x\r\n", i, spi_buf[i]);
					//					HAL_UART_Transmit(&huart1, (uint8_t *)spi_buf, 0x100, 800);
					//					HAL_UART_Receive(&huart1, &next_in, 1, 500);
					usb_res = CDC_Transmit_FS((uint8_t *)spi_buf, 0x1);
					cdc_recv();
					next_in = uart_buf[0];
				}
			}
			else if (next_in == 'a') {
				for (uint32_t i = 0; i < (0x800000/0x100); i++) {
					HAL_SPI_Receive(&hspi1, (uint8_t *)spi_buf, 0x100, 800);
					//					HAL_UART_Transmit(&huart1, (uint8_t *)spi_buf, 0x100, 800);
					usb_res = CDC_Transmit_FS((uint8_t *)spi_buf, 0x100);
				}
			}
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
			break;
		case 'e': // Chip erase
			// Set the Write Enable latch
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
			HAL_SPI_Transmit(&hspi1, (uint8_t *)&WRITE_EN, 1, 100);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

			// Send the Chip Erase command
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
			HAL_SPI_Transmit(&hspi1, (uint8_t *)&CHIP_ERASE, 1, 100);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

			// Wait for the Write in Progress bit to be cleared
			do {
				HAL_Delay(10);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
				HAL_SPI_Transmit(&hspi1, (uint8_t *)&READ_STATUS, 1, 100);
				HAL_SPI_Receive(&hspi1, (uint8_t *)spi_buf, 1, 100);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
			} while (spi_buf[0] != 0x40);

			uart_buf_len = sprintf(uart_buf, "e");
			usb_res = CDC_Transmit_FS((uint8_t *)uart_buf, uart_buf_len);
			break;
		case 'p': // Program chip

			// Ack that command was recv'ed
			uart_buf_len = sprintf(uart_buf, "p");
			usb_res = CDC_Transmit_FS((uint8_t *)uart_buf, uart_buf_len);

			uint8_t addr3 = 0;
			uint8_t addr2 = 0;
//			uint8_t addr1 = 0;
			const uint8_t addr1 = 0x00;
			uint8_t page[0x100] = {0x00};
			unsigned char next = '\0';

			// Recv if another page is coming
			cdc_recv();
			next = uart_buf[0];
			usb_res = CDC_Transmit_FS(&next, 1); // Ack

			do {
				// Recv Addr byte 3
				cdc_recv();
				addr3 = uart_buf[0];
				usb_res = CDC_Transmit_FS(&addr3, 1); // Ack

				// Recv Addr byte 2
				cdc_recv();
				addr2 = uart_buf[0];
				usb_res = CDC_Transmit_FS(&addr2, 1); // Ack

				// Recv Addr byte 1
//				cdc_recv();
//				addr1 = uart_buf[0];
//				usb_res = CDC_Transmit_FS(&addr1, 1); // Ack

				// Gather page of data from host
				for (uint16_t i = 0; i < 0x100; i++) {
					cdc_recv();
					page[i] = uart_buf[0];
					usb_res = CDC_Transmit_FS(&page[i], 1); // Ack
				}

				// Gather one byte from host
//				cdc_recv();
//				page[0] = uart_buf[0];
//				usb_res = CDC_Transmit_FS(page, 1); // Ack

				// Set the Write Enable latch
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
				HAL_SPI_Transmit(&hspi1, (uint8_t *)&WRITE_EN, 1, 100);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

				// Program chip
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
				HAL_SPI_Transmit(&hspi1, (uint8_t *)&PAGE_PROG, 1, 100);
				HAL_SPI_Transmit(&hspi1, (uint8_t *)&addr3, 1, 100);
				HAL_SPI_Transmit(&hspi1, (uint8_t *)&addr2, 1, 100);
				HAL_SPI_Transmit(&hspi1, (uint8_t *)&addr1, 1, 100);
				HAL_SPI_Transmit(&hspi1, (uint8_t *)page, 0x100, 500);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

				// Ack that page was sent
				uart_buf_len = sprintf(uart_buf, "s");
				usb_res = CDC_Transmit_FS((uint8_t *)uart_buf, uart_buf_len);

				// Wait for the Write in Progress bit to be cleared
				do {
					HAL_Delay(20);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
					HAL_SPI_Transmit(&hspi1, (uint8_t *)&READ_STATUS, 1, 100);
					HAL_SPI_Receive(&hspi1, (uint8_t *)spi_buf, 1, 100);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
				} while (spi_buf[0] != 0x40);

				// Ack that command completed
				uart_buf_len = sprintf(uart_buf, "d");
				usb_res = CDC_Transmit_FS((uint8_t *)uart_buf, uart_buf_len);

				// Recv if another page is coming
				cdc_recv();
				next = uart_buf[0];
				usb_res = CDC_Transmit_FS(&next, 1); // Ack
			} while (next == 'n');

			break;
		default:
			//		  uart_buf_len = sprintf(uart_buf, "Bad input\n");
			//		  HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, uart_buf_len, 100);
			break;

		}


		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
	PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{

	/* USER CODE BEGIN SPI1_Init 0 */

	/* USER CODE END SPI1_Init 0 */

	/* USER CODE BEGIN SPI1_Init 1 */

	/* USER CODE END SPI1_Init 1 */
	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

	/*Configure GPIO pin : PC13 */
	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : PA4 */
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
