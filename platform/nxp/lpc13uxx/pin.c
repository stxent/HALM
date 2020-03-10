/*
 * pin.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/lpc13uxx/pin_defs.h>
/*----------------------------------------------------------------------------*/
static volatile uint32_t *calcControlReg(struct PinData);
static void commonPinInit(struct Pin);
/*----------------------------------------------------------------------------*/
static volatile uint32_t *calcControlReg(struct PinData data)
{
  switch (data.port)
  {
    case 0:
      return &LPC_IOCON->PIO0[data.offset];

    case 1:
      return &LPC_IOCON->PIO1[data.offset];

    default:
      return 0;
  }
}
/*----------------------------------------------------------------------------*/
static void commonPinInit(struct Pin pin)
{
  pinSetFunction(pin, PIN_DEFAULT);
  pinSetPull(pin, PIN_NOPULL);
  pinSetType(pin, PIN_PUSHPULL);
}
/*----------------------------------------------------------------------------*/
struct Pin pinInit(PinNumber id)
{
  struct Pin pin;

  pin.data.port = PIN_TO_PORT(id);
  pin.data.offset = PIN_TO_OFFSET(id);
  pin.reg = id ? (void *)calcControlReg(pin.data) : 0;

  return pin;
}
/*----------------------------------------------------------------------------*/
void pinInput(struct Pin pin)
{
  commonPinInit(pin);
  LPC_GPIO->DIR[pin.data.port] &= ~(1UL << pin.data.offset);
}
/*----------------------------------------------------------------------------*/
void pinOutput(struct Pin pin, bool value)
{
  commonPinInit(pin);
  LPC_GPIO->DIR[pin.data.port] |= 1UL << pin.data.offset;
  pinWrite(pin, value);
}
/*----------------------------------------------------------------------------*/
void pinSetFunction(struct Pin pin, uint8_t function)
{
  volatile uint32_t * const reg = pin.reg;
  const uint32_t value = *reg;

  switch (function)
  {
    case PIN_DEFAULT:
      /* Some pins have default function value other than zero */
      if (pin.data.port == 0 && (pin.data.offset == 0
          || (pin.data.offset >= 10 && pin.data.offset <= 15)))
      {
        function = 1;
      }
      else
        function = 0;
      break;

    case PIN_ANALOG:
      *reg = value & ~IOCON_DIGITAL;
      return;
  }

  *reg = (value & ~IOCON_FUNC_MASK) | IOCON_FUNC(function);
}
/*----------------------------------------------------------------------------*/
void pinSetPull(struct Pin pin, enum PinPull pull)
{
  volatile uint32_t * const reg = pin.reg;
  uint32_t value = *reg & ~IOCON_MODE_MASK;

  switch (pull)
  {
    case PIN_NOPULL:
      value |= IOCON_MODE_INACTIVE;
      break;

    case PIN_PULLUP:
      value |= IOCON_MODE_PULLUP;
      break;

    case PIN_PULLDOWN:
      value |= IOCON_MODE_PULLDOWN;
      break;
  }

  *reg = value;
}
/*----------------------------------------------------------------------------*/
void pinSetType(struct Pin pin, enum PinType type)
{
  volatile uint32_t * const reg = pin.reg;

  switch (type)
  {
    case PIN_PUSHPULL:
      *reg &= ~IOCON_OD;
      break;

    case PIN_OPENDRAIN:
      *reg |= IOCON_OD;
      break;
  }
}