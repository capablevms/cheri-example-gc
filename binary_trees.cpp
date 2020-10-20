#include "gc.hpp"

#include <iostream>
#include <stdlib.h>

struct Node
{
  GCObject gc;
  Node *left;
  Node *right;

  int check() const
  {
    if (left)
    {
      return left->check() + 1 + right->check();
    }
    else
    {
      return 1;
    }
  }
};
const uint64_t Node_offsets[3] = {offsetof(Node, left), offsetof(Node, right), 0};

Node *node_new(const int d)
{
  Node *result = get_gc<Node>(Node_offsets);
  if (d > 0)
  {
    result->left = node_new(d - 1);
    result->right = node_new(d - 1);
  }
  else
  {
    result->left = result->right = nullptr;
  }
  return result;
}

// using LS = PointerListStyle;
// using LS = CompressedListStyle;
using LS = CheriCompressedListStyle;
using Iter = PointersLinkedListIterator<LS>;

int main(int argc, char *argv[])
{
  int min_depth = 4;
  int max_depth = std::max(min_depth + 2, (argc == 2 ? atoi(argv[1]) : 10));
  int stretch_depth = max_depth + 1;
  Pointers<2, LS> ptrs;

  {
    ptrs.values[0] = reinterpret_cast<GCObject *>(node_new(stretch_depth));
    std::cout << "stretch tree of depth " << stretch_depth << "\t "
              << "check: " << reinterpret_cast<Node *>(ptrs.values[0])->check() << std::endl;
    walk<Iter>(&ptrs);
  }

  ptrs.values[0] = reinterpret_cast<GCObject *>(node_new(max_depth));

  for (int d = min_depth; d <= max_depth; d += 2)
  {
    int iterations = 1 << (max_depth - d + min_depth), c = 0;
    for (int i = 1; i <= iterations; ++i)
    {
      ptrs.values[1] = reinterpret_cast<GCObject *>(node_new(d));
      c += reinterpret_cast<Node *>(ptrs.values[0])->check();
    }
    std::cout << iterations << "\t trees of depth " << d << "\t "
              << "check: " << c << std::endl;

    walk<Iter>(&ptrs);
    sweep();
  }

  std::cout << "long lived tree of depth " << max_depth << "\t "
            << "check: " << ((reinterpret_cast<Node *>(ptrs.values[0]))->check()) << "\n";

  walk<Iter>(&ptrs);
  sweep();
  return 0;
}
