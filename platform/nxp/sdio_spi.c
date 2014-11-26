/*
 * sdio_spi.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <string.h>
#include <bits.h>
#include <delay.h>
#include <memory.h>
#include <modules/sdio.h>
#include <modules/sdio_defs.h>
#include <platform/nxp/sdio_spi.h>
/*----------------------------------------------------------------------------*/
enum sdioResponseFlags
{
  FLAG_IDLE_STATE       = 0x01,
  FLAG_ERASE_RESET      = 0x02,
  FLAG_ILLEGAL_COMMAND  = 0x04,
  FLAG_CRC_ERROR        = 0x08,
  FLAG_ERASE_ERROR      = 0x10,
  FLAG_BAD_ADDRESS      = 0x20,
  FLAG_BAD_PARAMETER    = 0x40,
  /* MSB in response tokens is always set to zero */
  FLAG_NO_RESPONSE      = 0x80
};
/*----------------------------------------------------------------------------*/
enum sdioToken
{
  TOKEN_DATA_ACCEPTED     = 0x05,
  TOKEN_DATA_CRC_ERROR    = 0x0B,
  TOKEN_DATA_WRITE_ERROR  = 0x0D,
  TOKEN_START             = 0xFE,
  TOKEN_START_MULTIPLE    = 0xFC,
  TOKEN_STOP              = 0xFD
};
/*----------------------------------------------------------------------------*/
struct SdioStateEntry
{
  void (*execute)(struct SdioSpi *);
  enum sdioSpiState (*advance)(struct SdioSpi *);
  uint8_t next;
};
/*----------------------------------------------------------------------------*/
static void execute(struct SdioSpi *);
static void interruptHandler(void *);

static enum result checksumProcess(struct SdioSpi *, uint16_t);
static void checksumRequest(struct SdioSpi *);
static enum result responseProcess(struct SdioSpi *, enum sdioResponse);
static void responseRequest(struct SdioSpi *, enum sdioResponse);
static enum result responseTokenProcess(struct SdioSpi *);
static void sendCommand(struct SdioSpi *, uint32_t, uint32_t);
static void sendInitSequence(struct SdioSpi *);
static enum result tokenProcess(struct SdioSpi *, enum sdioToken);
static void tokenRequest(struct SdioSpi *);
/*----------------------------------------------------------------------------*/
static enum result sdioInit(void *, const void *);
static void sdioDeinit(void *);
static enum result sdioCallback(void *, void (*)(void *), void *);
static enum result sdioGet(void *, enum ifOption, void *);
static enum result sdioSet(void *, enum ifOption, const void *);
static uint32_t sdioRead(void *, uint8_t *, uint32_t);
static uint32_t sdioWrite(void *, const uint8_t *, uint32_t);
/*----------------------------------------------------------------------------*/
static const struct InterfaceClass sdioTable = {
    .size = sizeof(struct SdioSpi),
    .init = sdioInit,
    .deinit = sdioDeinit,

    .callback = sdioCallback,
    .get = sdioGet,
    .set = sdioSet,
    .read = sdioRead,
    .write = sdioWrite
};
/*----------------------------------------------------------------------------*/
const struct InterfaceClass * const SdioSpi = &sdioTable;
/*----------------------------------------------------------------------------*/
static void stateInitExecute(struct SdioSpi *interface)
{
  memset(interface->buffer, 0xFF, 10);
  ifWrite(interface->interface, interface->buffer, 10);
}
/*----------------------------------------------------------------------------*/
static void stateSendCommandExecute(struct SdioSpi *interface)
{
  pinReset(interface->cs);
  sendCommand(interface, interface->command, interface->argument);
  interface->iteration = 0;
}
/*----------------------------------------------------------------------------*/
static void stateWaitRespExecute(struct SdioSpi *interface)
{
  tokenRequest(interface);
  ++interface->iteration;
}
/*----------------------------------------------------------------------------*/
static enum sdioSpiState stateWaitRespAdvance(struct SdioSpi *interface)
{
  const enum result res = responseTokenProcess(interface);

