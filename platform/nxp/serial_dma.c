/*
 * serial_dma.c
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <platform/nxp/gpdma.h>
#include <platform/nxp/serial_dma.h>
#include <platform/nxp/uart_defs.h>
/*----------------------------------------------------------------------------*/
#define RX_FIFO_LEVEL 0 /* 1 character */
/*----------------------------------------------------------------------------*/
static void dmaHandler(void *);
static enum result dmaSetup(struct SerialDma *, uint8_t, uint8_t);
/*----------------------------------------------------------------------------*/
static enum result serialInit(void *, const void *);
static void serialDeinit(void *);
static enum result serialCallback(void *, void (*)(void *), void *);
static enum result serialGet(void *, enum ifOption, void *);
static enum result serialSet(void *, enum ifOption, const void *);
static uint32_t serialRead(void *, uint8_t *, uint32_t);
static uint32_t serialWrite(void *, const uint8_t *, uint32_t);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass serialTable = {
    .size = sizeof(struct SerialDma),
    .init = serialInit,
    .deinit = serialDeinit,

    .callback = serialCallback,
    .get = serialGet,
    .set = serialSet,
    .read = serialRead,
    .write = serialWrite
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass * const SerialDma = &serialTable;
/*----------------------------------------------------------------------------*/
static void dmaHandler(void *object)
{
  struct SerialDma * const interface = object;

  if (interface->callback)
    interface->callback(interface->callbackArgument);
}
/*----------------------------------------------------------------------------*/
static enum result dmaSetup(struct SerialDma *interface, uint8_t rxChannel,
    uint8_t txChannel)
{
  const struct GpDmaConfig channels[2] = {
      {
          .event = GPDMA_UART0_RX + interface->parent.channel,
          .channel = rxChannel,
          .source.increment = false,
          .destination.increment = true,
          .type = GPDMA_TYPE_P2M,
          .burst = DMA_BURST_1,
          .width = DMA_WIDTH_BYTE
      }, {
          .event = GPDMA_UART0_TX + interface->parent.channel,
          .channel = txChannel,
          .source.increment = true,
          .destination.increment = false,
          .type = GPDMA_TYPE_M2P,
          .burst = DMA_BURST_1,
          .width = DMA_WIDTH_BYTE
      }
  };

  interface->rxDma = init(GpDma, channels + 0);
  if (!interface->rxDma)
    return E_ERROR;
  dmaCallback(interface->rxDma, dmaHandler, interface);

  interface->txDma = init(GpDma, channels + 1);
  if (!interface->txDma)
    return E_ERROR;
  dmaCallback(interface->txDma, dmaHandler, interface);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result serialInit(void *object, const void *configPtr)
{
  const struct SerialDmaConfig * const config = configPtr;
  const struct UartBaseConfig parentConfig = {
      .channel = config->channel,
      .rx = config->rx,
      .tx = config->tx
  };
  struct SerialDma * const interface = object;
  struct UartRateConfig rateConfig;
  enum result res;

  /* Call base class constructor */
  if ((res = UartBase->init(object, &parentConfig)) != E_OK)
    return res;

  if ((res = uartCalcRate(object, config->rate, &rateConfig)) != E_OK)
    return res;

  if ((res = dmaSetup(interface, config->rxChannel, config->txChannel)) != E_OK)
    return res;

  interface->callback = 0;

  LPC_UART_Type * const reg = interface->parent.reg;

  /* Set 8-bit length */
  reg->LCR = LCR_WORD_8BIT;
  /* Enable FIFO and DMA, set RX trigger level */
  reg->FCR = (reg->FCR & ~FCR_RX_TRIGGER_MASK) | FCR_RX_TRIGGER(RX_FIFO_LEVEL)
      | FCR_ENABLE | FCR_DMA_ENABLE;
  /* Disable all interrupts */
  reg->IER = 0;
  /* Enable transmitter */
  reg->TER = TER_TXEN;

  uartSetParity(object, config->parity);
  uartSetRate(object, rateConfig);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void serialDeinit(void *object)
{
  struct SerialDma * const interface = object;

  /* Free DMA channel descriptors */
  deinit(interface->txDma);
  deinit(interface->rxDma);
  /* Call UART class destructor */
  UartBase->deinit(interface);
}
/*----------------------------------------------------------------------------*/
static enum result serialCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct SerialDma * const interface = object;

  interface->callbackArgument = argument;
  interface->callback = callback;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result serialGet(void *object, enum ifOption option,
    void *data __attribute__((unused)))
{
  /* TODO Add more options for SerialDma */
  struct SerialDma * const interface = object;

  switch (option)
  {
    case IF_STATUS:
      return !dmaActive(interface->rxDma) && !dmaActive(interface->txDma) ? E_OK
          : E_BUSY;

    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static enum result serialSet(void *object, enum ifOption option,
    const void *data)
{
  struct SerialDma * const interface = object;
  struct UartRateConfig rateConfig;
  enum result res;

  switch (option)
  {
    case IF_BLOCKING:
      interface->blocking = true;
      return E_OK;

    case IF_RATE:
      if ((res = uartCalcRate(object, *(uint32_t *)data, &rateConfig)) == E_OK)
        uartSetRate(object, rateConfig);
      return res;

    case IF_ZEROCOPY:
      interface->blocking = false;
      return E_OK;

    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static uint32_t serialRead(void *object, uint8_t *buffer, uint32_t length)
{
  struct SerialDma * const interface = object;
  LPC_UART_Type * const reg = interface->parent.reg;

  if (length && dmaStart(interface->rxDma, buffer, (const void *)&reg->RBR,
      length) == E_OK)
  {
    if (interface->blocking)
      while (dmaActive(interface->rxDma));

    return length;
  }
  else
    return 0;
}
/*----------------------------------------------------------------------------*/
static uint32_t serialWrite(void *object, const uint8_t *buffer,
    uint32_t length)
{
  struct SerialDma * const interface = object;
  LPC_UART_Type * const reg = interface->parent.reg;

  if (length && dmaStart(interface->txDma, (void *)&reg->THR, buffer,
      length) == E_OK)
  {
    if (interface->blocking)
      while (dmaActive(interface->txDma));

    return length;
  }
  else
    return 0;
}
