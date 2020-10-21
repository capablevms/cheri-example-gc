#pragma once

#include "gc.hpp"

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

static_assert(std::is_standard_layout_v<Pointers<8, PointerListStyle>>);


// Iterate all the pointers on the special Pointers structure on the stack.
// The Pointers structure makes a linked list so it can be easily followed.
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

