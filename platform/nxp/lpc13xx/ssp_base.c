/*
 * ssp_base.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <macro.h>
#include <platform/nxp/ssp_base.h>
#include "platform/nxp/lpc13xx/clocking.h"
#include "platform/nxp/lpc13xx/power.h"
/*----------------------------------------------------------------------------*/
/* SSP clock divisor is the number from 1 to 255 or 0 to disable */
#define DEFAULT_DIV       1
#define DEFAULT_DIV_VALUE 1
/*----------------------------------------------------------------------------*/
//FIXME Move to header
#define PRESETCTRL_SSP0 BIT(0)
#define PRESETCTRL_SSP1 BIT(2)
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(uint8_t, struct SspBase *);
/*----------------------------------------------------------------------------*/
static enum result sspInit(void *, const void *);
static void sspDeinit(void *);
/*----------------------------------------------------------------------------*/
static struct SspBase *descriptors[] = {0, 0};
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
/* SSP1 peripheral available only on LPC1313 */
const struct GpioDescriptor sspPins[] = {
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
        .key = 0 /* End of pin function association list */
    }
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass *SspBase = &sspTable;
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(uint8_t channel, struct SspBase *interface)
{
  assert(channel < sizeof(descriptors));

  if (descriptors[channel])
    return E_BUSY;

  descriptors[channel] = interface;
  return E_OK;
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
uint32_t sspGetClock(struct SspBase *interface __attribute__((unused)))
{
  return sysCoreClock / DEFAULT_DIV_VALUE;
}
/*----------------------------------------------------------------------------*/
static enum result sspInit(void *object, const void *configPtr)
{
  const struct SspBaseConfig * const config = configPtr;
  struct SspBase *interface = object;
  enum result res;

  /* Try to set peripheral descriptor */
  interface->channel = config->channel;
  if ((res = setDescriptor(interface->channel, interface)) != E_OK)
    return res;

  if ((res = sspSetupPins(interface, config)) != E_OK)
    return res;

  interface->handler = 0;

  switch (interface->channel)
  {
    case 0:
      sysClockEnable(CLK_SSP0);
      LPC_SYSCON->SSP0CLKDIV = DEFAULT_DIV;
      LPC_SYSCON->PRESETCTRL |= PRESETCTRL_SSP0;
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
      LPC_SYSCON->SSP1CLKDIV = DEFAULT_DIV;
      LPC_SYSCON->PRESETCTRL |= PRESETCTRL_SSP1;
      interface->reg = LPC_SSP1;
      interface->irq = SSP1_IRQ;
      break;
  }

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void sspDeinit(void *object)
{
  struct SspBase *interface = object;

  /* Put peripheral in reset and disable clock */
  switch (interface->channel)
  {
    case 0:
      LPC_SYSCON->PRESETCTRL &= ~PRESETCTRL_SSP0;
      LPC_SYSCON->SSP0CLKDIV = 0;
      sysClockDisable(CLK_SSP0);
      break;
    case 1:
      LPC_SYSCON->PRESETCTRL &= ~PRESETCTRL_SSP1;
      LPC_SYSCON->SSP1CLKDIV = 0;
      sysClockDisable(CLK_SSP1);
      break;
  }

  /* Release interface pins */
//  gpioDeinit(&interface->csPin);
  gpioDeinit(&interface->mosiPin);
  gpioDeinit(&interface->misoPin);
  gpioDeinit(&interface->sckPin);

  /* Reset SSP descriptor */
  setDescriptor(interface->channel, 0);
}