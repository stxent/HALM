/*
 * dma.c
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "dma.h"
/*----------------------------------------------------------------------------*/
/** Start DMA transmission.
 *  @param controller Pointer to Dma object.
 *  @param dest Destination memory address.
 *  @param src Source memory address.
 *  @param size Size of the transfer.
 *  @return Returns E_OK on success.
 */
enum result dmaStart(void *controller, void *dest, const void *src,
    uint32_t size)
{
  return ((struct DmaClass *)CLASS(controller))->start(controller,
      dest, src, size);
}
/*----------------------------------------------------------------------------*/
/** Start scatter-gather DMA transmission.
 *  @param controller Pointer to Dma object.
 *  @param first Pointer to first list descriptor.
 *  @return Returns E_OK on success.
 */
enum result dmaStartList(void *controller, const void *first)
{
  return ((struct DmaClass *)CLASS(controller))->startList(controller, first);
}
/*----------------------------------------------------------------------------*/
/** Disable a channel and lose data in the FIFO.
 *  @param controller Pointer to Dma object.
 */
void dmaStop(void *controller)
{
  ((struct DmaClass *)CLASS(controller))->stop(controller);
}
/*----------------------------------------------------------------------------*/
/** Disable a channel without losing data in the FIFO.
 *  @param controller Pointer to Dma object.
 */
void dmaHalt(void *controller)
{
  ((struct DmaClass *)CLASS(controller))->halt(controller);
}
/*----------------------------------------------------------------------------*/
/** Link next element to list in scatter-gather transmissions.
 *  @param controller Pointer to Dma object.
 *  @param current Pointer to current linked list item.
 *  @param next Pointer to next linked list item or zero on list end.
 *  @param dest Destination memory address.
 *  @param src Source memory address.
 *  @param size Size of the transfer.
 *  @return Returns E_OK on success.
 */
void dmaLinkItem(void *controller, void *current, void *next,
    void *dest, const void *src, uint16_t size)
{
  ((struct DmaClass *)CLASS(controller))->linkItem(controller, current, next,
      dest, src, size);
}
/*----------------------------------------------------------------------------*/
/** Check whether the channel is enabled or not.
 *  @param controller Pointer to Dma object.
 */
bool dmaIsActive(const struct Dma *dma)
{
  return dma->active;
}
