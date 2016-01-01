/*
 * platform/nxp/i2s_dma.h
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef PLATFORM_NXP_I2S_DMA_H_
#define PLATFORM_NXP_I2S_DMA_H_
/*----------------------------------------------------------------------------*/
#include <dma.h>
#include <platform/nxp/i2s_base.h>
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const I2sDma;
/*----------------------------------------------------------------------------*/
struct I2sDmaConfig
{
  /** Mandatory: sample rate for the receiver and the transmitter. */
  uint32_t rate;
  /**
   * Mandatory: size of the single buffer in samples. When the stereo mode
   * is enabled, one sample consists of two values for left and right channels.
   */
  uint32_t size;

  struct
  {
    /** Mandatory: receive data. */
    pinNumber sda;
    /** Optional: receive clock. */
    pinNumber sck;
    /** Optional: receive word select. */
    pinNumber ws;
    /** Optional: master clock output. */
    pinNumber mclk;
    /** Mandatory: direct memory access channel. */
    uint8_t dma;
  } rx;

  struct
  {
    /** Mandatory: transmit data. */
    pinNumber sda;
    /** Optional: transmit clock. */
    pinNumber sck;
    /** Optional: transmit word select. */
    pinNumber ws;
    /** Optional: master clock output. */
    pinNumber mclk;
    /** Mandatory: direct memory access channel. */
    uint8_t dma;
  } tx;

  /** Optional: interrupt priority. */
  irqPriority priority;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
  /** Mandatory: word width. */
  enum i2sWidth width;
  /** Optional: enable mono format. */
  bool mono;
  /** Optional: enable slave mode. */
  bool slave;
};
/*----------------------------------------------------------------------------*/
struct I2sDma
{
  struct I2sBase parent;

  void (*callback)(void *);
  void *callbackArgument;

  /* Direct memory access channel descriptors */
  struct Dma *rxDma, *txDma;
  /* Sample rate */
  uint32_t sampleRate;
  /* Size of each buffer */
  uint32_t size;
  /* Word width */
  enum i2sWidth width;
  /* Mono format enabled flag */
  bool mono;
  /* Receiver enabled flag */
  bool rx;
  /* Transmitter enabled flag */
  bool tx;
};
/*----------------------------------------------------------------------------*/
#endif /* PLATFORM_NXP_I2S_DMA_H_ */
