#include <cstddef>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <type_traits>
#include <utility>

#pragma once

#if defined NDEBUG
#define TRACE(format, ...)
#else
#define TRACE(format, ...)                                                                         \
  printf("%s::%s(%d) " format, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif

struct GCObject
{
  GCObject *next;
  uint8_t mark;
  const uint64_t *type;
};
static_assert(std::is_standard_layout_v<GCObject>);

static thread_local GCObject *start = nullptr;
static thread_local uint8_t timeWalked = 0;
static inline void *bottom = 0;

template <typename ListStyle> struct PointerSelector
{
  using ptr_type = const void *;
};

template <uint32_t count, typename ListStyle> struct Pointers
{
  uint32_t size = count;
  uint32_t depth;
  using ptr_type = typename PointerSelector<ListStyle>::ptr_type;
  ptr_type previous;
  GCObject *values[count] = {nullptr};

  Pointers() : depth(0), previous(ListStyle::end) {}

  Pointers(const void *previous)
  {
    this->previous = ListStyle::transform(previous);
    depth = reinterpret_cast<const Pointers *>(ListStyle::get(this->previous))->depth + 1;
  }
};

void sweep()
{
  GCObject **current = &start;
  while (true)
  {
    TRACE("LookingAt: %p, Type: %p, Mark: %x \n", current, (*current)->type, (*current)->mark);
    if ((*current)->mark != timeWalked)
    {
      TRACE("Freeing: %p\n", current);
      auto next = (*current)->next;
      free(*current);
      *current = next;
    }
    else
    {
      current = &(*current)->next;
    }
    if (*current == nullptr)
    {
      break;
    }
  }
}

void mark(uint8_t d, GCObject *object)
{
  if (object == nullptr)
  {
    return;
  }
  else if (object->mark == timeWalked)
  {
    return;
  }
  if (d > 0)
  {
    TRACE("pointer[%d]: Value: %p Type: %p\n", d, object, object->type);
  }
  object->mark = timeWalked;
  for (size_t index = 0; object->type[index] != 0; index++)
  {
    mark(d + 1,
         *reinterpret_cast<GCObject **>(reinterpret_cast<uint8_t *>(object) + object->type[index]));
  }
}

template <typename PointerIterator> void walk(void *start)
{
  timeWalked += 1;

  for (GCObject *lookedAt : PointerIterator(start))
  {
    TRACE("pointer[%d]: Value: %p\n", 0, lookedAt);
    mark(0, lookedAt);
  };
}

#if __has_feature(capabilities)
  #include<cheri/cheric.h>
#endif

// gc allocate and put on the linked list
template <typename Obj = GCObject> Obj *get_gc(const uint64_t *_offset)
{
  GCObject *result = reinterpret_cast<GCObject *>(malloc(sizeof(Obj)));
  TRACE("allocated: %p\n", result);

#if __has_feature(capabilities)
  result = cheri_setflags(result, 1);
#endif

  result->next = start;
  start = result;
  result->type = _offset;
  return reinterpret_cast<Obj *>(result);
}