  if (res == E_OK || res == E_IDLE)
  {
    const uint16_t flags = COMMAND_FLAG_VALUE(interface->command);

    if (flags & SDIO_DATA_MODE)
    {
      if (flags & SDIO_READ_WRITE)
      {
        /* Write data mode */
        //TODO
      }
      else
      {
        /* Read data mode */
        interface->iteration = 0;
        return SDIO_SPI_STATE_WAIT_READ_TOKEN;
      }
    }

    switch (COMMAND_RESP_VALUE(interface->command))
    {
      case SDIO_RESPONSE_SHORT:
        interface->tokenStatus = res;
        return SDIO_SPI_STATE_READ_SHORT;

      case SDIO_RESPONSE_LONG:
        interface->iteration = 0;
        return SDIO_SPI_STATE_WAIT_LONG;

      default:
        interface->status = res;
        return SDIO_SPI_STATE_IDLE;
    }
  }
  else if (res == E_BUSY)
  {
    /* Interface is busy, try again */
    return SDIO_SPI_STATE_WAIT_RESPONSE;
  }
  else
  {
    /* Error occurred, stop the transfer */
    interface->status = res;
    return SDIO_SPI_STATE_IDLE;
  }
}
/*----------------------------------------------------------------------------*/
static void stateReadShortExecute(struct SdioSpi *interface)
{
  /* Read 32-bit response */
  ifRead(interface->interface, interface->buffer, 4);
}
/*----------------------------------------------------------------------------*/
static enum sdioSpiState stateReadShortAdvance(struct SdioSpi *interface)
{
  interface->response[0] = fromBigEndian32(*(uint32_t *)interface->buffer);
  interface->status = interface->tokenStatus;
  return SDIO_SPI_STATE_IDLE;
}
/*----------------------------------------------------------------------------*/
static const struct SdioStateEntry stateTable[] = {
    [SDIO_SPI_STATE_IDLE] = {0, 0, SDIO_SPI_STATE_IDLE},
    [SDIO_SPI_STATE_INIT] = {stateInitExecute, 0, SDIO_SPI_STATE_SEND_CMD},
    [SDIO_SPI_STATE_SEND_CMD] = {stateSendCommandExecute, 0, SDIO_SPI_STATE_WAIT_RESPONSE},
    [SDIO_SPI_STATE_WAIT_RESPONSE] = {stateWaitRespExecute, stateWaitRespAdvance, 0},
    [SDIO_SPI_STATE_READ_SHORT] = {stateReadShortExecute, stateReadShortAdvance, 0},
    [SDIO_SPI_STATE_WAIT_LONG] = {0, 0, SDIO_SPI_STATE_IDLE},
    [SDIO_SPI_STATE_READ_LONG] = {0, 0, SDIO_SPI_STATE_IDLE},
    [SDIO_SPI_STATE_WAIT_READ_TOKEN] = {0, 0, SDIO_SPI_STATE_IDLE},
    [SDIO_SPI_STATE_READ_DATA] = {0, 0, SDIO_SPI_STATE_IDLE},
    [SDIO_SPI_STATE_READ_CRC] = {0, 0, SDIO_SPI_STATE_IDLE},
    [SDIO_SPI_STATE_WAIT_BUSY] = {0, 0, SDIO_SPI_STATE_IDLE}
};
/*----------------------------------------------------------------------------*/
static void execute(struct SdioSpi *interface)
{
  const uint16_t flags = COMMAND_FLAG_VALUE(interface->command);

  interface->status = E_BUSY;
  ifSet(interface->interface, IF_ACQUIRE, 0);

  if (flags & SDIO_INITIALIZE)
  {
    interface->state = SDIO_SPI_STATE_INIT;
    stateTable[SDIO_SPI_STATE_INIT].execute(interface);
  }
  else
  {
    interface->state = SDIO_SPI_STATE_SEND_CMD;
    stateTable[SDIO_SPI_STATE_SEND_CMD].execute(interface);
  }
}
/*----------------------------------------------------------------------------*/
static enum result checksumProcess(struct SdioSpi *interface, uint16_t checksum)
{
  //TODO
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void checksumRequest(struct SdioSpi *interface)
{
  ifRead(interface->interface, interface->buffer, 2);
}
/*----------------------------------------------------------------------------*/
static void dataBlockRequest(struct SdioSpi *interface)
{
  ifRead(interface->interface, interface->rxBuffer, interface->blockLength);
  interface->left -= interface->blockLength;
}
/*----------------------------------------------------------------------------*/
static void dataBlockProcess(struct SdioSpi *interface)
{

}
/*----------------------------------------------------------------------------*/
static enum result responseProcess(struct SdioSpi *interface,
    enum sdioResponse type)
{
  /* Responses come in big-endian format */
  if (type == SDIO_RESPONSE_SHORT)
  {
    interface->response[0] = fromBigEndian32(*(uint32_t *)interface->buffer);
  }
  else
  {
    const uint32_t *buffer = (const uint32_t *)interface->buffer;

    for (uint8_t index = 0; index < 4; ++index)
      interface->response[3 - index] = fromBigEndian32(buffer[index]);

    //TODO Add CRC checking
  }

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void responseRequest(struct SdioSpi *interface, enum sdioResponse type)
{
  if (type == SDIO_RESPONSE_SHORT)
  {
    /* Read 32-bit response */
    ifRead(interface->interface, interface->buffer, 4);
  }
  else
  {
    /* Read 128-bit response and 16-bit checksum */
    ifRead(interface->interface, interface->buffer, 18);
  }
}
/*----------------------------------------------------------------------------*/
static enum result responseTokenProcess(struct SdioSpi *interface)
{
  const uint8_t token = interface->buffer[0];
  enum result res;

