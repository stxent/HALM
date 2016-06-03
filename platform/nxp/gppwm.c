/*
 * gppwm.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/irq.h>
#include <halm/platform/nxp/gppwm.h>
#include <halm/platform/nxp/gppwm_defs.h>
/*----------------------------------------------------------------------------*/
#define UNPACK_CHANNEL(value)   (((value) >> 4) & 0x0F)
#define UNPACK_FUNCTION(value)  ((value) & 0x0F)
/*----------------------------------------------------------------------------*/
static inline volatile uint32_t *calcMatchChannel(LPC_PWM_Type *, uint8_t);
static uint8_t configMatchPin(uint8_t channel, pinNumber key);
/*----------------------------------------------------------------------------*/
static enum result unitAllocateChannel(struct GpPwmUnit *, uint8_t);
static void unitReleaseChannel(struct GpPwmUnit *, uint8_t);
static enum result unitInit(void *, const void *);
static void unitDeinit(void *);
/*----------------------------------------------------------------------------*/
static void channelSetEnabled(struct GpPwmUnit *, uint8_t, bool);
static enum result channelSetFrequency(struct GpPwmUnit *, uint32_t);
/*----------------------------------------------------------------------------*/
static enum result singleEdgeInit(void *, const void *);
static void singleEdgeDeinit(void *);
static uint32_t singleEdgeGetResolution(const void *);
static void singleEdgeSetDuration(void *, uint32_t);
static void singleEdgeSetEdges(void *, uint32_t, uint32_t);
static void singleEdgeSetEnabled(void *, bool);
static enum result singleEdgeSetFrequency(void *, uint32_t);
/*----------------------------------------------------------------------------*/
static enum result doubleEdgeInit(void *, const void *);
static void doubleEdgeDeinit(void *);
static uint32_t doubleEdgeGetResolution(const void *);
static void doubleEdgeSetDuration(void *, uint32_t);
static void doubleEdgeSetEdges(void *, uint32_t, uint32_t);
static void doubleEdgeSetEnabled(void *, bool);
static enum result doubleEdgeSetFrequency(void *, uint32_t);
/*----------------------------------------------------------------------------*/
static const struct EntityClass unitTable = {
    .size = sizeof(struct GpPwmUnit),
    .init = unitInit,
    .deinit = unitDeinit
};
/*----------------------------------------------------------------------------*/
static const struct PwmClass singleEdgeTable = {
    .size = sizeof(struct GpPwm),
    .init = singleEdgeInit,
    .deinit = singleEdgeDeinit,

    .getResolution = singleEdgeGetResolution,
    .setDuration = singleEdgeSetDuration,
    .setEdges = singleEdgeSetEdges,
    .setEnabled = singleEdgeSetEnabled,
    .setFrequency = singleEdgeSetFrequency
};
/*----------------------------------------------------------------------------*/
static const struct PwmClass doubleEdgeTable = {
    .size = sizeof(struct GpPwmDoubleEdge),
    .init = doubleEdgeInit,
    .deinit = doubleEdgeDeinit,

    .getResolution = doubleEdgeGetResolution,
    .setDuration = doubleEdgeSetDuration,
    .setEdges = doubleEdgeSetEdges,
    .setEnabled = doubleEdgeSetEnabled,
    .setFrequency = doubleEdgeSetFrequency
};
/*----------------------------------------------------------------------------*/
extern const struct PinEntry gpPwmPins[];
const struct EntityClass * const GpPwmUnit = &unitTable;
const struct PwmClass * const GpPwm = &singleEdgeTable;
const struct PwmClass * const GpPwmDoubleEdge = &doubleEdgeTable;
/*----------------------------------------------------------------------------*/
static inline volatile uint32_t *calcMatchChannel(LPC_PWM_Type *device,
    uint8_t channel)
{
  assert(channel && channel <= 6);

  return channel <= 3 ?
      (&device->MR1 + (channel - 1)) : (&device->MR4 + (channel - 4));
}
/*----------------------------------------------------------------------------*/
static uint8_t configMatchPin(uint8_t channel, pinNumber key)
{
  const struct PinEntry * const pinEntry = pinFind(gpPwmPins, key, channel);
  assert(pinEntry);

  const struct Pin pin = pinInit(key);

  pinOutput(pin, 0);
  pinSetFunction(pin, UNPACK_FUNCTION(pinEntry->value));

  return UNPACK_CHANNEL(pinEntry->value);
}
/*----------------------------------------------------------------------------*/
static enum result unitAllocateChannel(struct GpPwmUnit *unit, uint8_t channel)
{
  enum result res = E_BUSY;
  const uint8_t mask = 1 << channel;
  const irqState state = irqSave();

