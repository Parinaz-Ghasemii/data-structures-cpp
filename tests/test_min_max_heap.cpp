// Randomised comparison of MinMaxHeap against std::multiset, checking the heap property after every step.
#include <iostream>
#include <random>
#include <set>
#include "check.hpp"
#include "min_max_heap.hpp"

int main() {
    // empty heap
    {
        MinMaxHeap h;
        CHECK(h.empty());
        bool threw = false;
        try { h.findMin(); } catch (const std::out_of_range&) { threw = true; }
        CHECK(threw);
        threw = false;
        try { h.popMax(); } catch (const std::out_of_range&) { threw = true; }
        CHECK(threw);
    }
    // the case that overflowed the original implementation
    {
        MinMaxHeap h;
        for (int x : {5, 4, 3, 2, 1}) h.push(x);
        CHECK(h.isValid());
        CHECK(h.findMin() == 1 && h.findMax() == 5);
    }
    // random operation sequences; small value ranges force many duplicates
    for (int seed = 0; seed < 300; ++seed) {
        std::mt19937 rng(seed);
        int range = (seed % 3 == 0) ? 5 : (seed % 3 == 1 ? 50 : 100000);
        std::uniform_int_distribution<int> value(-range, range);
        std::uniform_int_distribution<int> op(0, 9);
        MinMaxHeap h;
        std::multiset<int> ref;
        for (int step = 0; step < 300; ++step) {
            int o = op(rng);
            if (o < 5 || ref.empty()) {                       // push
                int v = value(rng);
                h.push(v);
                ref.insert(v);
            } else if (o < 8) {                               // popMin
                int got = h.popMin();
                CHECK(got == *ref.begin());
                ref.erase(ref.begin());
            } else {                                          // popMax
                int got = h.popMax();
                CHECK(got == *ref.rbegin());
                ref.erase(std::prev(ref.end()));
            }
            CHECK(h.size() == ref.size());
            CHECK(h.isValid());
            if (!ref.empty()) {
                CHECK(h.findMin() == *ref.begin());
                CHECK(h.findMax() == *ref.rbegin());
            }
        }
        // drain alternately from both ends
        bool fromMin = true;
        while (!ref.empty()) {
            if (fromMin) { CHECK(h.popMin() == *ref.begin()); ref.erase(ref.begin()); }
            else         { CHECK(h.popMax() == *ref.rbegin()); ref.erase(std::prev(ref.end())); }
            fromMin = !fromMin;
            CHECK(h.isValid());
        }
        CHECK(h.empty());
    }
    std::cout << "min-max heap: all tests passed\n";
}
