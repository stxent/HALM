/*
 * halm/generic/byte_queue_extensions.h
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef HALM_GENERIC_BYTE_QUEUE_EXTENSIONS_H_
#define HALM_GENERIC_BYTE_QUEUE_EXTENSIONS_H_
/*----------------------------------------------------------------------------*/
#include <xcore/containers/byte_queue.h>
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

static inline void byteQueueDeferredPop(struct ByteQueue *queue,
    void **buffer, size_t *size)
{
  assert(queue->size > 0);

  *buffer = queue->data + queue->head;

  if (queue->head < queue->tail)
    *size = queue->size;
  else
    *size = queue->capacity - queue->head;
}

static inline void byteQueueAbandon(struct ByteQueue *queue, size_t size)
{
  assert(queue->size >= size);

  queue->head += size;
  if (queue->head >= queue->capacity)
    queue->head -= queue->capacity;
  queue->size -= size;
}

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* HALM_GENERIC_BYTE_QUEUE_EXTENSIONS_H_ */
