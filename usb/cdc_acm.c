/*
 * cdc_acm.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <stdlib.h>
#include <string.h>
#include <irq.h>
#include <usb/cdc_acm.h>
/*----------------------------------------------------------------------------*/
#define REQUEST_QUEUE_SIZE 2
/*----------------------------------------------------------------------------*/
static void cdcDataReceived(struct UsbRequest *, void *);
static void cdcDataSent(struct UsbRequest *, void *);
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

  //FIXME Check request status
  if (spaceLeft < request->length)
  {
    interface->queuedRxBytes += request->length;
    queuePush(&interface->rxRequestQueue, &request);
  }
  else
  {
    byteQueuePushArray(&interface->rxQueue, request->buffer, request->length);

    request->length = 0;
    request->status = 0;
    usbEpEnqueue(interface->rxDataEp, request);

    if (interface->callback)
      interface->callback(interface->callbackArgument);
  }
}
/*----------------------------------------------------------------------------*/
static void cdcDataSent(struct UsbRequest *request, void *argument)
{
  struct CdcAcm * const interface = argument;

  //FIXME Check request status
  if (byteQueueEmpty(&interface->txQueue))
  {
    queuePush(&interface->txRequestQueue, &request);
    return;
  }

  uint32_t available = byteQueueSize(&interface->txQueue);

  while (available)
  {
    const uint32_t bytesToWrite = byteQueuePopArray(&interface->txQueue,
        request->buffer, request->capacity);

    request->length = bytesToWrite;
    request->status = 0;

    usbEpEnqueue(interface->txDataEp, request);

    if (!(available = byteQueueSize(&interface->txQueue)))
      break;
    if (queueEmpty(&interface->txRequestQueue))
      break;

    queuePop(&interface->txRequestQueue, &request);
  }

  if (available < (byteQueueCapacity(&interface->txQueue) >> 1))
  {
    if (interface->callback)
      interface->callback(interface->callbackArgument);
  }
}
/*----------------------------------------------------------------------------*/
static enum result interfaceInit(void *object, const void *configBase)
{
  const struct CdcAcmConfig * const config = configBase;
  const struct CdcAcmBaseConfig parentConfig = {
      .device = config->device,
      .serial = config->serial,
      .composite = config->composite,
      .endpoint = {
          .interrupt = config->endpoint.interrupt,
          .rx = config->endpoint.rx,
          .tx = config->endpoint.tx
      }
  };
  struct CdcAcm * const interface = object;
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
      REQUEST_QUEUE_SIZE);
  if (res != E_OK)
    return res;
  res = queueInit(&interface->txRequestQueue, sizeof(struct UsbRequest *),
      REQUEST_QUEUE_SIZE);
  if (res != E_OK)
    return res;

  interface->callback = 0;
  interface->queuedRxBytes = 0;

  interface->notificationEp = usbDevAllocate(config->device,
      CDC_NOTIFICATION_EP_SIZE, config->endpoint.interrupt);
  if (!interface->notificationEp)
    return E_ERROR;
  interface->rxDataEp = usbDevAllocate(config->device, CDC_DATA_EP_SIZE,
      config->endpoint.tx);
  if (!interface->rxDataEp)
    return E_ERROR;
  interface->txDataEp = usbDevAllocate(config->device, CDC_DATA_EP_SIZE,
      config->endpoint.rx);
  if (!interface->txDataEp)
    return E_ERROR;

  //TODO Optimize?
  for (uint8_t index = 0; index < REQUEST_QUEUE_SIZE; ++index)
  {
    struct UsbRequest * const request = malloc(sizeof(struct UsbRequest));
    usbRequestInit(request, CDC_DATA_EP_SIZE);

    request->callback = cdcDataReceived;
    request->callbackArgument = interface;
    usbEpEnqueue(interface->rxDataEp, request);
  }

  for (uint8_t index = 0; index < REQUEST_QUEUE_SIZE; ++index)
  {
    struct UsbRequest * const request = malloc(sizeof(struct UsbRequest));
    usbRequestInit(request, CDC_DATA_EP_SIZE);

    request->callback = cdcDataSent;
    request->callbackArgument = interface;
    queuePush(&interface->txRequestQueue, &request);
  }

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void interfaceDeinit(void *object)
{
  struct CdcAcm * const interface = object;

  deinit(interface->txDataEp);
  deinit(interface->rxDataEp);
  deinit(interface->notificationEp);

  //TODO Deinit endpoints
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
      *(uint32_t *)data = interface->driver->state.coding.dteRate;
      return E_OK;

/* TODO Extend option list
    case IF_WIDTH:
      *(uint32_t *)data = interface->driver->lineCoding.dataBits;
      return E_OK; */

    default:
      return E_ERROR;
  }
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

  interruptsDisable();

  if (!byteQueueEmpty(&interface->rxQueue))
  {
    const uint32_t bytesToRead = byteQueuePopArray(&interface->rxQueue,
        buffer, length);

    buffer += bytesToRead;
    length -= bytesToRead;
  }

  /* TODO Use queuedRxBytes to simplify logic */
  while (!queueEmpty(&interface->rxRequestQueue))
  {
    struct UsbRequest *request;

    queuePop(&interface->rxRequestQueue, &request);
    interface->queuedRxBytes -= request->length;

    const bool spaceAvailable = request->length < length;
    const uint32_t bytesToRead = spaceAvailable ? request->length : length;

    memcpy(buffer, request->buffer, bytesToRead);
    buffer += bytesToRead;
    length -= bytesToRead;

    if (!spaceAvailable)
    {
      byteQueuePushArray(&interface->rxQueue, request->buffer + bytesToRead,
          request->length - bytesToRead);
    }

    request->length = 0;
    request->status = 0;
    usbEpEnqueue(interface->rxDataEp, request);

    if (!spaceAvailable)
      break;
  }

  interruptsEnable();
  return sourceLength - length;
}
/*----------------------------------------------------------------------------*/
static uint32_t interfaceWrite(void *object, const uint8_t *buffer,
    uint32_t length)
{
  struct CdcAcm * const interface = object;
  const uint32_t sourceLength = length;

  interruptsDisable();

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
    buffer += bytesToWrite;
    length -= bytesToWrite;

    usbEpEnqueue(interface->txDataEp, request);
  }

  if (length)
    length -= byteQueuePushArray(&interface->txQueue, buffer, length);

  interruptsEnable();
  return sourceLength - length;
}
