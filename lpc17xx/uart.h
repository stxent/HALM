/*
 * uart.h
 *
 *  Created on: Aug 27, 2012
 *      Author: xen
 */

#ifndef UART_H_
#define UART_H_
/*----------------------------------------------------------------------------*/
#include <stdint.h>
/*----------------------------------------------------------------------------*/
#include "LPC17xx.h"
/*----------------------------------------------------------------------------*/
#include "interface.h"
#include "queue.h"
#include "gpio.h"
/*----------------------------------------------------------------------------*/
struct UartConfig
{
  gpioKey rx;
  gpioKey tx;
  uint8_t channel;
  uint32_t rate;
};
/*----------------------------------------------------------------------------*/
enum ifResult uartInit(struct Interface *, const void *);
/*----------------------------------------------------------------------------*/
#endif /* UART_H_ */
