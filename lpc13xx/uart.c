/*
 * uart.c
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "lpc13xx_defs.h"
#include "uart.h"
#include "uart_defs.h"
#include "mutex.h"
/*----------------------------------------------------------------------------*/
/* UART settings */
#define FRACTION_VALUE                  15 /* Divisor from 1 to 15 */
/* In LPC13xx UART clock divider is value from 1 to 255, 0 to disable */
#define DEFAULT_DIV                     1
#define DEFAULT_DIV_VALUE               1
/*----------------------------------------------------------------------------*/
/* UART pin function values */
static const struct GpioPinFunc uartPins[] = {
    {
        .key = GPIO_TO_PIN(1, 6),
        .func = 0x01
    },
    {
        .key = GPIO_TO_PIN(1, 7),
        .func = 0x01
    },
    {
        .key = -1,
        .func = 0x00
    }
};
/*----------------------------------------------------------------------------*/
static enum result uartInit(struct Interface *, const void *);
static void uartDeinit(struct Interface *);
/*----------------------------------------------------------------------------*/
static const struct UartClass uartTable = {
    .parent = {
        .size = 0, /* Abstract class */
        .init = uartInit,
        .deinit = uartDeinit,

        .read = 0,
        .write = 0,
        .getopt = 0,
        .setopt = 0
    },
    .handler = 0
};
/*----------------------------------------------------------------------------*/
const struct UartClass *Uart = &uartTable;
/*----------------------------------------------------------------------------*/
static void * volatile descriptors[] = {0};
static Mutex lock = MUTEX_UNLOCKED;
/*----------------------------------------------------------------------------*/
enum result uartSetDescriptor(uint8_t channel, void *descriptor)
{
  enum result res = E_ERROR;

  if (channel < sizeof(descriptors))
  {
    mutexLock(&lock);
    if (!descriptors[channel])
    {
      descriptors[channel] = descriptor;
      res = E_OK;
    }
    mutexUnlock(&lock);
  }
  return res;
}
/*----------------------------------------------------------------------------*/
void UART_IRQHandler(void)
{
  if (descriptors[0])
    ((struct UartClass *)CLASS(descriptors[0]))->handler(descriptors[0]);
}
/*----------------------------------------------------------------------------*/
struct UartConfigRate uartCalcRate(uint32_t rate)
{
  uint32_t divisor, reminder;
  struct UartConfigRate result = {
      .high = 0,
      .low = 0,
      .fraction = 0
  };

  divisor = (SystemCoreClock / DEFAULT_DIV_VALUE) >> 4;
  reminder = divisor * FRACTION_VALUE / rate;
  divisor /= rate;
  reminder -= divisor * FRACTION_VALUE;
  if (!(divisor >= (1 << 16) || !divisor))
  {
    result.high = (uint8_t)(divisor >> 8);
    result.low = (uint8_t)divisor;
//    if (result.high > 0 || result.low > 1)
//      result.fraction = (uint8_t)(reminder | (FRACTION_VALUE << 4));
  }
  return result;
}
/*----------------------------------------------------------------------------*/
enum result uartSetRate(struct Uart *device, struct UartConfigRate rate)
{
  if (!rate.high && !rate.low)
    return E_ERROR;
  /* Enable DLAB access */
  device->reg->LCR |= LCR_DLAB;
  /* Set divisor of the baud rate generator */
  device->reg->DLL = rate.low;
  device->reg->DLM = rate.high;
  /* Set fractional divisor */
  device->reg->FDR = rate.fraction;
  /* Disable DLAB access */
  device->reg->LCR &= ~LCR_DLAB;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void uartDeinit(struct Interface *iface)
{
  struct Uart *device = (struct Uart *)iface;

  /* Disable UART peripheral power */
  LPC_SYSCON->UARTCLKDIV = 0;
  LPC_SYSCON->SYSAHBCLKCTRL &= ~SYSAHBCLKCTRL_UART;
  /* Release pins */
  gpioDeinit(&device->txPin);
  gpioDeinit(&device->rxPin);
  /* Reset UART descriptor */
  uartSetDescriptor(device->channel, 0);
}
/*----------------------------------------------------------------------------*/
static enum result uartInit(struct Interface *iface, const void *configPtr)
{
  /* Set pointer to device configuration data */
  const struct UartConfig *config = (const struct UartConfig *)configPtr;
  struct Uart *device = (struct Uart *)iface;
  uint8_t rxFunc, txFunc;

  /* Check device configuration data and availability */
  if (!config || uartSetDescriptor(config->channel, device) != E_OK)
    return E_ERROR;

  /* Check pin mapping */
  rxFunc = gpioFindFunc(uartPins, config->rx);
  txFunc = gpioFindFunc(uartPins, config->tx);
  if (!rxFunc || !txFunc)
    return E_ERROR;

  /* Setup UART pins */
  device->rxPin = gpioInit(config->rx, GPIO_INPUT);
  if (gpioGetKey(&device->rxPin) == -1)
    return E_ERROR;

  device->txPin = gpioInit(config->tx, GPIO_OUTPUT);
  if (gpioGetKey(&device->txPin) == -1)
  {
    gpioDeinit(&device->rxPin);
    return E_ERROR;
  }

  device->channel = config->channel;

  switch (config->channel)
  {
    case 0:
      LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_UART;
      LPC_SYSCON->UARTCLKDIV = DEFAULT_DIV; /* Divide AHB clock */
      device->reg = LPC_UART;
      device->irq = UART_IRQn;
      break;
    default:
      break;
  }

  /* Select the UART function of pins */
  gpioSetFunc(&device->rxPin, rxFunc);
  gpioSetFunc(&device->txPin, txFunc);

  /* Set interrupt priority, lowest by default */
  NVIC_SetPriority(device->irq, GET_PRIORITY(config->priority));
  return E_OK;
}
