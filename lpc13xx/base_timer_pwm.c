/*
 * base_timer.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include "base_timer_defs.h"
#include "base_timer_pwm.h"
#include "gpio.h"
/*----------------------------------------------------------------------------*/
struct BaseTimerPwmChannel
{
  struct Pwm parent;

  struct BaseTimerPwm *controller;
  uint32_t *reg;
  struct Gpio pin;
  uint8_t channel; /* Match channel */
};
/*----------------------------------------------------------------------------*/
/* Pack match channel and pin function in one value */
#define PACK_VALUE(function, channel)   (((channel) << 4) | (function))
/* Unpack function */
#define UNPACK_FUNCTION(value)          ((value) & 0x0F)
/* Unpack match channel */
#define UNPACK_CHANNEL(value)           (((value) >> 4) & 0x0F)
/*----------------------------------------------------------------------------*/
static const struct GpioDescriptor pwmPins[] = {
    {
        .key = GPIO_TO_PIN(0, 1), /* CT32B0_MAT2 */
        .channel = TIMER_CT32B0,
        .value = PACK_VALUE(2, 2)
    }, {
        .key = GPIO_TO_PIN(0, 8), /* CT16B0_MAT0 */
        .channel = TIMER_CT16B0,
        .value = PACK_VALUE(2, 0)
    }, {
        .key = GPIO_TO_PIN(0, 9), /* CT16B0_MAT1 */
        .channel = TIMER_CT16B0,
        .value = PACK_VALUE(2, 1)
    }, {
        .key = GPIO_TO_PIN(0, 10), /* CT16B0_MAT2 */
        .channel = TIMER_CT16B0,
        .value = PACK_VALUE(3, 2)
    }, {
        .key = GPIO_TO_PIN(0, 11), /* CT32B0_MAT3 */
        .channel = TIMER_CT32B0,
        .value = PACK_VALUE(3, 3)
    }, {
        .key = GPIO_TO_PIN(1, 1), /* CT32B1_MAT0 */
        .channel = TIMER_CT32B1,
        .value = PACK_VALUE(3, 0)
    }, {
        .key = GPIO_TO_PIN(1, 2), /* CT32B1_MAT1 */
        .channel = TIMER_CT32B1,
        .value = PACK_VALUE(3, 1)
    }, {
        .key = GPIO_TO_PIN(1, 3), /* CT32B1_MAT2 */
        .channel = TIMER_CT32B1,
        .value = PACK_VALUE(3, 2)
    }, {
        .key = GPIO_TO_PIN(1, 4), /* CT32B1_MAT3 */
        .channel = TIMER_CT32B1,
        .value = PACK_VALUE(2, 3)
    }, {
        .key = GPIO_TO_PIN(1, 6), /* CT32B0_MAT0 */
        .channel = TIMER_CT32B0,
        .value = PACK_VALUE(2, 0)
    }, {
        .key = GPIO_TO_PIN(1, 7), /* CT32B0_MAT1 */
        .channel = TIMER_CT32B0,
        .value = PACK_VALUE(2, 1)
    }, {
        .key = GPIO_TO_PIN(1, 9), /* CT16B1_MAT0 */
        .channel = TIMER_CT16B1,
        .value = PACK_VALUE(1, 0)
    }, {
        .key = GPIO_TO_PIN(1, 10), /* CT16B1_MAT1 */
        .channel = TIMER_CT16B1,
        .value = PACK_VALUE(2, 1)
    }, {
        /* End of pin function association list */
    }
};
/*----------------------------------------------------------------------------*/
static inline uint32_t *calcMatchChannel(LPC_TMR_TypeDef *, uint8_t);
static int8_t findEmptyChannel(uint8_t);
static void updateResolution(struct BaseTimerPwm *, uint8_t);
/*----------------------------------------------------------------------------*/
static enum result controllerInit(void *, const void *);
static void controllerDeinit(void *);
static void *controllerCreate(void *, gpioKey, uint8_t);
/*----------------------------------------------------------------------------*/
static void channelSetDutyCycle(void *, uint8_t);
static void channelSetEnabled(void *, bool);
static void channelSetPeriod(void *, uint16_t);
/*----------------------------------------------------------------------------*/
static const struct PwmControllerClass controllerTable = {
    .size = sizeof(struct BaseTimerPwm),
    .init = controllerInit,
    .deinit = controllerDeinit,

    .create = controllerCreate
};
/*----------------------------------------------------------------------------*/
static const struct PwmClass channelTable = {
    .size = sizeof(struct BaseTimerPwmChannel),
    .init = 0,
    .deinit = 0,

    .setDutyCycle = channelSetDutyCycle,
    .setEnabled = channelSetEnabled,
    .setPeriod = channelSetPeriod
};
/*----------------------------------------------------------------------------*/
const struct PwmControllerClass *BaseTimerPwm = &controllerTable;
const struct PwmClass *BaseTimerPwmChannel = &channelTable;
/*----------------------------------------------------------------------------*/
static inline uint32_t *calcMatchChannel(LPC_TMR_TypeDef *timer,
    uint8_t channel)
{
  return (uint32_t *)&timer->MR0 + channel;
}
/*----------------------------------------------------------------------------*/
static int8_t findEmptyChannel(uint8_t channels)
{
  int8_t pos = 4; /* Each timer has 4 match blocks */

  //FIXME Remove additional brackets?
  while (--pos >= 0 && (channels & (1 << pos)));

  return pos;
}
/*----------------------------------------------------------------------------*/
static void updateResolution(struct BaseTimerPwm *device, uint8_t channel)
{
//  LPC_TMR_TypeDef *reg = device->timer->reg;

  /* Disable timer counting */
  device->timer->reg->TCR &= ~TCR_CEN;
  /* Disable previous match channel */
  device->timer->reg->MCR &= ~MCR_RESET(device->current);

  device->current = channel;
  *calcMatchChannel(device->timer->reg, device->current) = device->resolution;
  /* Enable new match channel */
  device->timer->reg->MCR |= MCR_RESET(device->current);
  /* Enable timer */
  device->timer->reg->TCR |= TCR_CEN;
}
/*----------------------------------------------------------------------------*/
static void channelSetDutyCycle(void *object, uint8_t percentage)
{
  struct BaseTimerPwmChannel *pwm = object;

  *pwm->reg = !percentage ? (uint32_t)pwm->controller->resolution + 1
      : (uint32_t)pwm->controller->resolution
      * (uint32_t)(100 - percentage) / 100;
}
/*----------------------------------------------------------------------------*/
static void channelSetEnabled(void *object, bool state)
{

}
/*----------------------------------------------------------------------------*/
static void channelSetPeriod(void *object, uint16_t period)
{
  struct BaseTimerPwmChannel *pwm = object;

  *pwm->reg = period;
}
/*----------------------------------------------------------------------------*/
static void *controllerCreate(void *object, gpioKey output, uint8_t percentage)
{
  const struct GpioDescriptor *pin;
  struct BaseTimerPwm *device = object;
  struct BaseTimerPwmChannel *pwm;
  int8_t freeChannel;

  pin = gpioFind(pwmPins, output, device->timer->channel);
  assert(pin);

  freeChannel = findEmptyChannel(device->matches | UNPACK_CHANNEL(pin->value));
  if (freeChannel == -1)
    return 0; /* There are no free match channels left */

  if (!(pwm = init(BaseTimerPwmChannel, 0)))
    return 0;

  pwm->controller = device;
  pwm->channel = UNPACK_CHANNEL(pin->value);
  pwm->reg = calcMatchChannel(device->timer->reg, pwm->channel);

  device->matches |= UNPACK_CHANNEL(pin->value);
  updateResolution(device, (uint8_t)freeChannel);

  /* Initialize match output pin */
  pwm->pin = gpioInit(output, GPIO_OUTPUT);
  gpioSetFunction(&pwm->pin, UNPACK_FUNCTION(pin->value));

  pwmSetDutyCycle(pwm, percentage);
  device->timer->reg->PWMC |= PWMC_ENABLE(pwm->channel);

  return pwm;
}
/*----------------------------------------------------------------------------*/
static void controllerDeinit(void *object)
{
  struct BaseTimerPwm *device = object;

  deinit(device->timer); /* Delete BaseTimer object */
}
/*----------------------------------------------------------------------------*/
static enum result controllerInit(void *object, const void *configPtr)
{
  const struct BaseTimerPwmConfig * const config = configPtr;
  struct BaseTimerPwm *device = object;
  struct BaseTimerConfig timerConfig;
//  enum result res;

  /* Check device configuration data */
  assert(config);

  timerConfig.channel = config->channel;
  timerConfig.frequency = config->frequency * (uint32_t)config->resolution;

  /* Create parent BaseTimer object */
  if (!(device->timer = init(BaseTimer, &timerConfig)))
    return E_MEMORY;

  device->resolution = config->resolution;
  device->matches = 0;
  device->current = findEmptyChannel(device->matches);

  device->timer->reg->PWMC = 0;

  return E_OK;
}
