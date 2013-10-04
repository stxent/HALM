/*
 * spi.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include "platform/nxp/spi.h"
#include "platform/nxp/ssp_defs.h"
/*----------------------------------------------------------------------------*/
#define DEFAULT_PRIORITY  255 /* Lowest interrupt priority in Cortex-M3 */
#define FIFO_DEPTH        8
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *);
/*----------------------------------------------------------------------------*/
static enum result spiInit(void *, const void *);
static void spiDeinit(void *);
static enum result spiCallback(void *, void (*)(void *), void *);
static enum result spiGet(void *, enum ifOption, void *);
static enum result spiSet(void *, enum ifOption, const void *);
static uint32_t spiRead(void *, uint8_t *, uint32_t);
static uint32_t spiWrite(void *, const uint8_t *, uint32_t);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass spiTable = {
    .size = sizeof(struct Spi),
    .init = spiInit,
    .deinit = spiDeinit,

    .callback = spiCallback,
    .get = spiGet,
    .set = spiSet,
    .read = spiRead,
    .write = spiWrite
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass *Spi = &spiTable;
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *object)
{
  struct Spi *interface = object;
  LPC_SSP_TypeDef *reg = interface->parent.reg;
  uint32_t count;

  count = interface->length > FIFO_DEPTH ? FIFO_DEPTH : interface->length;

  if (interface->rxBuffer)
  {
    /* Read mode */
    while (reg->SR & SR_RNE)
    {
      --interface->left;
      *interface->rxBuffer++ = reg->DR;
    }

    while (count-- && reg->SR & SR_TNF)
    {
      reg->DR = 0xFF; /* TODO Select dummy frame value for SPI */
      --interface->length;
    }
  }
  else
  {
    /* Write mode */
    while (reg->SR & SR_RNE)
    {
      --interface->left;
      (void)reg->DR;
    }

    while (count-- && reg->SR & SR_TNF)
    {
      reg->DR = *interface->txBuffer++;
      --interface->length;
    }
  }

  if (!interface->left)
  {
    reg->IMSC &= ~(IMSC_RXIM | IMSC_RTIM);
    if (interface->callback)
      interface->callback(interface->callbackArgument);
  }
}
/*----------------------------------------------------------------------------*/
static enum result spiInit(void *object, const void *configPtr)
{
  const struct SpiConfig * const config = configPtr;
  const struct SspConfig parentConfig = {
      .channel = config->channel,
      .miso = config->miso,
      .mosi = config->mosi,
      .sck = config->sck,
      .cs = config->cs,
      .rate = config->rate,
      .frame = 8 /* Fixed frame size */
  };
  struct Spi *interface = object;
  enum result res;

  /* Call SSP class constructor */
  if ((res = Ssp->init(object, &parentConfig)) != E_OK)
    return res;

  interface->blocking = true;
  interface->callback = 0;
  interface->lock = SPIN_UNLOCKED;

  /* Set pointer to interrupt handler */
  interface->parent.handler = interruptHandler;

  /* Set interrupt priority, lowest by default */
  nvicSetPriority(interface->parent.irq, DEFAULT_PRIORITY);
  /* Enable SPI interrupt */
  nvicEnable(interface->parent.irq);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void spiDeinit(void *object)
{
  struct Spi *interface = object;

  nvicDisable(interface->parent.irq);
  Ssp->deinit(interface); /* Call SSP class destructor */
}
/*----------------------------------------------------------------------------*/
static enum result spiCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct Spi *interface = object;

  interface->callback = callback;
  interface->callbackArgument = argument;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result spiGet(void *object, enum ifOption option, void *data)
{
  struct Spi *interface = object;
  LPC_SSP_TypeDef *reg = interface->parent.reg;

  switch (option)
  {
    case IF_PRIORITY:
      *(uint32_t *)data = nvicGetPriority(interface->parent.irq);
      return E_OK;
    case IF_RATE:
      *(uint32_t *)data = sspGetRate(object);
      return E_OK;
    case IF_READY:
      return interface->left || reg->SR & SR_BSY ? E_BUSY : E_OK;
    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static enum result spiSet(void *object, enum ifOption option, const void *data)
{
  struct Spi *interface = object;

  switch (option)
  {
    case IF_ACQUIRE:
      return spinTryLock(&interface->lock) ? E_OK : E_BUSY;
    case IF_BLOCKING:
      interface->blocking = true;
      return E_OK;
    case IF_PRIORITY:
      nvicSetPriority(interface->parent.irq, *(uint32_t *)data);
      return E_OK;
    case IF_RATE:
      sspSetRate(object, *(uint32_t *)data);
      return E_OK;
    case IF_RELEASE:
      spinUnlock(&interface->lock);
      return E_OK;
    case IF_ZEROCOPY:
      interface->blocking = false;
      return E_OK;
    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static uint32_t spiRead(void *object, uint8_t *buffer, uint32_t length)
{
  struct Spi *interface = object;
  LPC_SSP_TypeDef *reg = interface->parent.reg;

  if (!length)
    return 0;

  //TODO In SPI: is it necessary to reset previous transmission and clear FIFO?

  interface->rxBuffer = buffer;
  interface->txBuffer = 0;
  interface->left = interface->length = length;

  /* Clear interrupt flags and enable interrupts */
  reg->ICR = ICR_RTIC;
  reg->IMSC |= IMSC_RXIM | IMSC_RTIM;

  /* Initiate reception by setting pending interrupt flag */
  nvicSetPending(interface->parent.irq);

  if (interface->blocking)
    while (interface->left || reg->SR & SR_BSY);

  return length;
}
/*----------------------------------------------------------------------------*/
static uint32_t spiWrite(void *object, const uint8_t *buffer, uint32_t length)
{
  struct Spi *interface = object;
  LPC_SSP_TypeDef *reg = interface->parent.reg;

  if (!length)
    return 0;

  interface->rxBuffer = 0;
  interface->txBuffer = buffer;
  interface->left = interface->length = length;

  /* Clear interrupt flags and enable interrupts */
  reg->ICR = ICR_RTIC;
  reg->IMSC |= IMSC_RXIM | IMSC_RTIM;

  /* Initiate transmission by setting pending interrupt flag */
  nvicSetPending(interface->parent.irq);

  if (interface->blocking)
    while (interface->left || reg->SR & SR_BSY);

  return length;
}
