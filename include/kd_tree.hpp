#pragma once
// 2-D k-d tree.
//
// Every node splits the plane with a vertical line (x) or a horizontal line (y); the
// split axis alternates with depth. A subtree on the "left" holds coordinates <= the
// node's coordinate on that axis, the "right" subtree holds coordinates >= it.
//
//   KDTree(points)      balanced build by median: O(n log^2 n) (sorts each level)
//   insert              O(depth); keeps working but does not rebalance
//   contains            O(depth) on average
//   nearest / kNearest  average O(log n) on well-spread data, worst case O(n):
//                       a branch is skipped only if the splitting line is farther away
//                       than the best candidates found so far
//   withinRadius        range query: all points within a given distance
//
// Distances are squared integer distances (no floating point, no rounding issues).
// Recursion depth equals the tree height, so build trees from a vector (balanced)
// rather than inserting millions of already-sorted points one by one.

#include <algorithm>
#include <cstddef>
#include <memory>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

struct Point {
    int x = 0;
    int y = 0;
    int label = 0;     // optional class label, used by the k-NN example
};

inline long long squaredDistance(const Point& a, const Point& b) {
    long long dx = static_cast<long long>(a.x) - b.x;
    long long dy = static_cast<long long>(a.y) - b.y;
    return dx * dx + dy * dy;
}

class KDTree {
public:
    KDTree() = default;

    explicit KDTree(std::vector<Point> points) : size_(points.size()) {
        root_ = build(points, 0, static_cast<int>(points.size()) - 1, 0);
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    int height() const { return height(root_.get()); }

    void insert(const Point& p) {
        ++size_;
        if (!root_) { root_ = std::make_unique<Node>(p, 0); return; }
        Node* n = root_.get();
        while (true) {
            bool goLeft = coord(p, n->axis) < coord(n->p, n->axis);
            auto& next = goLeft ? n->left : n->right;
            if (!next) { next = std::make_unique<Node>(p, 1 - n->axis); return; }
            n = next.get();
        }
    }

    // Is there a point with the same (x, y)? (label is ignored)
    bool contains(const Point& q) const { return contains(root_.get(), q); }

    // Closest stored point to q (any one of them if several are equally close).
    Point nearest(const Point& q) const {
        if (!root_) throw std::out_of_range("KDTree is empty");
        checks_ = 0;
        const Node* best = nullptr;
        long long bestD = 0;
        nearest(root_.get(), q, best, bestD);
        return best->p;
    }

    // The k closest points, nearest first (fewer if the tree holds fewer than k).
    std::vector<Point> kNearest(const Point& q, std::size_t k) const {
        checks_ = 0;
        std::vector<Point> out;
        if (k == 0 || !root_) return out;
        std::priority_queue<std::pair<long long, const Node*>> heap;   // farthest of the k on top
        kNearest(root_.get(), q, k, heap);
        out.resize(heap.size());
        for (std::size_t i = out.size(); i-- > 0; heap.pop()) out[i] = heap.top().second->p;
        return out;
    }

    // All points within `radius` of q (inclusive), in no particular order.
    std::vector<Point> withinRadius(const Point& q, int radius) const {
        checks_ = 0;
        std::vector<Point> out;
        if (radius < 0) return out;
        within(root_.get(), q, static_cast<long long>(radius) * radius, out);
        return out;
    }

    std::vector<Point> levelOrder() const {
        std::vector<Point> out;
        if (!root_) return out;
        std::queue<const Node*> q;
        q.push(root_.get());
        while (!q.empty()) {
            const Node* n = q.front();
            q.pop();
            out.push_back(n->p);
            if (n->left) q.push(n->left.get());
            if (n->right) q.push(n->right.get());
        }
        return out;
    }

    // How many points the last nearest/kNearest/withinRadius call measured a distance to.
    std::size_t lastDistanceChecks() const { return checks_; }

private:
    struct Node {
        Point p;
        int axis;                          // 0 = split on x, 1 = split on y
        std::unique_ptr<Node> left, right;
        Node(const Point& pt, int ax) : p(pt), axis(ax) {}
    };

    std::unique_ptr<Node> root_;
    std::size_t size_ = 0;
    mutable std::size_t checks_ = 0;

    static int coord(const Point& p, int axis) { return axis == 0 ? p.x : p.y; }

    static std::unique_ptr<Node> build(std::vector<Point>& pts, int lo, int hi, int axis) {
        if (lo > hi) return nullptr;
        std::sort(pts.begin() + lo, pts.begin() + hi + 1,
                  [axis](const Point& a, const Point& b) { return coord(a, axis) < coord(b, axis); });
        int mid = lo + (hi - lo) / 2;
        auto node = std::make_unique<Node>(pts[mid], axis);
        node->left = build(pts, lo, mid - 1, 1 - axis);
        node->right = build(pts, mid + 1, hi, 1 - axis);
        return node;
    }

    static int height(const Node* n) {
        return n ? 1 + std::max(height(n->left.get()), height(n->right.get())) : 0;
    }

    static bool contains(const Node* n, const Point& q) {
        if (!n) return false;
        if (n->p.x == q.x && n->p.y == q.y) return true;
        int a = coord(q, n->axis), b = coord(n->p, n->axis);
        if (a < b) return contains(n->left.get(), q);
        if (a > b) return contains(n->right.get(), q);
        // Equal on the split axis: equal coordinates may be on either side.
        return contains(n->left.get(), q) || contains(n->right.get(), q);
    }

    void nearest(const Node* n, const Point& q, const Node*& best, long long& bestD) const {
        if (!n) return;
        ++checks_;
        long long d = squaredDistance(n->p, q);
        if (!best || d < bestD) { best = n; bestD = d; }

        long long diff = static_cast<long long>(coord(q, n->axis)) - coord(n->p, n->axis);
        const Node* nearSide = diff < 0 ? n->left.get() : n->right.get();
        const Node* farSide = diff < 0 ? n->right.get() : n->left.get();

        nearest(nearSide, q, best, bestD);
        // The far side can only hold something closer if the splitting line is closer
        // than the best point found so far. (The original version compared against the
        // wrong candidate here, which could discard the true nearest point.)
        if (diff * diff < bestD) nearest(farSide, q, best, bestD);
    }

    using Heap = std::priority_queue<std::pair<long long, const Node*>>;

    void kNearest(const Node* n, const Point& q, std::size_t k, Heap& heap) const {
        if (!n) return;
        ++checks_;
        long long d = squaredDistance(n->p, q);
        if (heap.size() < k) heap.push({d, n});
        else if (d < heap.top().first) { heap.pop(); heap.push({d, n}); }

        long long diff = static_cast<long long>(coord(q, n->axis)) - coord(n->p, n->axis);
        const Node* nearSide = diff < 0 ? n->left.get() : n->right.get();
        const Node* farSide = diff < 0 ? n->right.get() : n->left.get();

        kNearest(nearSide, q, k, heap);
        if (heap.size() < k || diff * diff < heap.top().first) kNearest(farSide, q, k, heap);
    }

    void within(const Node* n, const Point& q, long long r2, std::vector<Point>& out) const {
        if (!n) return;
        ++checks_;
        if (squaredDistance(n->p, q) <= r2) out.push_back(n->p);
        long long diff = static_cast<long long>(coord(q, n->axis)) - coord(n->p, n->axis);
        const Node* nearSide = diff < 0 ? n->left.get() : n->right.get();
        const Node* farSide = diff < 0 ? n->right.get() : n->left.get();
        within(nearSide, q, r2, out);
        if (diff * diff <= r2) within(farSide, q, r2, out);
    }
};
