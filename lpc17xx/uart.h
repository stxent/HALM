/*
 * uart.h
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef UART_H_
#define UART_H_
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
#include <LPC17xx.h>
#include "gpio.h"
#include "interface.h"
/*----------------------------------------------------------------------------*/
struct UartClass;
/*----------------------------------------------------------------------------*/
extern const struct UartClass *Uart;
/*----------------------------------------------------------------------------*/
enum uartParity
{
  UART_PARITY_NONE = 0,
  UART_PARITY_ODD,
  UART_PARITY_EVEN
};
/*----------------------------------------------------------------------------*/
/* TODO Stop bits, frame width, additional signals and modes */
struct UartConfig
{
  uint32_t rate; /* Mandatory: baud rate */
  gpioKey rx, tx; /* Mandatory: RX and TX pins */
  uint8_t channel; /* Mandatory: Peripheral number */
  enum uartParity parity; /* Optional: even, odd or no parity */
};
/*----------------------------------------------------------------------------*/
struct UartConfigRate
{
  uint8_t high, low, fraction;
};
/*----------------------------------------------------------------------------*/
struct UartClass
{
  struct InterfaceClass parent;

  /* Interrupt handler, arguments: UART descriptor assigned to peripheral */
  void (*handler)(void *);
};
/*----------------------------------------------------------------------------*/
struct Uart
{
  struct Interface parent;

  LPC_UART_TypeDef *reg; /* Pointer to UART registers */
  IRQn_Type irq; /* IRQ identifier */

  struct Gpio rxPin, txPin; /* Peripheral pins */
  uint8_t channel; /* Peripheral number */
};
/*----------------------------------------------------------------------------*/
enum result uartCalcRate(struct UartConfigRate *, uint32_t);
enum result uartSetDescriptor(uint8_t, void *);
void uartSetRate(struct Uart *, struct UartConfigRate);
/*----------------------------------------------------------------------------*/
#endif /* UART_H_ */
