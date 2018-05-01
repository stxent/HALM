/*
 * bod.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/platform/nxp/bod.h>
#include <halm/platform/nxp/lpc17xx/system_defs.h>
#include <halm/platform/platform_defs.h>
/*----------------------------------------------------------------------------*/
static void resetInstance(void);
static bool setInstance(struct Bod *);
/*----------------------------------------------------------------------------*/
static enum Result bodInit(void *, const void *);
static void bodEnable(void *);
static void bodDisable(void *);
static void bodSetCallback(void *, void (*)(void *), void *);

#ifndef CONFIG_PLATFORM_NXP_BOD_NO_DEINIT
static void bodDeinit(void *);
#else
#define bodDeinit deletedDestructorTrap
#endif
/*----------------------------------------------------------------------------*/
static const struct InterruptClass bodTable = {
    .size = sizeof(struct Bod),
    .init = bodInit,
    .deinit = bodDeinit,

    .enable = bodEnable,
    .disable = bodDisable,
    .setCallback = bodSetCallback
};
/*----------------------------------------------------------------------------*/
const struct InterruptClass * const Bod = &bodTable;
static struct Bod *instance = 0;
/*----------------------------------------------------------------------------*/
static void resetInstance(void)
{
  instance = 0;
}
/*----------------------------------------------------------------------------*/
static bool setInstance(struct Bod *object)
{
  if (!instance)
  {
    instance = object;
    return true;
  }
  else
    return false;
}
/*----------------------------------------------------------------------------*/
void BOD_ISR(void)
{
  instance->callback(instance->callbackArgument);
}
/*----------------------------------------------------------------------------*/
static enum Result bodInit(void *object, const void *configBase)
{
  const struct BodConfig * const config = configBase;
  assert(config);

  struct Bod * const bod = object;

  if (setInstance(bod))
  {
    uint32_t pcon = LPC_SC->PCON | PCON_BODRPM | PCON_BORD;

    bod->callback = 0;
    bod->enabled = false;

    LPC_SC->PCON = pcon;
    LPC_SC->PCON = pcon & ~PCON_BOGD;
    if (config->resetLevel != BOD_RESET_DISABLED)
      LPC_SC->PCON = pcon & ~(PCON_BOGD | PCON_BODRPM | PCON_BORD);

    irqSetPriority(BOD_IRQ, config->priority);

    return E_OK;
  }
  else
    return E_BUSY;
}
/*----------------------------------------------------------------------------*/
#ifndef CONFIG_PLATFORM_NXP_BOD_NO_DEINIT
static void bodDeinit(void *object __attribute__((unused)))
{
  irqDisable(BOD_IRQ);
  resetInstance();
}
#endif
/*----------------------------------------------------------------------------*/
static void bodEnable(void *object)
{
  struct Bod * const bod = object;

  bod->enabled = true;
  if (bod->callback)
    irqEnable(BOD_IRQ);
}
/*----------------------------------------------------------------------------*/
static void bodDisable(void *object)
{
  struct Bod * const bod = object;

  bod->enabled = false;
  irqDisable(BOD_IRQ);
}
/*----------------------------------------------------------------------------*/
static void bodSetCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct Bod * const bod = object;

  bod->callbackArgument = argument;
  bod->callback = callback;

  if (bod->enabled && callback)
    irqEnable(BOD_IRQ);
  else
    irqDisable(BOD_IRQ);
}
