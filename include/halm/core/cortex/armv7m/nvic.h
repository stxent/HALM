/*
 * halm/core/cortex/armv7m/nvic.h
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_CORE_CORTEX_ARMV7M_NVIC_H_
#define HALM_CORE_CORTEX_ARMV7M_NVIC_H_
/*----------------------------------------------------------------------------*/
#include <stdint.h>
#include <xcore/helpers.h>
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

uint8_t nvicGetPriorityGrouping(void);
void nvicSetPriorityGrouping(uint8_t);
void nvicResetCore(void);
void nvicSetVectorTableOffset(uint32_t);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* HALM_CORE_CORTEX_ARMV7M_NVIC_H_ */