/*
 * halm/platform/stm32/stm32f0xx/exti_base.h
 * Copyright (C) 2021 xent
 * Project is distributed under the terms of the MIT License
 */

#ifndef HALM_PLATFORM_STM32_EXTI_BASE_H_
#error This header should not be included directly
#endif

#ifndef HALM_PLATFORM_STM32_STM32F0XX_EXTI_BASE_H_
#define HALM_PLATFORM_STM32_STM32F0XX_EXTI_BASE_H_
/*----------------------------------------------------------------------------*/
enum ExtiEvent
{
  EXTI_PIN0,
  EXTI_PIN1,
  EXTI_PIN2,
  EXTI_PIN3,
  EXTI_PIN4,
  EXTI_PIN5,
  EXTI_PIN6,
  EXTI_PIN7,
  EXTI_PIN8,
  EXTI_PIN9,
  EXTI_PIN10,
  EXTI_PIN11,
  EXTI_PIN12,
  EXTI_PIN13,
  EXTI_PIN14,
  EXTI_PIN15,
  EXTI_PVD,
  EXTI_RTC_ALARM,
  EXTI_USB_WAKEUP,
  EXTI_RTC_TAMPER_TIMESTAMP,
  EXTI_RTC_WAKEUP,
  EXTI_COMPARATOR1,
  EXTI_COMPARATOR2,
  EXTI_I2C1_WAKEUP,
  EXTI_RESERVED0,
  EXTI_USART1_WAKEUP,
  EXTI_USART2_WAKEUP,
  EXTI_CEC_WAKEUP,
  EXTI_USART3_WAKEUP,
  EXTI_RESERVED1,
  EXTI_RESERVED2,
  EXTI_COMPARATOR_VDDIO2,
  EXTI_EVENT_END
} __attribute__((packed));
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_STM32_STM32F0XX_EXTI_BASE_H_ */
