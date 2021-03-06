/**
  ******************************************************************************
  * @file    gpio.h
  * @author  zju.nesc.iotlab 浙江大学NeSC课题组物联网实验室
  * @version V1.0
  * @date    13-March-2019
  * @brief   GPIO configuration functions   GPIO配置方法
  ******************************************************************************
  */

#ifndef __GPIO_H
#define __GPIO_H

//-------------include---------------//
#include "sys.h"
#include "config.h"

/** 
  * @brief  子板LED端口定义
  * @{
  */ 
#ifdef HARDWARE_VERSION_V1
	#define LED1_PORT    GPIOC
	#define LED1_PIN     GPIO_Pin_0
	#define LED2_PORT    GPIOD
	#define LED2_PIN     GPIO_Pin_2
	#define LED3_PORT    GPIOB
	#define LED3_PIN     GPIO_Pin_3

	#define LED1         PCout(0)
	//!LED1关闭
	#define LED1_OFF(x)  (LED1 = x) 
	//!LED1取反
	#define LED1_CONV()  (LED1 = ~LED1)

	#define LED2         PDout(2)
	//!LED2关闭
	#define LED2_OFF(x)  (LED2 = x)
	//!LED2取反
	#define LED2_CONV()  (LED2 = ~LED2)

	#define LED3         PBout(3)
	//!LED3关闭
	#define LED3_OFF(x)  (LED3 = x)
	//!LED3取反
	#define LED3_CONV()  (LED3 = ~LED3)

	//!测试灯
	#define INFO_LED_CONV()    LED1_CONV()
	#define INFO_LED_OFF(x)    LED1_OFF(x)
	//!工作灯
	#define WORKING_LED_CONV() LED2_CONV()
	#define WORKING_LED_OFF(x) LED2_OFF(x)
	//!nodeId灯
	#define ID_LED_CONV()      LED3_CONV()
	#define ID_LED_OFF(x)      LED3_OFF(x)
#elif defined(HARDWARE_VERSION_V3)
	#define LED1_PORT    GPIOB
	#define LED1_PIN     GPIO_Pin_6
	#define LED2_PORT    GPIOB
	#define LED2_PIN     GPIO_Pin_7
	#define LED3_PORT    GPIOB
	#define LED3_PIN     GPIO_Pin_5

	#define LED1         PBout(6)
	//!LED1关闭
	#define LED1_OFF(x)  (LED1 = x) 
	//!LED1取反
	#define LED1_CONV()  (LED1 = ~LED1)

	#define LED2         PBout(7)
	//!LED2关闭
	#define LED2_OFF(x)  (LED2 = x)
	//!LED2取反
	#define LED2_CONV()  (LED2 = ~LED2)

	#define LED3         PBout(5)
	//!LED3关闭
	#define LED3_OFF(x)  (LED3 = x)
	//!LED3取反
	#define LED3_CONV()  (LED3 = ~LED3)

	//!测试灯
	#define INFO_LED_CONV()    LED1_CONV()
	#define INFO_LED_OFF(x)    LED1_OFF(x)
	//!工作灯
	#define WORKING_LED_CONV() LED2_CONV()
	#define WORKING_LED_OFF(x) LED2_OFF(x)
	//!nodeId灯
	#define ID_LED_CONV()      LED3_CONV()
	#define ID_LED_OFF(x)      LED3_OFF(x)
#endif

/**
  * @}
  */

/** 
  * @brief  蜂鸣器定义
  * @{
  */ 
#ifdef   HARDWARE_VERSION_V1
	#define BEEP_PORT           GPIOC
	#define BEEP_PIN            GPIO_Pin_1

	#define BEEP                PCout(1)
	#define BEEP_ON(x)          (BEEP = x)
	#define BEEP_CONV()         (BEEP = ~BEEP)
#elif defined(HARDWARE_VERSION_V3)
	#define BEEP_PORT           GPIOB
	#define BEEP_PIN            GPIO_Pin_1

	#define BEEP                PBout(1)
	#define BEEP_ON(x)          (BEEP = x)
	#define BEEP_CONV()         (BEEP = ~BEEP)
#endif
/**
  * @}
  */
  
/** 
  * @brief  电源通断芯片shutdown端口定义（低电平有效）
  * @{
  */ 
#define V24V_SHUTOWN_PORT   GPIOB
#define V24V_SHUTOWN_PIN    GPIO_Pin_14
//!传感器24V电源。x=1：关闭电源；x=0，开启电源
#define V24V_SHUTOWN(x)     PBout(14)=~x
/**
  * @}
  */

/** 
  * @brief  模拟信号输入模式选择
  * @{
  */
#define CH1_PORT          GPIOA
#define CH1_PIN           GPIO_Pin_15
#define CH2_PORT          GPIOC
#define CH2_PIN           GPIO_Pin_10
#define CH3_PORT          GPIOC
#define CH3_PIN           GPIO_Pin_12
#define CH4_PORT          GPIOC
#define CH4_PIN           GPIO_Pin_11
#define Channel1          PAout(15)
#define Channel2          PCout(10)
#define Channel3          PCout(12)
#define Channel4          PCout(11)
#define ADC_CURRENT_MODE  0
#define ADC_VOLTAGE_MODE  1
/**
  * @}
  */


/** 
  * @brief  数字信号电平检测
  * @{
  */
#ifdef   HARDWARE_VERSION_V1
	#define DIG1_PORT       GPIOA
	#define DIG1_PIN        GPIO_Pin_1
	#define DIG2_PORT       GPIOA
	#define DIG2_PIN        GPIO_Pin_0
	#define DIGITAL_INPUT1  ((~GPIO_ReadInputDataBit(DIG1_PORT,DIG1_PIN)) & 0x01)//电路中会把高读取为0，低读取为1，故取反
	#define DIGITAL_INPUT2  ((~GPIO_ReadInputDataBit(DIG2_PORT,DIG2_PIN)) & 0x01)
#elif defined(HARDWARE_VERSION_V3)
	#define DIG1_PORT       GPIOB
	#define DIG1_PIN        GPIO_Pin_3
	#define DIG2_PORT       GPIOD
	#define DIG2_PIN        GPIO_Pin_2
	#define DIGITAL_INPUT1  ((~GPIO_ReadInputDataBit(DIG1_PORT,DIG1_PIN)) & 0x01)//电路中会把高读取为0，低读取为1，故取反
	#define DIGITAL_INPUT2  ((~GPIO_ReadInputDataBit(DIG2_PORT,DIG2_PIN)) & 0x01)
#endif
/**
  * @}
  */

void GP_IO_Init(void);//初始化	
void setAdcModel(u8 modle[]);
#endif
