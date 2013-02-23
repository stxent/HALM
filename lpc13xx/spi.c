/*
 * spi.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "spi.h"
#include "ssp_defs.h"
/*----------------------------------------------------------------------------*/
/* SPI settings */
#define DEFAULT_PRIORITY                15 /* Lowest priority in Cortex-M3 */
/*----------------------------------------------------------------------------*/
static void spiHandler(void *);
static enum result spiInit(void *, const void *);
static void spiDeinit(void *);
static uint32_t spiRead(void *, uint8_t *, uint32_t);
static uint32_t spiWrite(void *, const uint8_t *, uint32_t);
static enum result spiGetOpt(void *, enum ifOption, void *);
static enum result spiSetOpt(void *, enum ifOption, const void *);
/*----------------------------------------------------------------------------*/
/* We like structures so we put a structure in a structure */
/* So we can initialize a structure while we initialize a structure */
static const struct SspClass spiTable = {
    .parent = {
        .size = sizeof(struct Spi),
        .init = spiInit,
        .deinit = spiDeinit,

        .read = spiRead,
        .write = spiWrite,
        .getopt = spiGetOpt,
        .setopt = spiSetOpt
    },
    .handler = spiHandler
};
/*----------------------------------------------------------------------------*/
const struct SspClass *Spi = &spiTable;
/*----------------------------------------------------------------------------*/
static void spiHandler(void *object)
{
  struct Spi *device = object;
  uint8_t status = device->parent.reg->MIS;

  if (status & (MIS_RXMIS | MIS_RTMIS))
  {
    /* Frame will be removed from FIFO after reading from DR register */
    while (device->parent.reg->SR & SR_RNE && device->left)
    {
      *device->rxBuffer++ = device->parent.reg->DR;
      device->left--;
    }
    /* Fill transmit FIFO with dummy frames */
    /* TODO Move to TX interrupt? */
    while (device->parent.reg->SR & SR_TNF && device->fill)
    {
      device->parent.reg->DR = 0xFF; /* TODO Select dummy frame value */
      device->fill--;
    }
    /* Disable receive interrupts when all frames have been received */
    if (!device->left)
      device->parent.reg->IMSC &= ~(IMSC_RXIM | IMSC_RTIM);
  }
  if (status & MIS_TXMIS)
  {
    /* Fill transmit FIFO */
    while (device->parent.reg->SR & SR_TNF && device->left)
    {
      device->parent.reg->DR = *device->txBuffer++;
      device->left--;
    }
    /* Disable transmit interrupt when all frames have been pushed to FIFO */
    if (!device->left)
      device->parent.reg->IMSC &= ~IMSC_TXIM;
  }
}
/*----------------------------------------------------------------------------*/
static enum result spiInit(void *object, const void *configPtr)
{
  /* Set pointer to device configuration data */
  const struct SpiConfig *config = configPtr;
  struct Spi *device = object;
  const struct SspConfig parentConfig = {
      .channel = config->channel,
      .sck = config->sck,
      .miso = config->miso,
      .mosi = config->mosi,
      .rate = config->rate,
      .frame = 8, /* Fixed frame size */
      .cs = config->cs
  };
  enum result res;

  /* Call SSP class constructor */
  if ((res = Ssp->parent.init(object, &parentConfig)) != E_OK)
    return res;
  device->channelLock = MUTEX_UNLOCKED;

  /* Set interrupt priority, lowest by default */
  NVIC_SetPriority(device->parent.irq, DEFAULT_PRIORITY);
  /* Enable UART interrupt */
  NVIC_EnableIRQ(device->parent.irq);
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void spiDeinit(void *object)
{
  struct Spi *device = object;

  NVIC_DisableIRQ(device->parent.irq);
  Spi->parent.deinit(device); /* Call SSP class destructor */
}
/*----------------------------------------------------------------------------*/
static uint32_t spiRead(void *object, uint8_t *buffer, uint32_t length)
{
  struct Spi *device = object;

  /* Check channel availability */
  if (device->parent.reg->SR & SR_BSY || !mutexTryLock(&device->channelLock))
    return 0;

  device->fill = length;
  /* Pop all previously received frames */
  while (device->parent.reg->SR & SR_RNE)
    (void)device->parent.reg->DR; /* TODO Check behavior */

  /* Fill transmit FIFO with dummy frames */
  while (device->parent.reg->SR & SR_TNF && device->fill)
  {
    device->parent.reg->DR = 0xFF; /* TODO Select dummy frame value */
    device->fill--;
  }

  device->rxBuffer = buffer;
  device->left = length;
  /* Enable receive FIFO half full and receive FIFO timeout interrupts */
  device->parent.reg->IMSC |= IMSC_RXIM | IMSC_RTIM;

  /* Wait until all frames will be received */
  while (device->left || (device->parent.reg->SR & SR_BSY));

  mutexUnlock(&device->channelLock);
  return length;
}
/*----------------------------------------------------------------------------*/
static uint32_t spiWrite(void *object, const uint8_t *buffer, uint32_t length)
{
  struct Spi *device = object;

  /* Check channel availability */
  if (device->parent.reg->SR & SR_BSY || !mutexTryLock(&device->channelLock))
    return 0;

  device->left = length;
  /* Fill transmit FIFO */
  while (device->parent.reg->SR & SR_TNF && device->left)
  {
    device->parent.reg->DR = *buffer++;
    device->left--;
  }

  /* Enable transmit interrupt when packet size is greater than FIFO length */
  if (device->left)
  {
    device->txBuffer = buffer;
    device->parent.reg->IMSC |= IMSC_TXIM;
  }
  /* Wait until all frames will be transmitted */
  while (device->left || !(device->parent.reg->SR & SR_TFE));

  mutexUnlock(&device->channelLock);
  return length;
}
/*----------------------------------------------------------------------------*/
static enum result spiGetOpt(void *object, enum ifOption option, void *data)
{
  return E_ERROR;
}
/*----------------------------------------------------------------------------*/
static enum result spiSetOpt(void *object, enum ifOption option,
    const void *data)
{
  struct Spi *device = object;

  switch (option)
  {
    case IF_SPEED:
      sspSetRate(object, *(uint32_t *)data);
      return E_OK;
    case IF_PRIORITY:
      NVIC_SetPriority(device->parent.irq, *(uint32_t *)data);
      return E_OK;
    default:
      return E_ERROR;
  }
}
