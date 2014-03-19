/*
 * platform/nxp/gptimer_defs.h
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef GPTIMER_DEFS_H_
#define GPTIMER_DEFS_H_
/*----------------------------------------------------------------------------*/
#include <bits.h>
/*------------------Interrupt Register----------------------------------------*/
#define IR_MATCH_INTERRUPT(channel)     BIT((channel))
#define IR_CAPTURE_INTERRUPT(channel)   BIT((channel) + 4)
/*------------------Timer Control Register------------------------------------*/
#define TCR_CEN                         BIT(0) /* Enable for counting */
#define TCR_CRES                        BIT(1) /* Synchronous reset */
/*------------------Match Control Register------------------------------------*/
#define MCR_INTERRUPT(channel)          BIT((channel) * 3)
#define MCR_RESET(channel)              BIT((channel) * 3 + 1)
#define MCR_STOP(channel)               BIT((channel) * 3 + 2)
/*------------------Capture Control Register----------------------------------*/
#define CCR_RISING_EDGE(channel)        BIT((channel) * 3)
#define CCR_FALLING_EDGE(channel)       BIT((channel) * 3 + 1)
#define CCR_INTERRUPT(channel)          BIT((channel) * 3 + 2)
/*------------------External Match Register-----------------------------------*/
#define EMR_CONTROL_MASK(channel)       BIT_FIELD(0x03, (channel) * 2 + 4)
#define EMR_CONTROL_CLEAR               1
#define EMR_CONTROL_SET                 2
#define EMR_CONTROL_TOGGLE              3
#define EMR_CONTROL(channel, value)     BIT_FIELD((value), (channel) * 2 + 4)
#define EMR_EXTERNAL_MATCH(channel)     BIT((channel))
/*----------------------------------------------------------------------------*/
#endif /* GPTIMER_DEFS_H_ */
