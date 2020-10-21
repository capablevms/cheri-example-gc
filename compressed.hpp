#pragma once

#include "gc.hpp"

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

template <> struct PointerSelector<CompressedListStyle>
{
  using ptr_type = uint32_t;
};

static_assert(std::is_standard_layout_v<Pointers<8, CompressedListStyle>>);
