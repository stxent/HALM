/*
 * ssp.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include "platform/nxp/ssp.h"
#include "platform/nxp/ssp_defs.h"
#include "platform/nxp/system.h"
#include "platform/nxp/lpc13xx/interrupts.h"
#include "platform/nxp/lpc13xx/power.h"
/*----------------------------------------------------------------------------*/
/* In LPC13xx SSP clock divisor is number from 1 to 255, 0 to disable */
#define DEFAULT_DIV       1
#define DEFAULT_DIV_VALUE 1
/*----------------------------------------------------------------------------*/
/* SSP1 peripheral available only on LPC1313 */
static const struct GpioDescriptor sspPins[] = {
    {
        .key = GPIO_TO_PIN(0, 2), /* SSP0_SSEL */
        .channel = 0,
        .value = 1
    }, {
        .key = GPIO_TO_PIN(0, 6), /* SSP0_SCK */
        .channel = 0,
        .value = 2
    }, {
        .key = GPIO_TO_PIN(0, 8), /* SSP0_MISO */
        .channel = 0,
        .value = 1
    }, {
        .key = GPIO_TO_PIN(0, 9), /* SSP0_MOSI */
        .channel = 0,
        .value = 1
    }, {
        .key = GPIO_TO_PIN(0, 10), /* SSP0_SCK */
        .channel = 0,
        .value = 2
    }, {
        .key = GPIO_TO_PIN(2, 0), /* SSP1_SSEL */
        .channel = 1,
        .value = 2
    }, {
        .key = GPIO_TO_PIN(2, 1), /* SSP1_SCK */
        .channel = 1,
        .value = 2
    }, {
        .key = GPIO_TO_PIN(2, 2), /* SSP1_MISO */
        .channel = 1,
        .value = 2
    }, {
        .key = GPIO_TO_PIN(2, 3), /* SSP1_MOSI */
        .channel = 1,
        .value = 2
    }, {
        .key = GPIO_TO_PIN(2, 11), /* SSP0_SCK */
        .channel = 0,
        .value = 1
    }, {
        /* End of pin function association list */
    }
};
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(uint8_t, struct Ssp *);
static inline void setupPins(struct Ssp *, const struct SspConfig *);
/*----------------------------------------------------------------------------*/
static enum result sspInit(void *, const void *);
static void sspDeinit(void *);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass sspTable = {
    .size = 0, /* Abstract class */
    .init = sspInit,
    .deinit = sspDeinit,

    .callback = 0,
    .get = 0,
    .set = 0,
    .read = 0,
    .write = 0
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass *Ssp = &sspTable;
/*----------------------------------------------------------------------------*/
static struct Ssp *descriptors[] = {0, 0};
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(uint8_t channel, struct Ssp *interface)
{
  assert(channel < sizeof(descriptors));

  if (descriptors[channel])
    return E_BUSY;

  descriptors[channel] = interface;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static inline void setupPins(struct Ssp *interface,
    const struct SspConfig *config)
{
  const struct GpioDescriptor *pin;

  /* Setup Serial Clock pin */
  pin = gpioFind(sspPins, config->sck, interface->channel);
  assert(pin);
  interface->sckPin = gpioInit(config->sck, GPIO_OUTPUT);
  gpioSetFunction(&interface->sckPin, pin->value);

  /* Setup MISO pin */
  pin = gpioFind(sspPins, config->miso, interface->channel);
  assert(pin);
  interface->misoPin = gpioInit(config->miso, GPIO_INPUT);
  gpioSetFunction(&interface->misoPin, pin->value);

  /* Setup MOSI pin */
  pin = gpioFind(sspPins, config->mosi, interface->channel);
  assert(pin);
  interface->mosiPin = gpioInit(config->mosi, GPIO_OUTPUT);
  gpioSetFunction(&interface->mosiPin, pin->value);

  /* Setup CS pin, only in slave mode */
//  pin = gpioFind(sspPins, config->cs, interface->channel);
//  assert(pin);
//  interface->csPin = gpioInit(config->cs, GPIO_INPUT);
//  gpioSetFunction(&interface->csPin, pin->value);
}
/*----------------------------------------------------------------------------*/
void SSP0_ISR(void)
{
  if (descriptors[0])
    descriptors[0]->handler(descriptors[0]);
}
/*----------------------------------------------------------------------------*/
void SSP1_ISR(void)
{
  if (descriptors[1])
    descriptors[1]->handler(descriptors[1]);
}
/*----------------------------------------------------------------------------*/
void sspSetRate(struct Ssp *interface, uint32_t rate)
{
  uint16_t divider;

  divider = ((sysCoreClock / DEFAULT_DIV_VALUE) >> 1) / rate - 1;
  /* FIXME Rewrite */
  interface->reg->CPSR = 2;
  interface->reg->CR0 &= ~CR0_SCR_MASK;
  interface->reg->CR0 |= CR0_SCR(divider);
}
/*----------------------------------------------------------------------------*/
uint32_t sspGetRate(struct Ssp *interface)
{
  uint32_t rate;
  uint16_t divider;

  divider = CR0_SCR_VALUE(interface->reg->CR0);
  rate = ((sysCoreClock / DEFAULT_DIV_VALUE) >> 1) / (divider + 1);
  return rate;
}
/*----------------------------------------------------------------------------*/
static enum result sspInit(void *object, const void *configPtr)
{
  const struct SspConfig * const config = configPtr;
  struct Ssp *interface = object;
  enum result res;

  /* Check interface configuration data */
  assert(config);

  /* Try to set peripheral descriptor */
  interface->channel = config->channel;
  if ((res = setDescriptor(interface->channel, interface)) != E_OK)
    return res;

  /* Configure interface pins */
  setupPins(interface, config);

  /* Reset pointer to interrupt handler */
  interface->handler = 0;

  switch (interface->channel)
  {
    case 0:
      sysClockEnable(CLK_SSP0);
      LPC_SYSCON->SSP0CLKDIV = DEFAULT_DIV; /* Divide AHB clock */
      LPC_SYSCON->PRESETCTRL = 1; /* FIXME */
      interface->reg = LPC_SSP0;
      interface->irq = SSP0_IRQ;

      /* Set SCK0 pin location register */
      switch (config->sck)
      {
        case GPIO_TO_PIN(0, 10):
          LPC_IOCON->SCK_LOC = 0;
          break;
        case GPIO_TO_PIN(2, 11):
          LPC_IOCON->SCK_LOC = 1;
          break;
        case GPIO_TO_PIN(0, 6):
          LPC_IOCON->SCK_LOC = 2;
          break;
      }
      break;
    case 1:
      sysClockEnable(CLK_SSP1);
      LPC_SYSCON->SSP1CLKDIV = DEFAULT_DIV; /* Divide AHB clock */
      LPC_SYSCON->PRESETCTRL = 4; /* FIXME */
      interface->reg = LPC_SSP1;
      interface->irq = SSP1_IRQ;
      break;
  }

  interface->reg->CR0 = 0;
  if (!config->frame)
    interface->reg->CR0 |= CR0_DSS(8);
  else
  {
    assert(config->frame >= 4 && config->frame <= 16);
    interface->reg->CR0 |= CR0_DSS(config->frame);
  }
  if (config->mode & 0x01) //TODO Remove magic numbers
    interface->reg->CR0 |= CR0_CPHA;
  if (config->mode & 0x02)
    interface->reg->CR0 |= CR0_CPOL;

  sspSetRate(interface, config->rate);
  interface->reg->CR1 = CR1_SSE; /* Enable peripheral */

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void sspDeinit(void *object)
{
  struct Ssp *interface = object;

  /* Disable UART peripheral power */
  switch (interface->channel)
  {
    case 0:
      LPC_SYSCON->PRESETCTRL = 0; /* FIXME */
      LPC_SYSCON->SSP0CLKDIV = 0;
      sysClockDisable(CLK_SSP0);
      break;
    case 1:
      LPC_SYSCON->PRESETCTRL = 0; /* FIXME */
      LPC_SYSCON->SSP1CLKDIV = 0;
      sysClockDisable(CLK_SSP1);
      break;
  }
//  gpioDeinit(&interface->csPin);
  gpioDeinit(&interface->mosiPin);
  gpioDeinit(&interface->misoPin);
  gpioDeinit(&interface->sckPin);
  /* Reset SSP descriptor */
  setDescriptor(interface->channel, 0);
}
