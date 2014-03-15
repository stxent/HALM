/*
 * platform/nxp/one_wire.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef ONE_WIRE_H_
#define ONE_WIRE_H_
/*----------------------------------------------------------------------------*/
#include <byte_queue.h>
#include <irq.h>
#include "uart_base.h"
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass *OneWire;
/*----------------------------------------------------------------------------*/
struct OneWireConfig
{
  gpio_t rx, tx; /* Mandatory: RX and TX pins */
  priority_t priority; /* Optional: interrupt priority */
  uint8_t channel; /* Mandatory: peripheral identifier */
};
/*----------------------------------------------------------------------------*/
union OneWireAddress
{
  uint64_t rom;
  struct
  {
    uint8_t family;
    uint8_t serial[6];
    uint8_t crc;
  };
};
/*----------------------------------------------------------------------------*/
enum oneWireState
{
  OW_IDLE = 0,
  OW_RESET,
  OW_RECEIVE,
  OW_TRANSMIT,
  OW_ERROR
};
/*----------------------------------------------------------------------------*/
struct OneWire
{
  struct UartBase parent;

  void (*callback)(void *);
  void *callbackArgument;

  /* Pointer to an input buffer */
  uint8_t *rxBuffer;
  /* Address of the device */
  union OneWireAddress address;
  /* Number of bytes to be transmitted */
  uint8_t left;
  /* Position in a receiving word and temporary buffer for this word */
  uint8_t bit, word;
  /* Output queue containing command, address and data */
  struct ByteQueue txQueue;

  /* Computed data rates for reset and data transmission modes */
  struct UartRateConfig dataRate, resetRate;

  /* Current interface state */
  enum oneWireState state;
  /* Selection between blocking mode and zero copy mode */
  bool blocking;
};
/*----------------------------------------------------------------------------*/
#endif /* ONE_WIRE_H_ */
