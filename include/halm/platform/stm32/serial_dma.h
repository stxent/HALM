/*
 * halm/platform/stm32/serial_dma.h
 * Copyright (C) 2021 xent
 * Project is distributed under the terms of the MIT License
 */

#ifndef HALM_PLATFORM_STM32_SERIAL_DMA_H_
#define HALM_PLATFORM_STM32_SERIAL_DMA_H_
/*----------------------------------------------------------------------------*/
#include <halm/generic/serial.h>
#include <halm/irq.h>
#include <halm/pin.h>
#include <xcore/interface.h>
#include <stdint.h>
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const SerialDma;

struct SerialDmaConfig
{
  /** Mandatory: size of the circular buffer. */
  size_t rxChunk;
  /** Mandatory: input queue size. */
  size_t rxLength;
  /** Mandatory: output queue size. */
  size_t txLength;
  /** Mandatory: baud rate. */
  uint32_t rate;
  /** Optional: parity bit setting. */
  enum SerialParity parity;
  /** Mandatory: serial input. */
  PinNumber rx;
  /** Mandatory: serial output. */
  PinNumber tx;
  /** Optional: interrupt priority. */
  IrqPriority priority;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
  /** Mandatory: number of the RX DMA stream. */
  uint8_t rxDma;
  /** Mandatory: number of the RX DMA stream. */
  uint8_t txDma;
};
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_STM32_SERIAL_DMA_H_ */
