/*
 * gptimer_base.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <xcore/memory.h>
#include <halm/platform/nxp/gptimer_base.h>
#include <halm/platform/nxp/lpc11exx/clocking.h>
#include <halm/platform/nxp/lpc11exx/system.h>
/*----------------------------------------------------------------------------*/
/* Pack capture or match channel and pin function in one value */
#define PACK_VALUE(function, channel) (((channel) << 4) | (function))
/*----------------------------------------------------------------------------*/
struct TimerBlockDescriptor
{
  LPC_TIMER_Type *reg;
  uint8_t resolution;
  enum sysClockBranch clock;
};
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(uint8_t, const struct GpTimerBase *,
    struct GpTimerBase *);
/*----------------------------------------------------------------------------*/
static enum result tmrInit(void *, const void *);
static void tmrDeinit(void *);
/*----------------------------------------------------------------------------*/
static const struct EntityClass tmrTable = {
    .size = 0, /* Abstract class */
    .init = tmrInit,
    .deinit = tmrDeinit
};
/*----------------------------------------------------------------------------*/
static const struct TimerBlockDescriptor timerBlockEntries[] = {
    {
        .reg = LPC_TIMER16B0,
        .resolution = 16,
        .clock = CLK_CT16B0
    },
    {
        .reg = LPC_TIMER16B1,
        .resolution = 16,
        .clock = CLK_CT16B1
    },
    {
        .reg = LPC_TIMER32B0,
        .resolution = 32,
        .clock = CLK_CT32B0
    },
    {
        .reg = LPC_TIMER32B1,
        .resolution = 32,
        .clock = CLK_CT32B1
    }
};
/*----------------------------------------------------------------------------*/
const struct PinEntry gpTimerCapturePins[] = {
    {
        .key = PIN(0, 2), /* CT16B0_CAP0 */
        .channel = GPTIMER_CT16B0,
        .value = PACK_VALUE(2, 0)
    }, {
        .key = PIN(0, 12), /* CT32B1_CAP0 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(3, 0)
    }, {
        .key = PIN(0, 17), /* CT32B0_CAP0 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(2, 0)
    }, {
        .key = PIN(0, 20), /* CT16B1_CAP0 */
        .channel = GPTIMER_CT16B1,
        .value = PACK_VALUE(1, 0)
    }, {
        .key = PIN(1, 4), /* CT32B1_CAP0 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(1, 0)
    }, {
        .key = PIN(1, 5), /* CT32B1_CAP1 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(1, 1)
    }, {
        .key = PIN(1, 16), /* CT16B0_CAP0 */
        .channel = GPTIMER_CT16B0,
        .value = PACK_VALUE(2, 0)
    }, {
        .key = PIN(1, 17), /* CT16B0_CAP1 */
        .channel = GPTIMER_CT16B0,
        .value = PACK_VALUE(1, 2) /* Capture channel defined as channel 1 */
    }, {
        .key = PIN(1, 18), /* CT16B1_CAP1 */
        .channel = GPTIMER_CT16B1,
        .value = PACK_VALUE(1, 1)
    }, {
        .key = PIN(1, 28), /* CT32B0_CAP0 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(1, 0)
    }, {
        .key = PIN(1, 29), /* CT32B0_CAP1 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(2, 2) /* Capture channel defined as channel 1 */
    }, {
        .key = 0 /* End of pin function association list */
    }
};
/*----------------------------------------------------------------------------*/
const struct PinEntry gpTimerMatchPins[] = {
    {
        .key = PIN(0, 1), /* CT32B0_MAT2 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(2, 2)
    }, {
        .key = PIN(0, 8), /* CT16B0_MAT0 */
        .channel = GPTIMER_CT16B0,
        .value = PACK_VALUE(2, 0)
    }, {
        .key = PIN(0, 9), /* CT16B0_MAT1 */
        .channel = GPTIMER_CT16B0,
        .value = PACK_VALUE(2, 1)
    }, {
        .key = PIN(0, 10), /* CT16B0_MAT2 */
        .channel = GPTIMER_CT16B0,
        .value = PACK_VALUE(3, 2)
    }, {
        .key = PIN(0, 11), /* CT32B0_MAT3 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(3, 3)
    }, {
        .key = PIN(0, 13), /* CT32B1_MAT0 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(3, 0)
    }, {
        .key = PIN(0, 14), /* CT32B1_MAT1 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(3, 1)
    }, {
        .key = PIN(0, 15), /* CT32B1_MAT2 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(3, 2)
    }, {
        .key = PIN(0, 16), /* CT32B1_MAT3 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(2, 3)
    }, {
        .key = PIN(0, 18), /* CT32B0_MAT0 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(2, 0)
    }, {
        .key = PIN(0, 19), /* CT32B0_MAT1 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(2, 1)
    }, {
        .key = PIN(0, 21), /* CT16B1_MAT0 */
        .channel = GPTIMER_CT16B1,
        .value = PACK_VALUE(1, 0)
    }, {
        .key = PIN(0, 22), /* CT16B1_MAT1 */
        .channel = GPTIMER_CT16B1,
        .value = PACK_VALUE(2, 1)
    }, {
        .key = PIN(1, 0), /* CT32B1_MAT1 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(1, 1)
    }, {
        .key = PIN(1, 1), /* CT32B1_MAT1 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(1, 1)
    }, {
        .key = PIN(1, 2), /* CT32B1_MAT2 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(1, 2)
    }, {
        .key = PIN(1, 3), /* CT32B1_MAT3 */
        .channel = GPTIMER_CT32B1,
        .value = PACK_VALUE(1, 3)
    }, {
        .key = PIN(1, 13), /* CT16B0_MAT0 */
        .channel = GPTIMER_CT16B0,
        .value = PACK_VALUE(2, 0)
    }, {
        .key = PIN(1, 14), /* CT16B0_MAT1 */
        .channel = GPTIMER_CT16B0,
        .value = PACK_VALUE(2, 1)
    }, {
        .key = PIN(1, 15), /* CT16B0_MAT2 */
        .channel = GPTIMER_CT16B0,
        .value = PACK_VALUE(2, 2)
    }, {
        .key = PIN(1, 23), /* CT16B1_MAT1 */
        .channel = GPTIMER_CT16B1,
        .value = PACK_VALUE(1, 1)
    }, {
        .key = PIN(1, 24), /* CT32B0_MAT0 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(1, 0)
    }, {
        .key = PIN(1, 25), /* CT32B0_MAT1 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(1, 1)
    }, {
        .key = PIN(1, 26), /* CT32B0_MAT2 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(1, 2)
    }, {
        .key = PIN(1, 27), /* CT32B0_MAT3 */
        .channel = GPTIMER_CT32B0,
        .value = PACK_VALUE(1, 3)
    }, {
        .key = 0 /* End of pin function association list */
    }
};
/*----------------------------------------------------------------------------*/
const struct EntityClass * const GpTimerBase = &tmrTable;
static struct GpTimerBase *descriptors[4] = {0};
/*----------------------------------------------------------------------------*/
static enum result setDescriptor(uint8_t channel,
    const struct GpTimerBase *state, struct GpTimerBase *timer)
{
  assert(channel < ARRAY_SIZE(descriptors));

  return compareExchangePointer((void **)(descriptors + channel), state,
      timer) ? E_OK : E_BUSY;
}
/*----------------------------------------------------------------------------*/
void TIMER16B0_ISR(void)
{
  descriptors[0]->handler(descriptors[0]);
}
/*----------------------------------------------------------------------------*/
void TIMER16B1_ISR(void)
{
  descriptors[1]->handler(descriptors[1]);
}
/*----------------------------------------------------------------------------*/
void TIMER32B0_ISR(void)
{
  descriptors[2]->handler(descriptors[2]);
}
/*----------------------------------------------------------------------------*/
void TIMER32B1_ISR(void)
{
  descriptors[3]->handler(descriptors[3]);
}
/*----------------------------------------------------------------------------*/
uint32_t gpTimerGetClock(const struct GpTimerBase *timer
    __attribute__((unused)))
{
  return clockFrequency(MainClock);
}
/*----------------------------------------------------------------------------*/
static enum result tmrInit(void *object, const void *configBase)
{
  const struct GpTimerBaseConfig * const config = configBase;
  struct GpTimerBase * const timer = object;
  enum result res;

  timer->channel = config->channel;
  timer->handler = 0;

  /* Try to set peripheral descriptor */
  if ((res = setDescriptor(timer->channel, 0, timer)) != E_OK)
    return res;

  const struct TimerBlockDescriptor * const entry =
      &timerBlockEntries[timer->channel];

  sysClockEnable(entry->clock);

  timer->irq = TIMER16B0_IRQ + timer->channel;
  timer->reg = entry->reg;
  timer->resolution = entry->resolution;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void tmrDeinit(void *object)
{
  const struct GpTimerBase * const timer = object;

  sysClockDisable(timerBlockEntries[timer->channel].clock);
  setDescriptor(timer->channel, timer, 0);
}
