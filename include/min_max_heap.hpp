#pragma once
// Min-max heap: a complete binary tree stored in an array where
//   * nodes on even levels (root = level 0) are <= everything in their subtree  ("min levels")
//   * nodes on odd levels                   are >= everything in their subtree  ("max levels")
// so the minimum is at the root and the maximum is one of the root's two children.
//
//   findMin / findMax      : O(1)
//   push / popMin / popMax : O(log n)
//
// Reference: Atkinson, Sack, Santoro, Strothotte, "Min-Max Heaps and Generalized
// Priority Queues", Communications of the ACM, 1986.

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

class MinMaxHeap {
public:
    bool empty() const { return h_.size() == 1; }
    std::size_t size() const { return h_.size() - 1; }

    int findMin() const { requireNonEmpty(); return h_[1]; }
    int findMax() const { requireNonEmpty(); return h_[maxIndex()]; }

    void push(int x) {
        h_.push_back(x);
        bubbleUp(h_.size() - 1);
    }

    int popMin() { requireNonEmpty(); return removeAt(1); }
    int popMax() { requireNonEmpty(); return removeAt(maxIndex()); }

    // Checks the min-max property for every node (O(n log n)); used by the tests.
    bool isValid() const {
        for (std::size_t j = 2; j <= size(); ++j) {
            for (std::size_t a = j / 2; a >= 1; a /= 2) {
                if (isMinLevel(a) ? h_[a] > h_[j] : h_[a] < h_[j]) return false;
            }
        }
        return true;
    }

private:
    std::vector<int> h_{0};   // 1-based: h_[0] is unused

    void requireNonEmpty() const {
        if (empty()) throw std::out_of_range("MinMaxHeap is empty");
    }

    // Level of node i is floor(log2 i); even levels are "min" levels.
    static bool isMinLevel(std::size_t i) {
        int level = 0;
        while (i > 1) { i >>= 1; ++level; }
        return level % 2 == 0;
    }

    std::size_t maxIndex() const {
        std::size_t n = size();
        if (n == 1) return 1;
        if (n == 2) return 2;
        return h_[2] >= h_[3] ? 2 : 3;
    }

    // ----- insertion -----
    void bubbleUp(std::size_t i) {
        if (i == 1) return;
        std::size_t p = i / 2;
        if (isMinLevel(i)) {
            if (h_[i] > h_[p]) { std::swap(h_[i], h_[p]); bubbleUpMax(p); }
            else bubbleUpMin(i);
        } else {
            if (h_[i] < h_[p]) { std::swap(h_[i], h_[p]); bubbleUpMin(p); }
            else bubbleUpMax(i);
        }
    }
    void bubbleUpMin(std::size_t i) {            // compare with the grandparent (i / 4)
        while (i >= 4 && h_[i] < h_[i / 4]) { std::swap(h_[i], h_[i / 4]); i /= 4; }
    }
    void bubbleUpMax(std::size_t i) {
        while (i >= 4 && h_[i] > h_[i / 4]) { std::swap(h_[i], h_[i / 4]); i /= 4; }
    }

    // ----- removal -----
    int removeAt(std::size_t i) {
        int value = h_[i];
        h_[i] = h_.back();
        h_.pop_back();
        if (i < h_.size()) trickleDown(i);       // i was the last slot -> nothing left to fix
        return value;
    }

    void trickleDown(std::size_t i) {
        trickleDownImpl(i, isMinLevel(i));
    }

    // `isMin` selects the min-level or the max-level version (they mirror each other).
    void trickleDownImpl(std::size_t i, bool isMin) {
        auto better = [&](int a, int b) { return isMin ? a < b : a > b; };   // "a belongs nearer the top"
        while (true) {
            // m = best node among the children and grandchildren of i
            std::size_t m = 0;
            std::size_t n = size();
            const std::size_t candidates[6] = {2 * i, 2 * i + 1, 4 * i, 4 * i + 1, 4 * i + 2, 4 * i + 3};
            for (std::size_t c : candidates) {
                if (c <= n && (m == 0 || better(h_[c], h_[m]))) m = c;
            }
            if (m == 0) return;                                  // no descendants
            if (m >= 4 * i) {                                    // m is a grandchild
                if (!better(h_[m], h_[i])) return;
                std::swap(h_[m], h_[i]);
                std::size_t p = m / 2;
                if (better(h_[p], h_[m])) std::swap(h_[m], h_[p]);   // keep m's parent consistent
                i = m;
            } else {                                             // m is a child
                if (better(h_[m], h_[i])) std::swap(h_[m], h_[i]);
                return;
            }
        }
    }
};
