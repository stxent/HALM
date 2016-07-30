/*
 * halm/platform/nxp/gpdma.h
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_GPDMA_H_
#define HALM_PLATFORM_NXP_GPDMA_H_
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
#include <halm/platform/nxp/gpdma_base.h>
/*----------------------------------------------------------------------------*/
extern const struct DmaClass * const GpDma;
/*----------------------------------------------------------------------------*/
struct GpDmaConfig
{
  /** Mandatory: request connection to the peripheral or memory. */
  enum gpDmaEvent event;
  /** Mandatory: transfer type. */
  enum gpDmaType type;
  /** Mandatory: channel number. */
  uint8_t channel;
};
/*----------------------------------------------------------------------------*/
struct GpDma
{
  struct GpDmaBase base;

  void (*callback)(void *);
  void *callbackArgument;

  /* Control register value */
  uint32_t control;
  /* The destination address of the data to be transferred */
  uintptr_t destination;
  /* The source address of the data */
  uintptr_t source;

  /* State of the transfer */
  uint8_t state;
};
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_GPDMA_H_ */
