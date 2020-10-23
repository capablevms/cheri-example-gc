#include "gc.hpp"
#include "compressed.hpp"
#include "cheri.hpp"
#include "pointers.hpp"

struct A {
  GCObject gc;
  int a;
};

struct C {
  GCObject gc;
  uint32_t c[16];
};

const uint64_t offsets[1] = {0};

using LS = PointerListStyle;
//using LS = CompressedListStyle;
//using LS = CheriCompressedListStyle;
using Iter = CheriStackIterator;
//using Iter = PointersLinkedListIterator<LS>;

void function_a(void* prev, uint32_t d);
void function_b(void* prev, uint32_t d);
void function_c(void* prev, uint32_t d);

void function_a(void* prev, uint32_t d) {
  Pointers<3, LS> ptrs(prev);
  ptrs.values[0] = (GCObject*)get_gc<A>(offsets);
  ptrs.values[1] = (GCObject*)get_gc<C>(offsets);
  ptrs.values[2] = (GCObject*)get_gc<C>(offsets);
  
  uint32_t s[16] = {};
  for(uint32_t i = 0; i < 16; i++) {
    s[i] = reinterpret_cast<C*>(ptrs.values[1])->c[i] + 
           reinterpret_cast<C*>(ptrs.values[2])->c[i];
  }

  for(uint32_t i = 0; i < 16; i++) {
    reinterpret_cast<A*>(ptrs.values[0])->a += s[i];
  }

  function_b(&ptrs, d);

}

void function_b(void* prev, uint32_t d) {
  Pointers<3, LS> ptrs(prev);
  ptrs.values[0] = (GCObject*)get_gc<A>(offsets);
  ptrs.values[1] = (GCObject*)get_gc<A>(offsets);
  ptrs.values[2] = (GCObject*)get_gc<A>(offsets);
 
  function_c(&ptrs, d);
}

void function_c(void* prev, uint32_t d) {
  Pointers<1, LS> ptrs(prev);
  ptrs.values[0] = (GCObject*)get_gc<A>(offsets);
  
  char path[32] = "00000000";
  path[d % 8] = '1';
  printf("Test: %s\n", path);

  if(d > 300) {
    walk<Iter>(&ptrs);
    sweep();
  }else{
    function_a(&ptrs, d + 1);
  }
}

int main() {
  bottom = __builtin_frame_address(0);
  Pointers<1, LS> root;
  
  for(uint32_t i = 0; i < 1; i++) {
    function_a(&root, 0);
  }

  walk<Iter>(&root);
  sweep();
}
