/*
 * console.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <ev.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <xcore/containers/byte_queue.h>
#include <halm/platform/generic/console.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 64
#define QUEUE_SIZE  2048
/*----------------------------------------------------------------------------*/
enum Cleanup
{
  CLEANUP_ALL,
  CLEANUP_THREAD,
  CLEANUP_MUTEX,
  CLEANUP_SEMAPHORE,
  CLEANUP_QUEUE
};

struct InterfaceWatcher
{
  ev_io io;
  void *instance;
};

struct Console
{
  struct Interface base;

  void (*callback)(void *);
  void *callbackArgument;

  struct ByteQueue rxQueue;
  pthread_mutex_t rxQueueLock;

  struct termios initialSettings;
  struct InterfaceWatcher watcher;
};
/*----------------------------------------------------------------------------*/
static void configurePort(struct Console *);
static void interfaceCallback(EV_P_ ev_io *, int);
/*----------------------------------------------------------------------------*/
static enum Result streamInit(void *, const void *);
static void streamDeinit(void *);
static enum Result streamSetCallback(void *, void (*)(void *), void *);
static enum Result streamGetParam(void *, enum IfParameter, void *);
static enum Result streamSetParam(void *, enum IfParameter, const void *);
static size_t streamRead(void *, void *, size_t);
static size_t streamWrite(void *, const void *, size_t);
/*----------------------------------------------------------------------------*/
const struct InterfaceClass * const Console = &(const struct InterfaceClass){
    .size = sizeof(struct Console),
    .init = streamInit,
    .deinit = streamDeinit,

    .setCallback = streamSetCallback,
    .getParam = streamGetParam,
    .setParam = streamSetParam,
    .read = streamRead,
    .write = streamWrite
};
/*----------------------------------------------------------------------------*/
static void configurePort(struct Console *interface)
{
  struct termios settings;

  tcgetattr(STDIN_FILENO, &settings);
  interface->initialSettings = settings;

  settings.c_lflag &= ~(ISIG | ICANON | ECHO);
  settings.c_cc[VMIN] = 0; /* Minimal data packet length */
  settings.c_cc[VTIME] = 0; /* Time to wait for data */

  tcsetattr(STDIN_FILENO, TCSANOW, &settings);
}
/*----------------------------------------------------------------------------*/
static void interfaceCallback(EV_P_ ev_io *w,
    int revents __attribute__((unused)))
{
  struct InterfaceWatcher * const watcher = (struct InterfaceWatcher *)w;
  struct Console * const interface = watcher->instance;
  uint8_t buffer[BUFFER_SIZE];
  ssize_t length;

  pthread_mutex_lock(&interface->rxQueueLock);
  while ((length = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0)
    byteQueuePushArray(&interface->rxQueue, buffer, (size_t)length);
  pthread_mutex_unlock(&interface->rxQueueLock);

  if (interface->callback)
    interface->callback(interface->callbackArgument);
}
/*----------------------------------------------------------------------------*/
static enum Result streamInit(void *object,
    const void *configBase __attribute__((unused)))
{
  struct Console * const interface = object;
  enum Result res;

  interface->callback = 0;

  if (pthread_mutex_init(&interface->rxQueueLock, 0))
    return E_ERROR;

  if ((res = byteQueueInit(&interface->rxQueue, QUEUE_SIZE)) != E_OK)
  {
    pthread_mutex_destroy(&interface->rxQueueLock);
    return res;
  }

  configurePort(interface);

  interface->watcher.instance = interface;
  ev_init(&interface->watcher.io, interfaceCallback);
  ev_io_set(&interface->watcher.io, STDIN_FILENO, EV_READ);
  ev_io_start(ev_default_loop(0), &interface->watcher.io);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void streamDeinit(void *object __attribute__((unused)))
{
  struct Console * const interface = object;

  ev_io_stop(ev_default_loop(0), &interface->watcher.io);

  /* Restore terminal settings */
  tcsetattr(STDIN_FILENO, TCSANOW, &interface->initialSettings);

  byteQueueDeinit(&interface->rxQueue);
  pthread_mutex_destroy(&interface->rxQueueLock);
}
/*----------------------------------------------------------------------------*/
static enum Result streamSetCallback(void *object, void (*callback)(void *),
    void *argument)
{
  struct Console * const interface = object;

  interface->callbackArgument = argument;
  interface->callback = callback;
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum Result streamGetParam(void *object, enum IfParameter parameter,
    void *data)
{
  struct Console *interface = object;

  switch (parameter)
  {
    case IF_AVAILABLE:
      pthread_mutex_lock(&interface->rxQueueLock);
      *(size_t *)data = byteQueueSize(&interface->rxQueue);
      pthread_mutex_unlock(&interface->rxQueueLock);
      return E_OK;

    case IF_PENDING:
      *(size_t *)data = 0;
      return E_OK;

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static enum Result streamSetParam(void *object __attribute__((unused)),
    enum IfParameter parameter __attribute__((unused)),
    const void *data __attribute__((unused)))
{
  return E_INVALID;
}
/*----------------------------------------------------------------------------*/
static size_t streamRead(void *object, void *buffer, size_t length)
{
  struct Console * const interface = object;

  pthread_mutex_lock(&interface->rxQueueLock);
  const size_t read = byteQueuePopArray(&interface->rxQueue, buffer, length);
  pthread_mutex_unlock(&interface->rxQueueLock);

  return read;
}
/*----------------------------------------------------------------------------*/
static size_t streamWrite(void *object __attribute__((unused)),
    const void *buffer, size_t length)
{
  if (!length)
    return 0;

  const ssize_t written = write(STDOUT_FILENO, buffer, length);

  if (written)
  {
    fsync(STDOUT_FILENO);
    return (size_t)written;
  }
  else
  {
    return 0;
  }
}
