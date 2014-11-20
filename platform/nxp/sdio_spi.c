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
#include <platform/nxp/sdio_spi.h>
/*----------------------------------------------------------------------------*/
#define COMMAND_CODE_MASK               BIT_FIELD(MASK(6), 0)
#define COMMAND_CODE(value)             BIT_FIELD((value), 0)
#define COMMAND_CODE_VALUE(command) \
    FIELD_VALUE((command), COMMAND_CODE_MASK, 0)
#define COMMAND_DATA_MASK               BIT_FIELD(MASK(2), 6)
#define COMMAND_DATA(value)             BIT_FIELD((value), 6)
#define COMMAND_DATA_VALUE(command) \
    FIELD_VALUE((command), COMMAND_DATA_MASK, 6)
#define COMMAND_RESPONSE_MASK           BIT_FIELD(MASK(2), 8)
#define COMMAND_RESPONSE(value)         BIT_FIELD((value), 8)
#define COMMAND_RESPONSE_VALUE(command) \
    FIELD_VALUE((command), COMMAND_RESPONSE_MASK, 8)
#define COMMAND_FLAGS_MASK              BIT_FIELD(MASK(8), 10)
#define COMMAND_FLAGS(value)            BIT_FIELD((value), 10)
#define COMMAND_FLAGS_VALUE(command) \
    FIELD_VALUE((command), COMMAND_FLAGS_MASK, 10)
/*----------------------------------------------------------------------------*/
enum sdioResponseFlags
{
  FLAG_IDLE_STATE       = 0x01,
  FLAG_ERASE_RESET      = 0x02,
  FLAG_ILLEGAL_COMMAND  = 0x04,
  FLAG_CRC_ERROR        = 0x08,
  FLAG_ERASE_ERROR      = 0x10,
  FLAG_BAD_ADDRESS      = 0x20,
  FLAG_BAD_PARAMETER    = 0x40
};
/*----------------------------------------------------------------------------*/
static void execute(struct SdioSpi *);
static enum result getShortResponse(struct SdioSpi *, uint32_t *);
static enum result waitForData(struct SdioSpi *, uint8_t *);
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
static enum result acquireBus(struct SdioSpi *interface)
{
  ifSet(interface->interface, IF_ACQUIRE, 0);
  pinReset(interface->cs);
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void releaseBus(struct SdioSpi *interface)
{
  pinSet(interface->cs);
  ifSet(interface->interface, IF_RELEASE, 0);
}
/*----------------------------------------------------------------------------*/
static void execute(struct SdioSpi *interface)
{
  const uint32_t argument = toBigEndian32(interface->argument);
  const uint8_t code = COMMAND_CODE_VALUE(interface->command);
  const uint8_t flags = COMMAND_FLAGS_VALUE(interface->command);
  const enum sdioDataMode mode = COMMAND_DATA_VALUE(interface->command);
  const enum sdioResponse response = COMMAND_RESPONSE_VALUE(interface->command);
  uint32_t bytesWritten;
  enum result res;

  interface->status = E_BUSY;

  if (flags & SDIO_INITIALIZE)
  {
    /* Send initialization sequence */
    memset(interface->buffer, 0xFF, 10);

    ifSet(interface->interface, IF_ACQUIRE, 0);
    bytesWritten = ifWrite(interface->interface, interface->buffer, 10);
    ifSet(interface->interface, IF_RELEASE, 0);

    if (bytesWritten != 10)
    {
      interface->status = E_INTERFACE;
      return;
    }
  }

  /* Fill the buffer */
  interface->buffer[0] = 0xFF;
  interface->buffer[1] = 0x40 | code;
  memcpy(interface->buffer + 2, &argument, sizeof(argument));

  // TODO Remove hardcoded values
  /* Checksum should be valid only for first CMD0 and CMD8 commands */
  switch (code)
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

  acquireBus(interface);

  bytesWritten = ifWrite(interface->interface, interface->buffer, 8);
  if (bytesWritten != 8)
  {
    interface->status = E_INTERFACE;
    releaseBus(interface);
    return;
  }

  if (response == SDIO_RESPONSE_SHORT)
  {
    /* Wait for 32-bit response */
    interface->status = getShortResponse(interface, interface->response);
  }
  else
  {
    // TODO Add support for long responses
    uint8_t errors;

    if ((res = waitForData(interface, &errors)) == E_OK)
    {
      if (flags & SDIO_INITIALIZE)
      {
        interface->status = errors & FLAG_IDLE_STATE == FLAG_IDLE_STATE ?
            E_OK : E_DEVICE;
      }
      else
      {
        interface->status = !errors ? E_OK : E_DEVICE;
      }
    }
    else
      interface->status = res;
  }

  releaseBus(interface);
}
/*----------------------------------------------------------------------------*/
static enum result getShortResponse(struct SdioSpi *interface, uint32_t *value)
{
  uint8_t response;
  enum result res;

  if ((res = waitForData(interface, &response)) == E_OK)
  {
    const uint32_t bytesRead = ifRead(interface->interface,
        interface->buffer, 4);

    if (response)
      res = E_ERROR;
    else if (bytesRead != sizeof(interface->buffer))
      res = E_INTERFACE;

    /* Response comes in big-endian format */
    memcpy(value, interface->buffer, 4);
    *value = fromBigEndian32(*value);
  }

  return res;
}
/*----------------------------------------------------------------------------*/
static enum result waitForData(struct SdioSpi *interface, uint8_t *value)
{
  /* Response will come after 1..8 queries */
  const uint32_t bytesRead = ifRead(interface->interface, interface->buffer, 8);

  if (bytesRead != 8)
    return E_INTERFACE;

  for (uint8_t index = 0; index < 8; ++index)
  {
    if (interface->buffer[index] != 0xFF)
    {
      *value = interface->buffer[index];
      return E_OK;
    }
  }

  return E_BUSY;
}
/*----------------------------------------------------------------------------*/
uint32_t sdioPrepareCommand(uint8_t command, enum sdioDataMode dataMode,
    enum sdioResponse response, uint8_t flags)
{
  return COMMAND_CODE(command) | COMMAND_DATA((uint8_t)dataMode)
      | COMMAND_RESPONSE((uint8_t)response) | COMMAND_FLAGS(flags);
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

  interface->argument = 0;
  interface->command = 0;
  interface->interface = config->interface;
  interface->status = E_OK;
  memset(interface->response, 0, sizeof(interface->response));

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void sdioDeinit(void *object __attribute__((unused)))
{

}
/*----------------------------------------------------------------------------*/
static enum result sdioCallback(void *object __attribute__((unused)),
    void (*callback)(void *) __attribute__((unused)),
    void *argument __attribute__((unused)))
{
  return E_ERROR;
}
/*----------------------------------------------------------------------------*/
static enum result sdioGet(void *object, enum ifOption option, void *data)
{
  struct SdioSpi * const interface = object;

  /* Additional options */
  switch ((enum sdioOption)option)
  {
    case IF_SDIO_RESPONSE:
    {
      const enum sdioResponse response =
          COMMAND_RESPONSE_VALUE(interface->command);

      if (response == SDIO_RESPONSE_NONE)
        return E_ERROR;

      if (response == SDIO_RESPONSE_LONG)
        memcpy(data, interface->response, sizeof(interface->response));
      else
        *(uint32_t *)data = interface->response[0];
      break;
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

}
/*----------------------------------------------------------------------------*/
static uint32_t sdioWrite(void *object, const uint8_t *buffer, uint32_t length)
{
  struct SdioSpi * const interface = object;

}