  if (token == 0xFF)
    res = interface->iteration >= 8 ? E_TIMEOUT : E_BUSY;
  else if (token & FLAG_NO_RESPONSE)
    res = E_DEVICE;
  else if (token & FLAG_ILLEGAL_COMMAND)
    res = E_INVALID;
  else if ((token & FLAG_IDLE_STATE) == FLAG_IDLE_STATE)
    res = E_IDLE;
  else if (token)
    res = E_ERROR;
  else
    res = E_OK;

  return res;
}
/*----------------------------------------------------------------------------*/
static void sendCommand(struct SdioSpi *interface, uint32_t command,
    uint32_t argument)
{
  argument = toBigEndian32(argument);

  /* Fill the buffer */
  interface->buffer[0] = 0xFF;
  interface->buffer[1] = 0x40 | COMMAND_CODE_VALUE(command);
  memcpy(interface->buffer + 2, &argument, sizeof(argument));

  // TODO Remove hardcoded values
  /* Checksum should be valid only for first CMD0 and CMD8 commands */
  switch (COMMAND_CODE_VALUE(command))
  {
    case 0:
      interface->buffer[6] = 0x94;
      break;

    case 8:
      interface->buffer[6] = 0x86;
      break;

    default:
      interface->buffer[6] = 0x00;
      break;
  }
  interface->buffer[6] |= 0x01; /* Add end bit */
  interface->buffer[7] = 0xFF;

  ifWrite(interface->interface, interface->buffer, 8);
}
/*----------------------------------------------------------------------------*/
static void sendInitSequence(struct SdioSpi *interface)
{

}
/*----------------------------------------------------------------------------*/
static enum result tokenProcess(struct SdioSpi *interface,
    enum sdioToken expected)
{
  const uint8_t token = interface->buffer[0];
  enum result res;

  if (token == 0xFF)
    res = interface->iteration >= 8 ? E_TIMEOUT : E_BUSY;
  else if (token != expected)
    res = E_DEVICE;
  else
    res = E_OK;

