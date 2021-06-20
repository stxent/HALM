/*
 * halm/platform/lpc/i2s_dma.h
 * Copyright (C) 2015, 2021 xent
 * Project is distributed under the terms of the MIT License
 */

#ifndef HALM_PLATFORM_LPC_I2S_DMA_H_
#define HALM_PLATFORM_LPC_I2S_DMA_H_
/*----------------------------------------------------------------------------*/
#include <halm/dma.h>
#include <halm/platform/lpc/i2s_base.h>
#include <xcore/stream.h>
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const I2SDma;

struct I2SDmaHandler;

struct I2SDmaConfig
{
  /** Mandatory: sample rate for the receiver and the transmitter. */
  uint32_t rate;
  /** Mandatory: request queue size. */
  size_t size;
  /** Mandatory: word width. */
  enum I2SWidth width;

  struct
  {
    /** Mandatory: receive data. */
    PinNumber sda;
    /** Optional: receive clock. */
    PinNumber sck;
    /** Optional: receive word select. */
    PinNumber ws;
    /** Optional: master clock output. */
    PinNumber mclk;
    /** Mandatory: direct memory access channel. */
    uint8_t dma;
  } rx;

  struct
  {
    /** Mandatory: transmit data. */
    PinNumber sda;
    /** Optional: transmit clock. */
    PinNumber sck;
    /** Optional: transmit word select. */
    PinNumber ws;
    /** Optional: master clock output. */
    PinNumber mclk;
    /** Mandatory: direct memory access channel. */
    uint8_t dma;
  } tx;

  /** Optional: interrupt priority. */
  IrqPriority priority;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
  /** Optional: enable mono format. */
  bool mono;
  /** Optional: enable slave mode. */
  bool slave;
};

struct I2SDma
{
  struct I2SBase base;

  /* Input stream */
  struct I2SDmaHandler *rxStream;
  /* Output stream */
  struct I2SDmaHandler *txStream;
  /* DMA channel descriptors for receiving data */
  struct Dma *rxDma;
  /* DMA channel descriptors for transmitting data */
  struct Dma *txDma;

  /* Sample rate */
  uint32_t sampleRate;
  /* Size of the sample in power of two format */
  uint8_t sampleSize;
  /* Enable mono mode */
  bool mono;
  /* Enable slave mode */
  bool slave;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

struct Stream *i2sDmaGetInput(struct I2SDma *);
struct Stream *i2sDmaGetOutput(struct I2SDma *);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_LPC_I2S_DMA_H_ */