  if (!(unit->matches & mask))
  {
    unit->matches |= mask;
    res = E_OK;
  }

  irqRestore(state);
  return res;
}
/*----------------------------------------------------------------------------*/
static void unitReleaseChannel(struct GpPwmUnit *unit, uint8_t channel)
{
  const irqState state = irqSave();

  unit->matches &= ~(1 << channel);

  irqRestore(state);
}
/*----------------------------------------------------------------------------*/
static enum result unitInit(void *object, const void *configBase)
{
  const struct GpPwmUnitConfig * const config = configBase;
  const struct GpPwmUnitBaseConfig baseConfig = {
      .channel = config->channel
  };
  struct GpPwmUnit * const unit = object;
  enum result res;

  const uint32_t clockFrequency = gpPwmGetClock(object);
  const uint32_t timerFrequency = config->frequency * config->resolution;

  assert(timerFrequency && timerFrequency <= clockFrequency);

  /* Call base class constructor */
  if ((res = GpPwmUnitBase->init(object, &baseConfig)) != E_OK)
    return res;

  unit->matches = 0;
  unit->resolution = config->resolution;

  LPC_PWM_Type * const reg = unit->base.reg;

  reg->TCR = 0;

  reg->IR = reg->IR; /* Clear pending interrupts */
  reg->PC = reg->TC = 0;
  reg->CTCR = 0;
  reg->CCR = 0;
  reg->PCR = 0;

  /* Configure timings */
  reg->PR = clockFrequency / timerFrequency - 1;
  reg->MR0 = unit->resolution;
  reg->MCR = MCR_RESET(0);