  return res;
}
/*----------------------------------------------------------------------------*/
static void tokenRequest(struct SdioSpi *interface)
{
  ifRead(interface->interface, interface->buffer, 1);
}
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *object)
{
  struct SdioSpi * const interface = object;
  enum sdioSpiState next;

  if (stateTable[interface->state].advance)
    next = stateTable[interface->state].advance(interface);
  else
    next = stateTable[interface->state].next;

  interface->state = next;

  if (interface->state != SDIO_SPI_STATE_IDLE)
  {
    if (stateTable[interface->state].execute)
      stateTable[interface->state].execute(interface);
  }
  else
  {
    /* Finalize the transfer */
    pinSet(interface->cs);
    ifSet(interface->interface, IF_RELEASE, 0);

    if (interface->callback)
      interface->callback(interface->callbackArgument);
  }
}
/*----------------------------------------------------------------------------*/
static enum result sdioInit(void *object, const void *configBase)
{
  const struct SdioSpiConfig * const config = configBase;
  struct SdioSpi * const interface = object;
  enum result res;

  interface->cs = pinInit(config->cs);
  if (!pinValid(interface->cs))
    return E_VALUE;
  pinOutput(interface->cs, 1);

  interface->interface = config->interface;

  res = ifSet(interface->interface, IF_ZEROCOPY, 0);
  if (res != E_OK)
    return res;

  res = ifCallback(interface->interface, interruptHandler, interface);
  if (res != E_OK)
    return res;

  interface->blockLength = 512; /* 512 bytes by default */
  interface->left = 0;
  interface->rxBuffer = 0;
  interface->txBuffer = 0;

  interface->argument = 0;
  interface->blocking = true;
  interface->callback = 0;
  interface->command = 0;
  interface->state = SDIO_SPI_STATE_IDLE;
  interface->status = E_OK;
  memset(interface->response, 0, sizeof(interface->response));

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void sdioDeinit(void *object __attribute__((unused)))
{

}
/*----------------------------------------------------------------------------*/
static enum result sdioCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct SdioSpi * const interface = object;

  interface->callbackArgument = argument;
  interface->callback = callback;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result sdioGet(void *object, enum ifOption option, void *data)
{
  struct SdioSpi * const interface = object;

  /* Additional options */
  switch ((enum sdioOption)option)
  {
    case IF_SDIO_MODE:
    {
      *(uint32_t *)data = SDIO_SPI;
      return E_OK;
    }

    case IF_SDIO_RESPONSE:
    {
      const enum sdioResponse response = COMMAND_RESP_VALUE(interface->command);

      if (response == SDIO_RESPONSE_NONE)
        return E_ERROR;

      if (response == SDIO_RESPONSE_SHORT)
        memcpy(data, interface->response, 4);
      else
        memcpy(data, interface->response, 16);
      return E_OK;
    }

    default:
      break;
  }

  switch (option)
  {
    case IF_STATUS:
      return interface->status;

    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static enum result sdioSet(void *object, enum ifOption option,
    const void *data)
{
  struct SdioSpi * const interface = object;

  /* Additional options */
  switch ((enum sdioOption)option)
  {
    case IF_SDIO_EXECUTE:
      execute(interface);
      return E_OK;

    case IF_SDIO_ARGUMENT:
      interface->argument = *(const uint32_t *)data;
      return E_OK;

    case IF_SDIO_COMMAND:
      interface->command = *(const uint32_t *)data;
      return E_OK;

    default:
      break;
  }

  switch (option)
  {
    default:
      return E_ERROR;
  }
}
/*----------------------------------------------------------------------------*/
static uint32_t sdioRead(void *object, uint8_t *buffer, uint32_t length)
{
  struct SdioSpi * const interface = object;

  /* Check buffer alignment */
  assert(!(length % interface->blockLength));

  interface->status = E_BUSY;
  ifSet(interface->interface, IF_ACQUIRE, 0);

  interface->left = length;
  interface->rxBuffer = buffer;

  /* Begin execution */
  interface->state = SDIO_SPI_STATE_SEND_CMD;
  stateTable[SDIO_SPI_STATE_SEND_CMD].execute(interface);

  return length;
}
/*----------------------------------------------------------------------------*/
static uint32_t sdioWrite(void *object, const uint8_t *buffer, uint32_t length)
{
  struct SdioSpi * const interface = object;

  /* Check buffer alignment */
  assert(!(length % interface->blockLength));

  return length;
}
