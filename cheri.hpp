#pragma once


#include <machine/cherireg.h>
#if __has_feature(capabilities)
#include "gc.hpp"
#include <cheri/cheric.h>

#include "../examples/include/common.h"
// On the stack inked list compression with cheri intrinsics.
struct CheriCompressedListStyle
{
  static inline const uint32_t end = 0;

  static const uint32_t transform(const void *ptr)
  {
    auto offset = cheri_getaddress(ptr) - cheri_getbase(cheri_getstack());
    return static_cast<uint32_t>(offset);
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

  // Check if the pointer is executable. 
  // Done by using the permition bits.
  bool is_exec(void *ptr)
  {
    TRACE("%p %d\n", ptr, (cheri_getperm(ptr) != 0x6817d));
    //inspect_pointer(*location);
    return ((cheri_getperm(ptr) != 0x6817d) );

  }
  
  // Skip all the things that arn't pointers.
  // Also skip all the pointers we don't care about.  
  void find_next() {
    TRACE("%lu\n", cheri_getaddress(bottom) - cheri_getaddress(location));
    while(!cheri_gettag(*location) or !cheri_getflags(*location) or is_exec(*location))  
    {
      if(location >= bottom) {
        location = (void**)bottom;
        return;
      }
      //TRACE("Looking for pointers: %p %p\n", location, *location);
      location += 1;
    }
    TRACE("t: %d f: %d e: %d \n", cheri_gettag(*location), cheri_getflags(*location), !is_exec(*location));
    TRACE("Found pointer: %p\n", *location);
  }

  
  CheriStackIterator(void *start)
  {
    location = reinterpret_cast<void **>(cheri_getstack());
  }

  CheriStackIterator &operator++()
  {
    location += 1;
    return *this;
  }

  bool operator!=(CheriStackIterator iter)
  {
    find_next();
    TRACE("%lu %d\n", cheri_getaddress(bottom) - cheri_getaddress(location), (location != iter.location));
    return location != bottom;
  }

  GCObject *operator*() const
  {
    auto result = cheri_setoffset(*location, 0);
    return reinterpret_cast<GCObject *>(result);
  }

  CheriStackIterator& begin()
  {
    return *this;
  }

  CheriStackIterator& end()
  {
    return *this;
  }
};
#endif

