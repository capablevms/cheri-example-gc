#include <cstddef>
#include <optional>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "gc.hpp"

using LS = CompressedListStyle;

struct String
{
  GCObject gc;
  uint32_t size;
  uint8_t value[256];
};
const uint64_t String_offsets[1] = {0};

struct Car
{
  GCObject gc;
  uint32_t speed;
  uint32_t color;
  String *brand;
};
const uint64_t Car_offsets[2] = {offsetof(Car, brand), 0};

static_assert(std::is_standard_layout_v<Car>);
static_assert(std::is_standard_layout_v<String>);

String *string_new(char *str)
{
  String *result = get_gc<String>(String_offsets);
  strcpy(reinterpret_cast<char *>(result->value), str);
  return result;
}

Car *car_new(uint32_t speed, uint32_t color)
{
  Car *result = get_gc<Car>(Car_offsets);
  result->color = color;
  result->speed = speed;
  return result;
}

Car *car_combine(const void *prev, Car *car1, Car *car2)
{
  Pointers<1, LS> ptrs(prev);
  ptrs.values[0] = (GCObject *)car_new(car1->speed, car2->color);
  reinterpret_cast<Car *>(ptrs.values[0])->brand = string_new("Combined");

  return reinterpret_cast<Car *>(ptrs.values[0]);
}

void car_print(const void *prev, Car *self)
{
  Pointers<1, LS> ptrs(prev);

  printf("Car(speed: %u, color: %u", self->speed, self->color);
  if (self->brand != nullptr)
  {
    printf(", brand: \"%s\")\n", self->brand->value);
    self->brand = nullptr;
  }
  else
  {
    printf(")\n");
  }

  walk<PointersLinkedListIterator<LS>>(&ptrs);
  sweep();
}

int main()
{
  Pointers<2, LS> ptrs;
  CompressedListStyle::bottom = __builtin_frame_address(0);

  ptrs.values[0] = (GCObject *)car_new(120, 0);
  ptrs.values[1] = (GCObject *)car_new(80, 1);
  reinterpret_cast<Car *>(ptrs.values[1])->brand = string_new("MyOwn");

  ptrs.values[0] = (GCObject *)car_combine(&ptrs, reinterpret_cast<Car *>(ptrs.values[0]),
                                           reinterpret_cast<Car *>(ptrs.values[1]));

  car_print(&ptrs, reinterpret_cast<Car *>(ptrs.values[1]));
  car_print(&ptrs, reinterpret_cast<Car *>(ptrs.values[0]));

  printf("FINISH\n");
}
