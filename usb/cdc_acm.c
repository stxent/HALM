/*
 * cdc_acm.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <stdlib.h>
#include <string.h>
#include <irq.h>
#include <usb/cdc_acm.h>
#include <usb/usb_trace.h>
/*----------------------------------------------------------------------------*/
#define REQUEST_POOL_SIZE (2 * 2)
/*----------------------------------------------------------------------------*/
static void cdcDataReceived(struct UsbRequest *, void *);
static void cdcDataSent(struct UsbRequest *, void *);
static void resetBuffers(struct CdcAcm *);
/*----------------------------------------------------------------------------*/
static enum result interfaceInit(void *, const void *);
static void interfaceDeinit(void *);
static enum result interfaceCallback(void *, void (*)(void *), void *);
static enum result interfaceGet(void *, enum ifOption, void *);
static enum result interfaceSet(void *, enum ifOption, const void *);
static uint32_t interfaceRead(void *, uint8_t *, uint32_t);
static uint32_t interfaceWrite(void *, const uint8_t *, uint32_t);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass interfaceTable = {
    .size = sizeof(struct CdcAcm),
    .init = interfaceInit,
    .deinit = interfaceDeinit,

    .callback = interfaceCallback,
    .get = interfaceGet,
    .set = interfaceSet,
    .read = interfaceRead,
    .write = interfaceWrite
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass * const CdcAcm = &interfaceTable;
/*----------------------------------------------------------------------------*/
static void cdcDataReceived(struct UsbRequest *request, void *argument)
{
  struct CdcAcm * const interface = argument;
  const uint32_t spaceLeft = byteQueueCapacity(&interface->rxQueue)
      - byteQueueSize(&interface->rxQueue);
  bool error = false;
  bool event = false;
  bool returnToPool = false;

  if (request->status == REQUEST_CANCELLED)
  {
    returnToPool = true;
  }
  else if (request->status != REQUEST_COMPLETED)
  {
    error = true;
  }
  else
  {
    if (spaceLeft >= request->length)
    {
      byteQueuePushArray(&interface->rxQueue, request->buffer, request->length);

      request->length = 0;
      request->status = 0;

      if (usbEpEnqueue(interface->rxDataEp, request) != E_OK)
        error = true;
    }
    else
    {
      interface->queuedRxBytes += request->length;
      returnToPool = true;
    }

    event = true;
  }

  if (returnToPool || error)
  {
    queuePush(&interface->rxRequestQueue, &request);
  }

  if (error)
  {
    interface->suspended = true;
    usbTrace("cdc_acm: suspended in read callback");
  }
  else if (event && interface->callback)
  {
    interface->callback(interface->callbackArgument);
  }
}
/*----------------------------------------------------------------------------*/
static void cdcDataSent(struct UsbRequest *request, void *argument)
{
  struct CdcAcm * const interface = argument;
  bool error = false;
  bool returnToPool = false;

  if (request->status == REQUEST_CANCELLED)
  {
    returnToPool = true;
  }
  else if (request->status != REQUEST_COMPLETED)
  {
    error = true;
  }
  else if (byteQueueEmpty(&interface->txQueue))
  {
    if (queueSize(&interface->txRequestQueue) == REQUEST_POOL_SIZE / 2 - 1
        && request->length == CDC_DATA_EP_SIZE)
    {
      /* Send empty packet to finish data transfer */
      request->length = 0;
      request->status = 0;

      if (usbEpEnqueue(interface->txDataEp, request) != E_OK)
        error = true;
    }
    else
    {
      returnToPool = true;
    }
  }
  else
  {
    /* Try to send remaining data */
    while (1)
    {
      request->length = byteQueuePopArray(&interface->txQueue,
          request->buffer, request->capacity);
      request->status = 0;

      if (usbEpEnqueue(interface->txDataEp, request) != E_OK)
      {
        /* Hardware error occurred, suspend the interface and wait for reset */
        error = true;
        break;
      }

      if (byteQueueEmpty(&interface->txQueue))
        break;

      if (queueEmpty(&interface->txRequestQueue))
        break;
      queuePop(&interface->txRequestQueue, &request);
    }
  }

  if (returnToPool || error)
  {
    queuePush(&interface->txRequestQueue, &request);
  }

  if (error)
  {
    interface->suspended = true;
    usbTrace("cdc_acm: suspended in write callback");
  }
  else if (byteQueueSize(&interface->txQueue) <
      (byteQueueCapacity(&interface->txQueue) >> 1))
  {
    /* Notify when the transmit queue is at least half-empty */
    if (interface->callback)
      interface->callback(interface->callbackArgument);
  }
}
/*----------------------------------------------------------------------------*/
static void resetBuffers(struct CdcAcm *interface)
{
  /* Return queued requests to pools */
  usbEpClear(interface->rxDataEp);
  usbEpClear(interface->txDataEp);

  /* Clear byte queues */
  byteQueueClear(&interface->rxQueue);
  byteQueueClear(&interface->txQueue);

  interface->queuedRxBytes = 0;
}
/*----------------------------------------------------------------------------*/
void cdcAcmOnParametersChanged(struct CdcAcm *interface)
{
  interface->updated = true;

  if (interface->callback)
    interface->callback(interface->callbackArgument);
}
/*----------------------------------------------------------------------------*/
void cdcAcmOnStatusChanged(struct CdcAcm *interface, uint8_t status)
{
  if (!interface->suspended && (status & DEVICE_STATUS_SUSPENDED))
  {
    interface->suspended = true;

    usbTrace("cdc_acm: suspended externally");
  }

  if (status & DEVICE_STATUS_RESET)
  {
    interface->suspended = true;
    resetBuffers(interface);

    while (!queueEmpty(&interface->rxRequestQueue))
    {
      struct UsbRequest *request;

      queuePop(&interface->rxRequestQueue, &request);
      request->length = 0;
      request->status = 0;

      if (usbEpEnqueue(interface->rxDataEp, request) != E_OK)
      {
        queuePush(&interface->rxRequestQueue, &request);
        break;
      }
    }

    interface->suspended = false;
    usbTrace("cdc_acm: reset completed");
  }

  if (interface->callback)
    interface->callback(interface->callbackArgument);
}
/*----------------------------------------------------------------------------*/
static enum result interfaceInit(void *object, const void *configBase)
{
  const struct CdcAcmConfig * const config = configBase;
  struct CdcAcm * const interface = object;
  const struct CdcAcmBaseConfig parentConfig = {
      .device = config->device,
      .owner = interface,
      .endpoint = {
          .interrupt = config->endpoint.interrupt,
          .rx = config->endpoint.rx,
          .tx = config->endpoint.tx
      }
  };
  enum result res;

  interface->driver = init(CdcAcmBase, &parentConfig);
  if (!interface->driver)
    return E_ERROR;

  /* Input buffer should be greater or equal to endpoint buffer */
  if (config->rxLength < CDC_DATA_EP_SIZE)
    return E_VALUE;

  res = byteQueueInit(&interface->rxQueue, config->rxLength);
  if (res != E_OK)
    return res;
  res = byteQueueInit(&interface->txQueue, config->txLength);
  if (res != E_OK)
    return res;
  res = queueInit(&interface->rxRequestQueue, sizeof(struct UsbRequest *),
      REQUEST_POOL_SIZE / 2);
  if (res != E_OK)
    return res;
  res = queueInit(&interface->txRequestQueue, sizeof(struct UsbRequest *),
      REQUEST_POOL_SIZE / 2);
  if (res != E_OK)
    return res;

  interface->callback = 0;
  interface->queuedRxBytes = 0;
  interface->suspended = true;
  interface->updated = false;

  interface->notificationEp = usbDevAllocate(config->device,
      config->endpoint.interrupt);
  if (!interface->notificationEp)
    return E_ERROR;
  interface->rxDataEp = usbDevAllocate(config->device, config->endpoint.rx);
  if (!interface->rxDataEp)
    return E_ERROR;
  interface->txDataEp = usbDevAllocate(config->device, config->endpoint.tx);
  if (!interface->txDataEp)
    return E_ERROR;

  /* Allocate requests */
  interface->requests = malloc(REQUEST_POOL_SIZE * sizeof(struct UsbRequest));
  if (!interface->requests)
    return E_MEMORY;

  for (unsigned short index = 0; index < REQUEST_POOL_SIZE; ++index)
  {
    res = usbRequestInit(interface->requests + index, CDC_DATA_EP_SIZE);
    if (res != E_OK)
      return res;
  }

  struct UsbRequest *request = interface->requests;

  for (unsigned short index = 0; index < REQUEST_POOL_SIZE / 2; ++index)
  {
    usbRequestCallback(request, cdcDataReceived, interface);
    queuePush(&interface->rxRequestQueue, &request);
    ++request;

    usbRequestCallback(request, cdcDataSent, interface);
    queuePush(&interface->txRequestQueue, &request);
    ++request;
  }

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void interfaceDeinit(void *object)
{
  struct CdcAcm * const interface = object;

  usbEpClear(interface->notificationEp);
  usbEpClear(interface->txDataEp);
  usbEpClear(interface->rxDataEp);

  assert(queueSize(&interface->rxRequestQueue) == REQUEST_POOL_SIZE / 2);
  assert(queueSize(&interface->txRequestQueue) == REQUEST_POOL_SIZE / 2);

  for (unsigned short index = 0; index < REQUEST_POOL_SIZE; ++index)
    usbRequestDeinit(interface->requests + index);
  free(interface->requests);

  deinit(interface->txDataEp);
  deinit(interface->rxDataEp);
  deinit(interface->notificationEp);

  queueDeinit(&interface->txRequestQueue);
  queueDeinit(&interface->rxRequestQueue);
  byteQueueDeinit(&interface->txQueue);
  byteQueueDeinit(&interface->rxQueue);

  deinit(interface->driver);
}
/*----------------------------------------------------------------------------*/
static enum result interfaceCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct CdcAcm * const interface = object;

  interface->callbackArgument = argument;
  interface->callback = callback;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result interfaceGet(void *object, enum ifOption option,
    void *data)
{
  struct CdcAcm * const interface = object;

  switch (option)
  {
    case IF_AVAILABLE:
      *(uint32_t *)data = byteQueueSize(&interface->rxQueue)
          + interface->queuedRxBytes;
      return E_OK;

    case IF_PENDING:
      *(uint32_t *)data = byteQueueSize(&interface->txQueue);
      return E_OK;

    case IF_RATE:
      *(uint32_t *)data = interface->driver->lineCoding.dteRate;
      return E_OK;

/* TODO Extend option list
    case IF_WIDTH:
      *(uint32_t *)data = interface->driver->lineCoding.dataBits;
      return E_OK; */

    default:
      break;
  }

  switch ((enum cdcAcmOption)option)
  {
    case IF_CDC_ACM_STATUS:
    {
      uint32_t status = 0;

      if (interface->updated)
      {
        interface->updated = false;
        status |= CDC_ACM_LINE_CHANGED;
      }
      if (interface->suspended)
        status |= CDC_ACM_SUSPENDED;
      if (!byteQueueEmpty(&interface->rxQueue))
        status |= CDC_ACM_RX_AVAILABLE;
      if (byteQueueEmpty(&interface->txQueue))
        status |= CDC_ACM_TX_EMPTY;

      *(uint32_t *)data = status;
      return E_OK;
    }

    default:
      break;
  }

  return E_ERROR;
}
/*----------------------------------------------------------------------------*/
static enum result interfaceSet(void *object __attribute__((unused)),
    enum ifOption option __attribute__((unused)),
    const void *data __attribute__((unused)))
{
  return E_ERROR;
}
/*----------------------------------------------------------------------------*/
static uint32_t interfaceRead(void *object, uint8_t *buffer, uint32_t length)
{
  struct CdcAcm * const interface = object;
  const uint32_t sourceLength = length;

  if (!length || interface->suspended)
    return 0;

  const irqState state = irqSave();

  if (!byteQueueEmpty(&interface->rxQueue))
  {
    const uint32_t bytesRead = byteQueuePopArray(&interface->rxQueue,
        buffer, length);

    buffer += bytesRead;
    length -= bytesRead;
  }

  while (!queueEmpty(&interface->rxRequestQueue))
  {
    const uint32_t spaceAvailable = byteQueueCapacity(&interface->rxQueue)
        - byteQueueSize(&interface->rxQueue);
    struct UsbRequest *request;

    queuePeek(&interface->rxRequestQueue, &request);
    if (spaceAvailable < request->length)
      break;

    queuePop(&interface->rxRequestQueue, 0);
    interface->queuedRxBytes -= request->length;

    const uint8_t *bufferPosition = request->buffer;
    uint16_t bytesLeft = request->length;

    if (length)
    {
      const uint16_t chunkLength = bytesLeft < length ? bytesLeft : length;

      memcpy(buffer, bufferPosition, chunkLength);
      buffer += chunkLength;
      length -= chunkLength;
      bufferPosition += chunkLength;
      bytesLeft -= chunkLength;
    }

    if (bytesLeft)
      byteQueuePushArray(&interface->rxQueue, bufferPosition, bytesLeft);

    request->length = 0;
    request->status = 0;

    if (usbEpEnqueue(interface->rxDataEp, request) != E_OK)
    {
      /* Hardware error occurred, suspend the interface and wait for reset */
      interface->suspended = true;
      queuePush(&interface->rxRequestQueue, &request);

      usbTrace("cdc_acm: suspended in read function");
      break;
    }
  }

  irqRestore(state);
  return sourceLength - length;
}
/*----------------------------------------------------------------------------*/
static uint32_t interfaceWrite(void *object, const uint8_t *buffer,
    uint32_t length)
{
  struct CdcAcm * const interface = object;
  const uint32_t sourceLength = length;

  if (!length || interface->suspended)
    return 0;

  const irqState state = irqSave();

  if (byteQueueEmpty(&interface->txQueue)
      && !queueEmpty(&interface->txRequestQueue))
  {
    struct UsbRequest *request;

    queuePop(&interface->txRequestQueue, &request);

    const uint32_t bytesToWrite = length > request->capacity ?
        request->capacity : length;

    request->length = bytesToWrite;
    request->status = 0;
    memcpy(request->buffer, buffer, bytesToWrite);

    if (usbEpEnqueue(interface->txDataEp, request) != E_OK)
    {
      /* Hardware error occurred, suspend the interface and wait for reset */
      interface->suspended = true;
      queuePush(&interface->txRequestQueue, &request);

      usbTrace("cdc_acm: suspended in write function");

      irqRestore(state);
      return 0;
    }
    else
    {
      buffer += bytesToWrite;
      length -= bytesToWrite;
    }
  }

  if (length)
    length -= byteQueuePushArray(&interface->txQueue, buffer, length);

  irqRestore(state);
  return sourceLength - length;
}
