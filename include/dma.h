/*
 * dma.h
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

/**
 * @file
 * Abstract Direct Memory Access interface for embedded systems.
 */

#ifndef DMA_H_
#define DMA_H_
/*----------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <entity.h>
#include <error.h>
/*----------------------------------------------------------------------------*/
/** DMA burst transfer size. */
enum dmaBurst
{
  DMA_BURST_1 = 0,
  DMA_BURST_2,
  DMA_BURST_4,
  DMA_BURST_8,
  DMA_BURST_16,
  DMA_BURST_32,
  DMA_BURST_64,
  DMA_BURST_128,
  DMA_BURST_256,
  DMA_BURST_512,
  DMA_BURST_1024
};
/*----------------------------------------------------------------------------*/
/** DMA transfer width. */
enum dmaWidth
{
  DMA_WIDTH_BYTE = 0,
  DMA_WIDTH_HALFWORD,
  DMA_WIDTH_WORD,
  DMA_WIDTH_DOUBLEWORD
};
/*----------------------------------------------------------------------------*/
/* Class descriptor */
struct DmaClass
{
  CLASS_HEADER

  bool (*active)(void *);
  void (*callback)(void *, void (*)(void *), void *);
  enum result (*start)(void *, void *, const void *, uint32_t);
  void (*stop)(void *);

  /* Operations with buffer lists */
  void *(*allocate)(void *, uint32_t, bool);
  enum result (*execute)(void *, const void *);
};
/*----------------------------------------------------------------------------*/
struct Dma
{
  struct Entity parent;
};
/*----------------------------------------------------------------------------*/
/**
 * Check whether the channel is enabled or not.
 * @param channel Pointer to a Dma object.
 * @return @b true when the transmission is active or @b false otherwise.
 */
static inline bool dmaActive(void *channel)
{
  return ((struct DmaClass *)CLASS(channel))->active(channel);
}
/*----------------------------------------------------------------------------*/
/**
 * Set callback function for the transmission completion event.
 * @param channel Pointer to a Dma object.
 * @param callback Callback function.
 * @param argument Callback function argument.
 */
static inline void dmaCallback(void *channel, void (*callback)(void *),
    void *argument)
{
  ((struct DmaClass *)CLASS(channel))->callback(channel, callback, argument);
}
/*----------------------------------------------------------------------------*/
/**
 * Start the transfer.
 * @param channel Pointer to a Dma object.
 * @param destination Destination memory address.
 * @param source Source memory address.
 * @param size Transfer size in number of transfers.
 * @return @b E_OK on success.
 */
static inline enum result dmaStart(void *channel, void *destination,
    const void *source, uint32_t size)
{
  return ((struct DmaClass *)CLASS(channel))->start(channel, destination,
      source, size);
}
/*----------------------------------------------------------------------------*/
/**
 * Disable the channel.
 * @param channel Pointer to a Dma object.
 */
static inline void dmaStop(void *channel)
{
  ((struct DmaClass *)CLASS(channel))->stop(channel);
}
/*----------------------------------------------------------------------------*/
/**
 * Allocate memory for a buffer list.
 * @param channel Pointer to a Dma object.
 * @param size Size of the list in elements.
 * @param circular Set @b true to create a circular buffer list.
 * @return Pointer to an allocated list on success or zero otherwise.
 */
static inline void *dmaAllocate(void *channel, uint32_t size, bool circular)
{
  return ((struct DmaClass *)CLASS(channel))->allocate(channel, size, circular);
}
/*----------------------------------------------------------------------------*/
/**
 * Start the scatter-gather transfer.
 * @param channel Pointer to a Dma object.
 * @param list Pointer to a buffer list.
 * @return @b E_OK on success.
 */
static inline enum result dmaExecute(void *channel, const void *list)
{
  return ((struct DmaClass *)CLASS(channel))->execute(channel, list);
}
/*----------------------------------------------------------------------------*/
#endif /* DMA_H_ */
