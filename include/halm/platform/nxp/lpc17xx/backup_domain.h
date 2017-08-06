/*
 * halm/platform/nxp/lpc17xx/backup_domain.h
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_PLATFORM_NXP_LPC17XX_BACKUP_DOMAIN_H_
#define HALM_PLATFORM_NXP_LPC17XX_BACKUP_DOMAIN_H_
/*----------------------------------------------------------------------------*/
#include <stddef.h>
#include <xcore/helpers.h>
#include <halm/target.h>
#include <halm/platform/platform_defs.h>
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

static inline void *backupDomainAddress(void)
{
  return (void *)LPC_RTC->GPREG;
}

static inline size_t backupDomainSize(void)
{
  return sizeof(LPC_RTC->GPREG);
}

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* HALM_PLATFORM_NXP_LPC17XX_BACKUP_DOMAIN_H_ */
