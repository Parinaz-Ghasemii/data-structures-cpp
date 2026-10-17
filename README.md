# Data structures in C++: k-d tree, min-max heap, compressed trie

Header-only C++17 implementations of three classic data structures, each checked against a brute-force reference with randomized tests.

The k-d tree is the centerpiece. It supports nearest-neighbor, **k-nearest-neighbor** and radius queries, and a small example uses it to classify points with k-NN.

| Structure | Header | Used for |
|---|---|---|
| 2-D k-d tree | [`include/kd_tree.hpp`](include/kd_tree.hpp) | Nearest-neighbor search, k-NN classification, range queries |
| Min-max heap | [`include/min_max_heap.hpp`](include/min_max_heap.hpp) | A priority queue with O(1) access to both the minimum and the maximum |
| Standard trie and compressed trie (radix tree) | [`include/trie.hpp`](include/trie.hpp) | Word sets with prefix search; the two variants show the memory saving of compression |

## Quick start

Needs `make` and a C++17 compiler (`g++` or Apple `clang++`; on macOS run `xcode-select --install` once).

```bash
make test                 # builds everything into build/ and runs all tests
./build/knn_classifier    # the k-NN example
```

## k-d tree

A k-d tree stores points in a binary tree. Each level splits the plane along one axis (x, then y, then x, ...), so a query can skip whole regions of space that are too far away to matter.

```cpp
#include "kd_tree.hpp"

std::vector<Point> pts = {{2,3}, {5,4}, {9,6}, {4,7}, {8,1}, {7,2}};
KDTree tree(pts);                        // balanced: splits on the median at every level

Point q; q.x = 9; q.y = 2;
Point nn = tree.nearest(q);              // closest stored point
auto five = tree.kNearest(q, 5);         // 5 closest, nearest first
auto close = tree.withinRadius(q, 3);    // everything within distance 3
bool has  = tree.contains(q);
tree.insert(q);                          // incremental insert (not rebalanced)
```

| Operation | Cost |
|---|---|
| Build from n points (sorts each level) | O(n log² n) |
| `nearest` / `kNearest` | about O(log n) on well-spread data, O(n) in the worst case |
| `insert`, `contains` | O(depth) |
| `withinRadius` | depends on how many points are inside the radius |

The search pruning rule: after exploring the side of a node that contains the query, the other side is visited only if the splitting line is closer to the query than the best candidates found so far.

`tree.lastDistanceChecks()` reports how many points a query actually measured, so the saving over a brute-force scan is easy to see.

### Example: k-NN classification

`examples/knn_classifier.cpp` classifies points from two noisy clusters by a majority vote among the 5 nearest training points, then counts how much work each query needs:

```text
k-NN classification (k = 5, 300 training points, 200 test points)
  accuracy:                       197 / 200
  same neighbours as brute force: 200 / 200
  distance computations per query: 25.2 (brute force: 300)

nearest neighbour among 50000 random points (1000 queries)
  distance computations per query: 22.1 (brute force: 50000)
  tree height: 16
```

The fixed random seed makes the output identical on every run. These counts hold for uniformly spread 2-D data. k-d trees lose most of this advantage as the number of dimensions grows, which is one reason k-NN libraries use other structures (ball trees, approximate methods) for high-dimensional data.

### Command-line demo

```bash
printf '7\n2 3\n5 4\n9 6\n4 7\n8 1\n7 2\n6 9\n9 2\n' | ./build/kd_tree_demo
```
Input: `n`, then `n` points, then the query point. Output: the tree in level order, then the nearest point (`8 1`).

## Min-max heap

A complete binary tree in an array whose levels alternate between "min" levels (a node is ≤ everything below it) and "max" levels (a node is ≥ everything below it). The minimum sits at the root and the maximum is one of the root's two children, so both are available in O(1).

```cpp
#include "min_max_heap.hpp"

MinMaxHeap h;
for (int x : {5, 17, 3, 9, 1, 12, 20, 7}) h.push(x);
h.findMin();   // 1
h.findMax();   // 20
h.popMax();    // 20
h.popMin();    // 1
```

| Operation | Cost |
|---|---|
| `findMin`, `findMax` | O(1) |
| `push`, `popMin`, `popMax` | O(log n) |

Compared with using two separate heaps, one structure answers both queries without keeping two copies in sync. Operations on an empty heap throw `std::out_of_range`.

## Tries

```cpp
#include "trie.hpp"

CompressedTrie t;
for (auto w : {"apple", "banana", "app", "application"}) t.insert(w);
t.wordsWithPrefix("app");     // app, apple, application   (alphabetical)
t.contains("banana");         // true
t.erase("apple");
```

`StandardTrie` stores one character per edge. `CompressedTrie` stores whole substrings on its edges, so a chain of single-child nodes becomes one node. `./build/trie_demo` inserts six words into both and compares them:

```text
nodes (standard trie):   21
nodes (compressed trie): 9
```

Insert, search and erase take O(L) for a word of length L in both variants. The compressed trie uses fewer nodes, which pays off most when words share long prefixes; the standard trie has simpler code. Words must be lowercase `a-z`; anything else makes `insert` throw `std::invalid_argument`.

## Testing

```bash
make test
```

Each structure is tested with hundreds of randomized scenarios compared against a simple reference:

- **k-d tree:** nearest, k-nearest, radius and contains queries against a brute-force scan; trees built from a vector and trees grown by insertion; many duplicate points; tree height of the balanced build.
- **Min-max heap:** random push/popMin/popMax sequences against `std::multiset`, checking the min-max property after every operation.
- **Tries:** random insert/erase/lookup/prefix operations against `std::set`, for both variants, plus a structural check that the compressed trie never keeps a redundant node.

The tests also pass under AddressSanitizer and UBSan (no memory errors or leaks). As a check on the tests themselves, I injected deliberate bugs (for example skipping the far branch in the k-d tree search, or flipping the heap's level parity) and confirmed that each one made the suite fail.

## Limitations

- The k-d tree is 2-D only, has no deletion, and `insert` does not rebalance. Recursion depth equals the tree height, so avoid inserting very large sorted sequences one by one; build from a vector instead.
- Coordinates are `int` and distances are exact squared integers; there is no floating-point version.
- Nearest-neighbor cost is only logarithmic on average; adversarial or high-dimensional data can force a full scan.
- Tries accept only lowercase `a-z`.

## Background

This started as a data-structures course project. The first versions had real bugs (a wrong pruning step in the k-d tree search, an out-of-bounds heap, a double free in the trie delete), which is why this version is rewritten around randomized tests against brute-force references.

## References

- J. L. Bentley, "Multidimensional binary search trees used for associative searching", *Communications of the ACM*, 1975.
- M. D. Atkinson, J.-R. Sack, N. Santoro, T. Strothotte, "Min-max heaps and generalized priority queues", *Communications of the ACM*, 1986.
