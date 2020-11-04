#pragma once


#if __has_feature(capabilities)
#include "gc.hpp"
#include <machine/cherireg.h>
#include <cheri/cheric.h>

// On the stack inked list compression with CHERI intrinsics.
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
  // Done by using the permission bits.
  // In practice this checks if the pointer has specific permissions 
  // as this directly differentiates pointers on the heap and ones on the stack.
  bool is_exec(void *ptr)
  {
    TRACE("%p %d\n", ptr, (cheri_getperm(ptr) != 0x6817d));
    return ((cheri_getperm(ptr) != 0x6817d) );

  }
  
  // find the next valid GC pointer by iterating the stack and checking 
  // the meta information for each 16 bytes.  
  void find_next() {
    TRACE("%lu\n", cheri_getaddress(bottom) - cheri_getaddress(location));
    // If the value doesn't have a tag it can be ignored
    // If the value doesn't have a flag it can be ignored
    // If the value doesn't have the correct permission it can be ignored.
    while(!cheri_gettag(*location) or !cheri_getflags(*location) or is_exec(*location))  
    {
      if(location >= bottom) {
        location = (void**)bottom;
        return;
      }
#ifdef VERBOSE
      TRACE("Looking for pointers: %p %p\n", location, *location);
#endif
      location += 1;
    }
    TRACE("t: %d f: %d e: %d \n", cheri_gettag(*location), cheri_getflags(*location), !is_exec(*location));
    TRACE("Found pointer: %p\n", *location);
  }

  // The iteration starts from the current stack pointer. 
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

