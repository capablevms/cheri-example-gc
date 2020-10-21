#pragma once


#if __has_feature(capabilities)
#include "gc.hpp"
#include <cheri/cheric.h>

// On the stack inked list compression with cheri intrinsics.
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

// Stack Iterator that gives all the pointers it could find on the stack.
struct CheriStackIterator
{
  void **location;

  // Check if the pointer is pointing on the stack.
  // Done by checking it aginst the bottom stack pointr's bounds.
  bool is_stack(void *ptr)
  {
    return cheri_is_address_inbounds(bottom, cheri_getaddress(ptr));
  }

  // Check if the pointer is executable. 
  // Done by using the permition bits.
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

    // Skip all the things that arn't pointers.
    // Also skip all the pointers we don't care about.
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

