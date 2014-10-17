/*
 * platform/nxp/dac_dma.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef PLATFORM_NXP_DAC_DMA_H_
#define PLATFORM_NXP_DAC_DMA_H_
/*----------------------------------------------------------------------------*/
#include <interface.h>
#include <dma.h>
#include "dac_base.h"
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const DacDma;
/*----------------------------------------------------------------------------*/
struct DacDmaConfig
{
  /** Mandatory: conversion frequency. */
  uint32_t frequency;
  /** Mandatory: size of the single buffer in elements. */
  uint32_t size;
  /** Optional: initial output value. */
  uint16_t value;
  /** Mandatory: analog output. */
  pin_t pin;
  /** Mandatory: memory access channel. */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
struct DacDma
{
  struct DacBase parent;

  void (*callback)(void *);
  void *callbackArgument;

  struct Dma *dma;

  /* Internal data buffer containing two pages and four chunks */
  uint16_t *buffer;
  /* Size of the page */
  uint32_t size;
  /* Update flag indicating that the second buffer is ready */
  bool updated;
};
/*----------------------------------------------------------------------------*/
#endif /* PLATFORM_NXP_DAC_DMA_H_ */
