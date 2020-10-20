#include <cstddef>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <type_traits>
#include <utility>

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

struct CompressedListStyle
{
  static inline const uint32_t end = 0;

  static uint32_t transform(const void *ptr)
  {
    return static_cast<uint32_t>(reinterpret_cast<uint8_t *>(bottom) -
                                 reinterpret_cast<const uint8_t *>(ptr));
  }

  static void *get(uint32_t distance)
  {
    return reinterpret_cast<uint8_t *>(bottom) - distance;
  }

  static bool invalid(uint32_t distance)
  {
    return distance == 0;
  }
};

template <typename ListStyle> struct PointerSelector
{
  using ptr_type = const void *;
};

template <> struct PointerSelector<CompressedListStyle>
{
  using ptr_type = uint32_t;
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
static_assert(std::is_standard_layout_v<Pointers<8, CompressedListStyle>>);

struct PointerListStyle
{
  static inline const Pointers<0, PointerListStyle> *end = nullptr;

  static const Pointers<0, PointerListStyle> *transform(const void *ptr)
  {
    return reinterpret_cast<const Pointers<0, PointerListStyle> *>(ptr);
  }

  static const void *get(const void *ptr)
  {
    return ptr;
  }

  static bool invalid(const void *ptr)
  {
    return ptr == nullptr;
  }
};

template <> struct PointerSelector<PointerListStyle>
{
  using ptr_type = const Pointers<0, PointerListStyle> *;
};

#if __has_feature(capabilities)
#include <cheri/cheric.h>
struct CheriCompressedListStyle
{
  static inline const uint32_t end = 0;

  static const uint32_t transform(const void *ptr)
  {
    return static_cast<uint32_t>(cheri_getoffset(ptr));
  }

  static const void *get(uint32_t offset)
  {
    return cheri_setoffset(cheri_getstack(), offset);
  }

  static bool invalid(uint32_t offset)
  {
    return offset == 0;
  }
};

template <> struct PointerSelector<CheriCompressedListStyle>
{
  using ptr_type = uint32_t;
};

#endif

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

template <typename ListStyle> struct PointersLinkedListIterator
{
  const Pointers<0, ListStyle> *curr;
  uint32_t index;

  PointersLinkedListIterator()
  {
    curr = nullptr;
  }

  PointersLinkedListIterator(void *start)
  {
    index = 0;
    curr = reinterpret_cast<Pointers<0, ListStyle> *>(start);

    /*
    TRACE("Size: %u Depth: %u\n", curr->size, curr->depth);
    for (uint32_t size = 0; size < curr->size; size++) {

      TRACE("pointer[%d]: Value: %p\n", size, curr->values[size]);
    }
    */
  }

  PointersLinkedListIterator &operator++()
  {
    index += 1;
    if (index == curr->size)
    {
      TRACE("check: %u %p\n", curr->previous, ListStyle::get(curr->previous));
      if (ListStyle::invalid(curr->previous))
      {
        curr = nullptr;
      }
      else
      {
        index = 0;
        curr = reinterpret_cast<const Pointers<0, ListStyle> *>(ListStyle::get(curr->previous));
        TRACE("Size: %u Depth: %u\n", curr->size, curr->depth);
      }
    }
    return *this;
  }

  bool operator!=(PointersLinkedListIterator iter) const
  {
    return curr != iter.curr;
  }
  GCObject *operator*() const
  {
    return curr->values[index];
  }

  PointersLinkedListIterator &begin()
  {
    return *this;
  }

  PointersLinkedListIterator end()
  {
    return std::move(PointersLinkedListIterator());
  }
};

#if __has_feature(capabilities)
struct CheriStackIterator
{
  void **location;

  bool is_stack(void *ptr)
  {
    return cheri_is_address_inbounds(bottom, cheri_getaddress(ptr));
  }

  bool is_exec(void *ptr)
  {
    return ((cheri_getperm(ptr) & 0b10) >> 1);
  }

  CheriStackIterator()
  {
    location = reinterpret_cast<void **>(bottom);
  }

  CheriStackIterator(void *start)
  {
    location = reinterpret_cast<void **>(start);
  }

  CheriStackIterator &operator++()
  {
    location -= 1;
    while (!cheri_gettag(*location) and (is_stack(*location) or is_exec(*location)))
    {
      location -= 1;
      TRACE("Looking for pointers: %p\n", location);
    }
    TRACE("Found pointer: %p\n", location);
    return *this;
  }

  bool operator!=(CheriStackIterator iter) const
  {
    return location != iter.location;
  }
  GCObject *operator*() const
  {
    return reinterpret_cast<GCObject *>(*location);
  }

  CheriStackIterator &begin()
  {
    return *this;
  }

  CheriStackIterator end()
  {
    return std::move(CheriStackIterator());
  }
};
#endif

template <typename PointerIterator> void walk(void *start)
{
  timeWalked += 1;

  for (GCObject *lookedAt : PointerIterator(start))
  {
    TRACE("pointer[%d]: Value: %p\n", 0, lookedAt);
    mark(0, lookedAt);
  };
}

template <typename Obj> Obj *get_gc(const uint64_t *_offset)
{
  GCObject *result = reinterpret_cast<GCObject *>(malloc(sizeof(Obj)));
  result->next = start;
  start = result;
  result->type = _offset;
  return reinterpret_cast<Obj *>(result);
}
