/*
 * serial.c
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include "platform/nxp/serial.h"
#include "platform/nxp/uart_defs.h"
/*----------------------------------------------------------------------------*/
#define DEFAULT_PRIORITY  255 /* Lowest interrupt priority in Cortex-M3 */
#define RX_FIFO_LEVEL     2 /* 8 characters */
#define TX_FIFO_SIZE      8
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *);
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
    .size = sizeof(struct Serial),
    .init = serialInit,
    .deinit = serialDeinit,

    .callback = serialCallback,
    .get = serialGet,
    .set = serialSet,
    .read = serialRead,
    .write = serialWrite
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass *Serial = &serialTable;
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *object)
{
  struct Serial *interface = object;

  /* Interrupt status cleared when performed read operation on IIR register */
  (void)interface->parent.reg->IIR;

  /* Byte will be removed from FIFO after reading from RBR register */
  while (interface->parent.reg->LSR & LSR_RDR)
  {
    uint8_t data = interface->parent.reg->RBR;
    /* Received bytes will be dropped when queue becomes full */
    if (!queueFull(&interface->rxQueue))
      queuePush(&interface->rxQueue, data);
  }
  if (interface->parent.reg->LSR & LSR_THRE)
  {
    /* Fill FIFO with selected burst size or less */
    uint32_t limit = queueSize(&interface->txQueue) < TX_FIFO_SIZE
        ? queueSize(&interface->txQueue) : TX_FIFO_SIZE;
    while (limit--)
      interface->parent.reg->THR = queuePop(&interface->txQueue);
  }
}
/*----------------------------------------------------------------------------*/
static enum result serialInit(void *object, const void *configPtr)
{
  /* Set pointer to interface configuration data */
  const struct SerialConfig * const config = configPtr;
  struct Serial *interface = object;
  struct UartConfig parentConfig;
  enum result res;

  /* Check interface configuration data */
  assert(config);

  /* Initialize parent configuration structure */
  parentConfig.channel = config->channel;
  parentConfig.rx = config->rx;
  parentConfig.tx = config->tx;
  parentConfig.rate = config->rate;
  parentConfig.parity = config->parity;

  /* Call UART class constructor */
  if ((res = Uart->init(object, &parentConfig)) != E_OK)
    return res;

  /* Set pointer to hardware interrupt handler */
  interface->parent.handler = interruptHandler;

  /* Initialize RX and TX queues */
  if ((res = queueInit(&interface->rxQueue, config->rxLength)) != E_OK)
    return res;
  if ((res = queueInit(&interface->txQueue, config->txLength)) != E_OK)
    return res;
  /* Create mutex */
  if ((res = mutexInit(&interface->queueLock)) != E_OK)
    return res;


  /* Enable and clear FIFO, set RX trigger level to 8 bytes */
  interface->parent.reg->FCR &= ~FCR_RX_TRIGGER_MASK;
  interface->parent.reg->FCR |= FCR_ENABLE | FCR_RX_TRIGGER(RX_FIFO_LEVEL);
  /* Enable RBR and THRE interrupts */
  interface->parent.reg->IER |= IER_RBR | IER_THRE;
  interface->parent.reg->TER = TER_TXEN;

  /* Set interrupt priority, lowest by default */
  nvicSetPriority(interface->parent.irq, DEFAULT_PRIORITY);
  /* Enable UART interrupt */
  nvicEnable(interface->parent.irq);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void serialDeinit(void *object)
{
  struct Serial *interface = object;

  nvicDisable(interface->parent.irq); /* Disable interrupt */
  queueDeinit(&interface->txQueue);
  queueDeinit(&interface->rxQueue);
  Uart->deinit(interface); /* Call UART class destructor */
}
/*----------------------------------------------------------------------------*/
static enum result serialCallback(void *object, void (*callback)(void *),
    void *argument)
{
  /* Callback functionality not implemented */
  return E_ERROR;
}
/*----------------------------------------------------------------------------*/
static enum result serialGet(void *object, enum ifOption option, void *data)
{
  struct Serial *interface = object;

  switch (option)
  {
    case IF_AVAILABLE:
      *(uint32_t *)data = queueSize(&interface->rxQueue);
      return E_OK;
    case IF_PENDING:
      *(uint32_t *)data = queueSize(&interface->txQueue);
      return E_OK;
    case IF_PRIORITY:
      *(uint32_t *)data = nvicGetPriority(interface->parent.irq);
      return E_OK;
    case IF_RATE:
      /* TODO */
      return E_ERROR;
    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static enum result serialSet(void *object, enum ifOption option,
    const void *data)
{
  struct Serial *interface = object;
  enum result res;

  switch (option)
  {
    case IF_PRIORITY:
      nvicSetPriority(interface->parent.irq, *(uint32_t *)data);
      return E_OK;
    case IF_RATE:
    {
      struct UartRateConfig rate;

      if ((res = uartCalcRate(&rate, *(uint32_t *)data)) != E_OK)
        return res;
      uartSetRate(object, rate);
      return E_OK;
    }
    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static uint32_t serialRead(void *object, uint8_t *buffer, uint32_t length)
{
  struct Serial *interface = object;
  uint32_t read;

  mutexLock(&interface->queueLock);
  nvicDisable(interface->parent.irq);
  read = queuePopArray(&interface->rxQueue, buffer, length);
  nvicEnable(interface->parent.irq);
  mutexUnlock(&interface->queueLock);

  return read;
}
/*----------------------------------------------------------------------------*/
static uint32_t serialWrite(void *object, const uint8_t *buffer,
    uint32_t length)
{
  struct Serial *interface = object;
  uint32_t written = 0;

  mutexLock(&interface->queueLock);
  nvicDisable(interface->parent.irq);
  /* Check transmitter state */
  if (interface->parent.reg->LSR & LSR_TEMT && queueEmpty(&interface->txQueue))
  {
    /* Transmitter is idle, fill TX FIFO */
    uint32_t limit = length < TX_FIFO_SIZE ? length : TX_FIFO_SIZE;
    for (; written < limit; ++written)
      interface->parent.reg->THR = *buffer++;
    length -= written;
  }
  /* Fill TX queue with the rest of data */
  if (length)
    written += queuePushArray(&interface->txQueue, buffer, length);
  nvicEnable(interface->parent.irq);
  mutexUnlock(&interface->queueLock);

  return written;
}
