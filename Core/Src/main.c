/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Modbus.h"
#include "semphr.h"
#include "ds18b20.h"
#include "adc_sensor.h"

/* USER CODE END Includes */


/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

/* Definitions for Sensor_measurement_task */
osThreadId_t Sensor_measurement_Handle;
const osThreadAttr_t Sensor_measurement_attributes = {
  .name = "Sensor_measurement",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 128 * 4
};

/*Definition mutex for control read sensor sequence*/
SemaphoreHandle_t Sensor_Semaphore = NULL;


/* USER CODE BEGIN PV */
modbusHandler_t ModbusH;
uint16_t ModbusDATA[8];
/* USER CODE END PV */


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define Voltage_686  1000
#define  Voltage_401  2000

//For debug
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/*....................................Init function..................................*/
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


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}


static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}


static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RS485_GPIO_Port, RS485_Pin, GPIO_PIN_RESET);
  
  HAL_GPIO_WritePin(Pump_GPIO_Port, Pump_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(Valve_GPIO_Port, Valve_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : RS485_Pin */
  GPIO_InitStruct.Pin = RS485_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RS485_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Pump_Pin */
  GPIO_InitStruct.Pin = Pump_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Pump_GPIO_Port, &GPIO_InitStruct);
  
  /*Configure GPIO pin : Valve_Pin */
  GPIO_InitStruct.Pin = Valve_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Valve_GPIO_Port, &GPIO_InitStruct);

}


static void MX_ADC1_Init(void)
{

  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM3) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 72;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM1)
  {
  /* USER CODE BEGIN TIM1_MspInit 0 */

  /* USER CODE END TIM1_MspInit 0 */
    /* TIM1 clock enable */
    __HAL_RCC_TIM1_CLK_ENABLE();
  /* USER CODE BEGIN TIM1_MspInit 1 */

  /* USER CODE END TIM1_MspInit 1 */
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM1)
  {
  /* USER CODE BEGIN TIM1_MspDeInit 0 */

  /* USER CODE END TIM1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM1_CLK_DISABLE();
  /* USER CODE BEGIN TIM1_MspDeInit 1 */

  /* USER CODE END TIM1_MspDeInit 1 */
  }
} 

stm_err_t get_sensors_value(float* temperature, int* PH_value, int* ORP_value)
{
  /*Temperture read*/
  DS18B20_ReadAll();
  DS18B20_StartAll();
  if(!DS18B20_GetTemperature(0, temperature)) return STM_FAIL;
  
  /*PH read*/
  *PH_value = read_ph_sensor(adc_read_sensor_voltage(PH_sensor), Voltage_686, Voltage_401);
  
  /*ORP read*/
  *ORP_value = read_orp_sensor(adc_read_sensor_voltage(ORP_sensor));

  return STM_OK;
}

void write_data_to_modbus_register(float temperature, int PH_value, int ORP_value)
{
  xSemaphoreTake(ModbusH.ModBusSphrHandle , 100);
    if(temperature <50) ModbusDATA[2] = (int)(temperature*10);
    if((PH_value > 0) && (PH_value<=140)) ModbusDATA[3] = PH_value;
    if((ORP_value > 0) && (PH_value<=300)) ModbusDATA[4] = ORP_value;
  xSemaphoreGive(ModbusH.ModBusSphrHandle);
}


/*..........................FreeRTOS Task.............................*/

void Sensor_measurement(void *argument)
{
  osDelay(100);
  
  float temperature;
  int PH_value;
  int ORP_value;

  uint32_t interval;
  for(;;)
  {
    osDelay(100);
    xSemaphoreTake(Sensor_Semaphore , portMAX_DELAY);
    xSemaphoreGive(Sensor_Semaphore);
    
    /*Open valve and turn on pump to make the container full of water*/
    HAL_GPIO_WritePin(Valve_GPIO_Port, Valve_Pin, 1);
    HAL_GPIO_WritePin(Pump_GPIO_Port, Pump_Pin, 1);

    /*Delay 1 minutes*/
    osDelay(1*60*1000);
    
    /*Close valve and turn off pump*/
    HAL_GPIO_WritePin(Valve_GPIO_Port, Valve_Pin, 0);
    HAL_GPIO_WritePin(Pump_GPIO_Port, Pump_Pin, 0);

    while(get_sensors_value(&temperature, &PH_value, &ORP_value) == STM_FAIL)
    {
      printf("Get sensors value failed, retry...\n");
      osDelay(500);
    }

    write_data_to_modbus_register(temperature, PH_value, ORP_value);

    /*Open valve to release water and make the container empty*/
    HAL_GPIO_WritePin(Valve_GPIO_Port, Valve_Pin, 1);

    /*Delay 1 minutes*/
    osDelay(1*60*1000);
    
    /*Close valve*/
    HAL_GPIO_WritePin(Valve_GPIO_Port, Valve_Pin, 0);


    /*Delay and restart after the interval*/
    xSemaphoreTake(ModbusH.ModBusSphrHandle , 100);
    interval = ModbusDATA[1]*60*1000;
    xSemaphoreGive(ModbusH.ModBusSphrHandle);
    
    osDelay(interval);

  }
}

void StartDefaultTask(void *argument)
{
  for(;;)
  { 
    if(!ModbusDATA[0])
    {
      xSemaphoreTake(Sensor_Semaphore , portMAX_DELAY);
    
      while(!ModbusDATA[0]) osDelay(1);
      xSemaphoreGive(Sensor_Semaphore);
    }
    
    osDelay(1);
  }
}




/* USER CODE END 0 */


int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();


  MX_TIM1_Init();
  HAL_TIM_Base_Start(&htim1);

  ModbusH.uModbusType = MB_SLAVE;
  ModbusH.port =  &huart3;
  ModbusH.u8id = 3; //Modbus slave ID
  ModbusH.u16timeOut = 1000;
  ModbusH.EN_Port = RS485_GPIO_Port;
  ModbusH.EN_Pin = RS485_Pin;
  ModbusH.u16regs = ModbusDATA;
  ModbusH.u16regsize= sizeof(ModbusDATA)/sizeof(ModbusDATA[0]);
  ModbusH.xTypeHW = USART_HW;

  //Initialize Modbus RTU
  ModbusInit(&ModbusH);
  ModbusStart(&ModbusH);

  //Init DS18b20
  DS18B20_Init(DS18B20_Resolution_12bits);

  /* Init scheduler RTOS*/
  osKernelInitialize();

  /*Mutex for control reading sensor sequence*/
  Sensor_Semaphore = xSemaphoreCreateMutex();

  /*Time interval default*/
  ModbusDATA[1] = 1;


  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  Sensor_measurement_Handle = osThreadNew(Sensor_measurement, NULL, &Sensor_measurement_attributes);

  


  /* Start scheduler */
  osKernelStart();
}
