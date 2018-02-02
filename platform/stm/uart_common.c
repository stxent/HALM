/*
 * uart_common.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/platform/stm/uart_base.h>
#include <halm/platform/stm/uart_defs.h>
/*----------------------------------------------------------------------------*/
extern const struct PinEntry uartPins[];
/*----------------------------------------------------------------------------*/
void uartConfigPins(struct UartBase *interface,
    const struct UartBaseConfig *config)
{
  if (config->rx)
  {
    /* Configure UART RX pin */
    const struct PinEntry * const pinEntry = pinFind(uartPins,
        config->rx, interface->channel);
    assert(pinEntry);

    const struct Pin pin = pinInit(config->rx);

    pinInput(pin);
    pinSetFunction(pin, pinEntry->value);
  }

  if (config->tx)
  {
    /* Configure UART TX pin */
    const struct PinEntry * const pinEntry = pinFind(uartPins,
        config->tx, interface->channel);
    assert(pinEntry);

    const struct Pin pin = pinInit(config->tx);

    pinOutput(pin, true);
    pinSetFunction(pin, pinEntry->value);
  }
}
/*----------------------------------------------------------------------------*/
enum SerialParity uartGetParity(const struct UartBase *interface)
{
  const STM_USART_Type * const reg = interface->reg;
  const uint32_t value = reg->CR1;

  if (value & CR1_PCE)
    return (value & CR1_PS) ? SERIAL_PARITY_ODD : SERIAL_PARITY_EVEN;
  else
    return SERIAL_PARITY_NONE;
}
/*----------------------------------------------------------------------------*/
uint32_t uartGetRate(const struct UartBase *interface)
{
  const STM_USART_Type * const reg = interface->reg;
  const uint32_t value = reg->BRR;

  return value ? (uartGetClock(interface) / value) : 0;
}
/*----------------------------------------------------------------------------*/
void uartSetParity(struct UartBase *interface, enum SerialParity parity)
{
  STM_USART_Type * const reg = interface->reg;
  uint32_t value = reg->CR1 & ~(CR1_PS | CR1_PCE | CR1_M);

  if (parity != SERIAL_PARITY_NONE)
  {
    value |= CR1_PCE | CR1_M;
    if (parity == SERIAL_PARITY_ODD)
      value |= CR1_PS;
  }

  reg->CR1 = value;
}
/*----------------------------------------------------------------------------*/
void uartSetRate(struct UartBase *interface, uint32_t rate)
{
  STM_USART_Type * const reg = interface->reg;
  reg->BRR = rate ? ((uartGetClock(interface) + rate / 2) / rate) : 0;
}
