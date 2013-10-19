/*
 * spinlock.c
 * Copyright (C) 2012 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <core/cortex/m3/asm.h>
#include <spinlock.h>
/*----------------------------------------------------------------------------*/
void spinLock(spinlock_t *lock)
{
  do
  {
    /* Wait until lock becomes free */
    while (__ldrexb(lock) != SPIN_UNLOCKED);
  }
  while (__strexb(SPIN_LOCKED, lock) != 0); /* Try to lock */
  __dmb();
}
/*----------------------------------------------------------------------------*/
bool spinTryLock(spinlock_t *lock)
{
  if (__ldrexb(lock) == SPIN_UNLOCKED) /* Get current state */
  {
    /* Try to lock */
    while (__strexb(SPIN_LOCKED, lock) != 0);
    __dmb();
    return true;
  }
  return false; /* Already locked */
}
/*----------------------------------------------------------------------------*/
void spinUnlock(spinlock_t *lock)
{
  __dmb(); /* Ensure memory operations completed before releasing lock */
  *lock = SPIN_UNLOCKED;
}