/*
 * dma.c
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "lpc17xx_defs.h"
#include "lpc17xx_sys.h"
#include "dma.h"
#include "dma_defs.h"
#include "mutex.h"
/*----------------------------------------------------------------------------*/
#define CHANNEL_COUNT                   8
/*----------------------------------------------------------------------------*/
static inline LPC_GPDMACH_TypeDef *calcChannel(uint8_t);
static void setMux(struct Dma *, enum dmaLine);
/*----------------------------------------------------------------------------*/
static enum result dmaInit(void *, const void *);
static void dmaDeinit(void *);
/*----------------------------------------------------------------------------*/
static const struct DmaClass dmaTable = {
    .size = sizeof(struct Dma),
    .init = dmaInit,
    .deinit = dmaDeinit
};
/*----------------------------------------------------------------------------*/
const struct DmaClass *Dma = &dmaTable;
/*----------------------------------------------------------------------------*/
static void * volatile descriptors[] = {0, 0, 0, 0, 0, 0, 0, 0};
static Mutex lock = MUTEX_UNLOCKED;
/* Initialized descriptors count */
static uint16_t instances = 0;
/*----------------------------------------------------------------------------*/
enum result dmaSetDescriptor(uint8_t channel, void *descriptor)
{
  enum result res = E_ERROR;

  mutexLock(&lock);
  if (!descriptors[channel])
  {
    descriptors[channel] = descriptor;
    res = E_OK;
  }
  mutexUnlock(&lock);
  return res;
}
/*----------------------------------------------------------------------------*/
void DMA_IRQHandler(void)
{
  uint8_t counter = 0;
  uint32_t stat = LPC_GPDMA->DMACIntTCStat;

  for (; counter < CHANNEL_COUNT; counter++)
  {
    if (stat & (1 << counter))
    {
      if (descriptors[counter])
      {
        /* Clear channel descriptor */
        ((struct Dma **)descriptors)[counter]->active = false;
        ((struct Dma **)descriptors)[counter] = 0;
      }
      /* Clear terminal count interrupt flag */
      LPC_GPDMA->DMACIntTCClear = 1 << counter;
    }
  }
  LPC_GPDMA->DMACIntErrClr = (1 << CHANNEL_COUNT) - 1;
}
/*----------------------------------------------------------------------------*/
static inline LPC_GPDMACH_TypeDef *calcChannel(uint8_t channel)
{
  return (LPC_GPDMACH_TypeDef *)((void *)LPC_GPDMACH0 +
      ((void *)LPC_GPDMACH1 - (void *)LPC_GPDMACH0) * channel);
}
/*----------------------------------------------------------------------------*/
static void setMux(struct Dma *device, enum dmaLine line)
{
  uint8_t pos;

  if (line >= DMA_LINE_UART0_TX && line != DMA_LINE_MEMORY)
  {
    pos = (line >> 2) - 8;
    device->muxMask &= ~(1 << pos);
    device->muxValue |= (line & 1) << pos;
  }
}
/*----------------------------------------------------------------------------*/
static enum result dmaInit(void *object, const void *configPtr)
{
  const struct DmaConfig *config = configPtr;
  struct Dma *device = object;

  if (!config || config->channel >= CHANNEL_COUNT)
    return E_ERROR;

  /* TODO Add channel allocation, when channel is -1 */
  device->channel = (uint8_t)config->channel;
  device->active = false;
  device->direction = config->direction;
  device->reg = calcChannel(device->channel);

  device->control = C_CONTROL_INT |
      C_CONTROL_SRC_BURST(config->burst) | C_CONTROL_DEST_BURST(config->burst) |
      C_CONTROL_SRC_WIDTH(config->width) | C_CONTROL_DEST_WIDTH(config->width);
  if (config->source.increment)
    device->control |= C_CONTROL_SRC_INC;
  if (config->destination.increment)
    device->control |= C_CONTROL_DEST_INC;

  device->config = C_CONFIG_TYPE(config->direction) |
      C_CONFIG_IE | C_CONFIG_ITC;
  if (config->source.line != DMA_LINE_MEMORY)
    device->config |= C_CONFIG_SRC_PERIPH(config->source.line);
  if (config->destination.line != DMA_LINE_MEMORY)
    device->config |= C_CONFIG_DEST_PERIPH(config->destination.line);

