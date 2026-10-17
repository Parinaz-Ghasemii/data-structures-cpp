// Reads:  n, then n integers.
// Prints: the minimum, the maximum, then all values in ascending order
//         (obtained by repeatedly removing the minimum).
#include <iostream>
#include "min_max_heap.hpp"

int main() {
    int n;
    if (!(std::cin >> n) || n <= 0) {
        std::cout << "Empty heap.\n";
        return 0;
    }
    MinMaxHeap heap;
    for (int i = 0; i < n; ++i) {
        int x;
        std::cin >> x;
        heap.push(x);
    }
    std::cout << "min " << heap.findMin() << "\nmax " << heap.findMax() << '\n';
    while (!heap.empty()) std::cout << heap.popMin() << (heap.empty() ? '\n' : ' ');
}
