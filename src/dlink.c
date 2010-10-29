/*
 * Copyright (c) 2003-2010 Eric Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains linked list functions.
 * Idea from ircd-ratbox.
 */

#include "../inc/shrike.h"

static BlockHeap *dlink_heap;

void dlink_init(void)
{
  /* God I hate naming structs ending with _t ... --dKingston */
  if ((dlink_heap = BlockHeapCreate(sizeof(dlink_t), DLINK_HEAP)) == NULL)
  {
    slog(LG_ERROR, "dlink_init(): block allocator failed.");
    exit(EXIT_FAILURE);
  }
}

/* NOTE: Nearly *all* of these are *heavily* used.
 * Multple calls to each are made on each network state change.
 * Therefore, I have inlined them in hopes of speed.
 */
inline dlink_t *dlink_create(void)
{
  dlink_t *dl = BlockHeapAlloc(dlink_heap);

  cnt.node++;
  return dl;
}

inline void dlink_free(dlink_t *dl)
{
  BlockHeapFree(dlink_heap, dl);
  cnt.node--;
}

inline void dlink_add(void *data, dlink_t *dl, list_t *l)
{
  dl->data = data;
  dl->next = l->head;

  if (l->head != NULL)
    l->head->prev = dl;
  else if (l->tail == NULL)
    l->tail = dl;

  l->head = dl;
  l->length++;
}

inline void dlink_add_tail(void *data, dlink_t *dl, list_t *l)
{
  dl->data = data;
  dl->next = NULL;
  dl->prev = l->tail;

  if (l->tail != NULL)
    l->tail->next = dl;
  else if (l->head == NULL)
    l->head = dl;

  l->tail = dl;
  l->length++;
}

inline dlink_t *dlink_find(void *data, list_t *l)
{
  dlink_t *dl;

  DLINK_FOREACH(dl, l->head)
    if (dl->data == data)
      return dl;

  return NULL;
}

inline void dlink_delete(dlink_t *dl, list_t *l)
{
  if (dl->next != NULL)
    dl->next->prev = dl->prev;
  else
    l->tail = dl->prev;

  if (dl->prev != NULL)
    dl->prev->next = dl->next;
  else
    l->head = dl->next;

  dl->next = dl->prev = NULL;
  l->length--;
}