  /* Enable timer and PWM mode */
  reg->TCR = TCR_CEN | TCR_PWM_ENABLE;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void unitDeinit(void *object)
{
  struct GpPwmUnit * const unit = object;
  LPC_PWM_Type * const reg = unit->base.reg;

  reg->TCR &= ~(TCR_CEN | TCR_PWM_ENABLE);
  GpPwmUnitBase->deinit(unit);
}
/*----------------------------------------------------------------------------*/
static void channelSetEnabled(struct GpPwmUnit *unit, uint8_t channel,
    bool state)
{
  LPC_PWM_Type * const reg = unit->base.reg;

  if (state)
    reg->PCR |= PCR_OUTPUT_ENABLED(channel);
  else
    reg->PCR &= ~PCR_OUTPUT_ENABLED(channel);
}
/*----------------------------------------------------------------------------*/
static enum result channelSetFrequency(struct GpPwmUnit *unit,
    uint32_t frequency)
{
  LPC_PWM_Type * const reg = unit->base.reg;

  const uint32_t clockFrequency = gpPwmGetClock((struct GpPwmUnitBase *)unit);
  const uint32_t timerFrequency = frequency * unit->resolution;

  if (!timerFrequency || timerFrequency > clockFrequency)
    return E_VALUE;

  /* TODO Add scaling of PWM match values */
  reg->PR = clockFrequency / timerFrequency - 1;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum result singleEdgeInit(void *object, const void *configBase)
{
  const struct GpPwmConfig * const config = configBase;
  struct GpPwm * const pwm = object;
  enum result res;

  /* Initialize output pin */
  const uint8_t channel = configMatchPin(config->parent->base.channel,
      config->pin);

  /* Allocate channels */
  if ((res = unitAllocateChannel(config->parent, channel)) != E_OK)
    return res;

  pwm->channel = channel;
  pwm->unit = config->parent;

  LPC_PWM_Type * const reg = pwm->unit->base.reg;

  /* Calculate pointer to match register for fast access */
  pwm->value = calcMatchChannel(reg, channel);

  /* Set initial duration */
  singleEdgeSetDuration(pwm, 0);

  /* Select single edge mode */
  reg->PCR &= ~PCR_DOUBLE_EDGE(channel);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void singleEdgeDeinit(void *object)
{
  struct GpPwm * const pwm = object;
  LPC_PWM_Type * const reg = pwm->unit->base.reg;

  reg->PCR &= ~PCR_OUTPUT_ENABLED(pwm->channel);
  unitReleaseChannel(pwm->unit, pwm->channel);
}
/*----------------------------------------------------------------------------*/
static uint32_t singleEdgeGetResolution(const void *object)
{
  return ((const struct GpPwm *)object)->unit->resolution;
}
/*----------------------------------------------------------------------------*/
static void singleEdgeSetDuration(void *object, uint32_t duration)
{
  struct GpPwm * const pwm = object;
  LPC_PWM_Type * const reg = pwm->unit->base.reg;
  const uint32_t resolution = pwm->unit->resolution;

  if (duration >= resolution)
  {
    /*
     * If match register is set to a value greater than resolution,
     * than output stays high during all cycle.
     */
    duration = resolution + 1;
  }

  *pwm->value = duration;
  reg->LER |= LER_ENABLE_LATCH(pwm->channel);
}
/*----------------------------------------------------------------------------*/
static void singleEdgeSetEdges(void *object,
    uint32_t leading __attribute__((unused)), uint32_t trailing)
{
  /* Leading edge time is constant in the single edge mode */
  assert(leading == 0);

  singleEdgeSetDuration(object, trailing);
}
/*----------------------------------------------------------------------------*/
static void singleEdgeSetEnabled(void *object, bool state)
{
  struct GpPwm * const pwm = object;

  channelSetEnabled(pwm->unit, pwm->channel, state);
}
/*----------------------------------------------------------------------------*/
static enum result singleEdgeSetFrequency(void *object, uint32_t frequency)
{
  struct GpPwm * const pwm = object;

  return channelSetFrequency(pwm->unit, frequency);
}
/*----------------------------------------------------------------------------*/
static enum result doubleEdgeInit(void *object, const void *configBase)
{
  const struct GpPwmDoubleEdgeConfig * const config = configBase;
  struct GpPwmDoubleEdge * const pwm = object;
  enum result res;

  /* Initialize output pin */
  const uint8_t channel = configMatchPin(config->parent->base.channel,
      config->pin);
  assert(channel > 1); /* First channel cannot be a double edged output */

  /* Allocate channels */
  if ((res = unitAllocateChannel(config->parent, channel - 1)) != E_OK)
    return res;
  if ((res = unitAllocateChannel(config->parent, channel)) != E_OK)
    return res;

  pwm->channel = channel;
  pwm->unit = config->parent;

  LPC_PWM_Type * const reg = pwm->unit->base.reg;

  /* Setup channels and initial edge times */
  pwm->leading = calcMatchChannel(reg, channel - 1);
  pwm->trailing = calcMatchChannel(reg, channel);

  /* Set initial values */
  doubleEdgeSetEdges(pwm, 0, 0);

  /* Select double edge mode */
  reg->PCR |= PCR_DOUBLE_EDGE(channel);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void doubleEdgeDeinit(void *object)
{
  struct GpPwmDoubleEdge * const pwm = object;
  LPC_PWM_Type * const reg = pwm->unit->base.reg;

  reg->PCR &= ~PCR_OUTPUT_ENABLED(pwm->channel);
  unitReleaseChannel(pwm->unit, pwm->channel);
  unitReleaseChannel(pwm->unit, pwm->channel - 1);
}
/*----------------------------------------------------------------------------*/
static uint32_t doubleEdgeGetResolution(const void *object)
{
  return ((const struct GpPwmDoubleEdge *)object)->unit->resolution;
}
/*----------------------------------------------------------------------------*/
static void doubleEdgeSetDuration(void *object, uint32_t duration)
{
  struct GpPwmDoubleEdge * const pwm = object;
  LPC_PWM_Type * const reg = pwm->unit->base.reg;
  const uint32_t resolution = pwm->unit->resolution;

  uint32_t center;
  uint32_t leading = *pwm->leading;
  uint32_t trailing = *pwm->trailing;

  if (leading > trailing)
  {
    const uint32_t half = resolution / 2;

    center = trailing + (leading - trailing) / 2;
    center = center < half ? center + half : center - half;
  }
  else
  {
    center = leading;
    if (trailing < resolution)
      center += (trailing - leading) / 2;
  }

  if (duration >= resolution)
  {
    leading = center;
    trailing = resolution + 1;
  }
  else
  {
    duration = duration / 2;

    const uint32_t negOffset = center - duration;
    const uint32_t posOffset = center + duration;

    leading = center >= duration ? negOffset : negOffset + resolution;
    trailing = posOffset < resolution ? posOffset : posOffset - resolution;
  }

  *pwm->leading = leading;
  *pwm->trailing = trailing;
  reg->LER |= LER_ENABLE_LATCH(pwm->channel)
      | LER_ENABLE_LATCH(pwm->channel - 1);
}
/*----------------------------------------------------------------------------*/
static void doubleEdgeSetEdges(void *object, uint32_t leading,
    uint32_t trailing)
{
  struct GpPwmDoubleEdge * const pwm = object;
  const uint32_t resolution = pwm->unit->resolution;
  LPC_PWM_Type * const reg = pwm->unit->base.reg;

  assert(leading < resolution);

  if (trailing >= resolution)
    trailing = resolution + 1;

  *pwm->leading = leading;
  *pwm->trailing = trailing;
  reg->LER |= LER_ENABLE_LATCH(pwm->channel)
      | LER_ENABLE_LATCH(pwm->channel - 1);
}
/*----------------------------------------------------------------------------*/
static void doubleEdgeSetEnabled(void *object, bool state)
{
  struct GpPwmDoubleEdge * const pwm = object;

  channelSetEnabled(pwm->unit, pwm->channel, state);
}
/*----------------------------------------------------------------------------*/
static enum result doubleEdgeSetFrequency(void *object, uint32_t frequency)
{
  struct GpPwmDoubleEdge * const pwm = object;

  return channelSetFrequency(pwm->unit, frequency);
}
/*----------------------------------------------------------------------------*/
/**
 * Create single edge PWM channel.
 * @param unit Pointer to a GpPwmUnit object.
 * @param pin Pin used as a signal output.
 * @return Pointer to a new Pwm object on success or zero on error.
 */
void *gpPwmCreate(void *unit, pinNumber pin)
{
  const struct GpPwmConfig channelConfig = {
      .parent = unit,
      .pin = pin
  };

  return init(GpPwm, &channelConfig);
}
/*----------------------------------------------------------------------------*/
/**
 * Create double edge PWM channel.
 * @param unit Pointer to a GpPwmUnit object.
 * @param pin Pin used as a signal output.
 * @return Pointer to a new Pwm object on success or zero on error.
 */
void *gpPwmCreateDoubleEdge(void *unit, pinNumber pin)
{
  const struct GpPwmDoubleEdgeConfig channelConfig = {
      .parent = unit,
      .pin = pin
  };

  return init(GpPwmDoubleEdge, &channelConfig);
}