  /* Reset multiplexer register values */
  device->muxMask = 0xFF;
  device->muxValue = 0x00;
  /* Calculate new mask and value */
  setMux(device, config->source.line);
  setMux(device, config->destination.line);

  mutexLock(&lock);
  if (!instances)
  {
    LPC_SC->PCONP |= PCONP_PCGPDMA;
    LPC_GPDMA->DMACConfig |= DMA_ENABLE;
    NVIC_EnableIRQ(DMA_IRQn);
    //TODO add priority config
    //NVIC_SetPriority(device->irq, GET_PRIORITY(config->priority));
  }
  instances++;
  mutexUnlock(&lock);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void dmaDeinit(void *object)
{
  struct Dma *device = object;

  mutexLock(&lock);
  instances--;
  /* Disable DMA peripheral when no active descriptors exist */
  if (!instances)
  {
    NVIC_DisableIRQ(DMA_IRQn);
    LPC_GPDMA->DMACConfig &= ~DMA_ENABLE;
    LPC_SC->PCONP &= ~PCONP_PCGPDMA;
  }
  mutexUnlock(&lock);
}
/*----------------------------------------------------------------------------*/
enum result dmaStart(struct Dma *device, void *dest, const void *src,
    uint16_t size)
{
  if (dmaSetDescriptor(device->channel, device) != E_OK)
    return E_ERROR;

  LPC_SC->DMAREQSEL &= device->muxMask;
  LPC_SC->DMAREQSEL |= device->muxValue;

  device->reg->DMACCSrcAddr = (uint32_t)src;
  device->reg->DMACCDestAddr = (uint32_t)dest;

  /* Transfer size is 12-bit width value */
  device->reg->DMACCControl = device->control | C_CONTROL_SIZE(size);

  device->reg->DMACCLLI = 0;
  device->reg->DMACCConfig = device->config;

  /* Clear interrupt requests */
  LPC_GPDMA->DMACIntTCClear |= 1 << device->channel;
  LPC_GPDMA->DMACIntErrClr |= 1 << device->channel;

  device->active = true;
  device->reg->DMACCConfig |= C_CONFIG_ENABLE;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
enum result dmaStartList(struct Dma *device, struct DmaListItem *first)
{
  if (dmaSetDescriptor(device->channel, device) != E_OK)
    return E_ERROR;

  LPC_SC->DMAREQSEL &= device->muxMask;
  LPC_SC->DMAREQSEL |= device->muxValue;

  device->reg->DMACCSrcAddr = first->source;
  device->reg->DMACCDestAddr = first->destination;
  device->reg->DMACCControl = first->control;
  device->reg->DMACCLLI = first->next;

  device->reg->DMACCConfig = device->config;

  /* Clear interrupt requests */
  LPC_GPDMA->DMACIntTCClear |= 1 << device->channel;
  LPC_GPDMA->DMACIntErrClr |= 1 << device->channel;

  device->active = true;
  device->reg->DMACCConfig |= C_CONFIG_ENABLE;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
void dmaStop(struct Dma *device)
{
  /* Disable channel */
  device->reg->DMACCConfig &= ~C_CONFIG_ENABLE;
}
/*----------------------------------------------------------------------------*/
void dmaHalt(struct Dma *device)
{
  /* Ignore future requests */
  device->reg->DMACCConfig |= C_CONFIG_HALT;
  //FIXME Clear enable bit?
}
/*----------------------------------------------------------------------------*/
uint16_t dmaGetCount(const struct Dma *device)
{
  /*
   * Reading transfer size when the channel is active does not give
   * useful information
   */
  //FIXME Check work with linked lists
  if (!dmaIsActive(device))
  {
    /* Return transferred bytes count */
    return device->reg->DMACCControl & C_CONTROL_SIZE_MASK;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
bool dmaIsActive(const struct Dma *device)
{
  return device->active;
}
/*----------------------------------------------------------------------------*/
/* Scatter-gather support */
enum result dmaLinkItem(struct Dma *device, struct DmaListItem *current,
    struct DmaListItem *next, void *dest, const void *src, uint16_t size)
{
  current->source = (uint32_t)src;
  current->destination = (uint32_t)dest;
  current->next = (uint32_t)next;
  current->control = device->control | C_CONTROL_SIZE(size);
}
