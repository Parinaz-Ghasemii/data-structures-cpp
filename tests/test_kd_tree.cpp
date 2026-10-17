// Randomised comparison of KDTree against brute force.
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <set>
#include <utility>
#include "check.hpp"
#include "kd_tree.hpp"

static std::vector<long long> bruteDistances(const std::vector<Point>& pts, const Point& q) {
    std::vector<long long> d;
    for (const Point& p : pts) d.push_back(squaredDistance(p, q));
    std::sort(d.begin(), d.end());
    return d;
}

static void checkAgainstBruteForce(const KDTree& tree, const std::vector<Point>& pts, std::mt19937& rng, int coordRange) {
    std::uniform_int_distribution<int> c(-coordRange, coordRange);
    for (int t = 0; t < 20; ++t) {
        Point q;
        q.x = c(rng);
        q.y = c(rng);
        std::vector<long long> d = bruteDistances(pts, q);

        // nearest
        Point nn = tree.nearest(q);
        CHECK(squaredDistance(nn, q) == d[0]);

        // k nearest, several k (including k > n)
        for (std::size_t k : {std::size_t(1), std::size_t(3), std::size_t(7), pts.size() + 5}) {
            std::vector<Point> nb = tree.kNearest(q, k);
            CHECK(nb.size() == std::min(k, pts.size()));
            std::vector<long long> got;
            for (const Point& p : nb) got.push_back(squaredDistance(p, q));
            CHECK(std::is_sorted(got.begin(), got.end()));
            CHECK(std::equal(got.begin(), got.end(), d.begin()));
        }

        // radius query
        int radius = std::uniform_int_distribution<int>(0, coordRange)(rng);
        std::vector<Point> inside = tree.withinRadius(q, radius);
        std::vector<std::pair<int, int>> got, expected;
        for (const Point& p : inside) got.push_back({p.x, p.y});
        for (const Point& p : pts)
            if (squaredDistance(p, q) <= static_cast<long long>(radius) * radius) expected.push_back({p.x, p.y});
        std::sort(got.begin(), got.end());
        std::sort(expected.begin(), expected.end());
        CHECK(got == expected);

        // contains
        bool present = std::any_of(pts.begin(), pts.end(), [&](const Point& p) { return p.x == q.x && p.y == q.y; });
        CHECK(tree.contains(q) == present);
    }
    for (const Point& p : pts) CHECK(tree.contains(p));
}

int main() {
    // the case where the original returned the wrong neighbour
    {
        std::vector<Point> pts;
        const int xy[][2] = {{2, 14}, {1, 15}, {15, 1}, {15, 0}, {7, 12}, {12, 11}, {15, 12}, {13, 5}, {3, 5}, {11, 11}};
        for (const auto& p : xy) { Point pt; pt.x = p[0]; pt.y = p[1]; pts.push_back(pt); }
        KDTree tree(pts);
        Point q; q.x = 9; q.y = 12;
        Point nn = tree.nearest(q);
        CHECK(nn.x == 7 && nn.y == 12);
    }
    // empty tree
    {
        KDTree tree;
        CHECK(tree.empty() && tree.height() == 0);
        Point q;
        bool threw = false;
        try { tree.nearest(q); } catch (const std::out_of_range&) { threw = true; }
        CHECK(threw);
        CHECK(tree.kNearest(q, 3).empty());
        CHECK(tree.withinRadius(q, 10).empty());
        CHECK(!tree.contains(q));
        CHECK(tree.levelOrder().empty());
    }
    // random trees: built balanced, and built by repeated insert; many duplicates in small ranges
    for (int seed = 0; seed < 300; ++seed) {
        std::mt19937 rng(seed);
        int range = (seed % 3 == 0) ? 4 : (seed % 3 == 1 ? 20 : 1000);
        std::uniform_int_distribution<int> c(-range, range);
        int n = std::uniform_int_distribution<int>(1, 60)(rng);
        std::vector<Point> pts(n);
        for (Point& p : pts) { p.x = c(rng); p.y = c(rng); }

        KDTree built(pts);
        CHECK(built.size() == pts.size());
        CHECK(built.levelOrder().size() == pts.size());
        int maxHeight = 1;
        while ((1 << maxHeight) <= n) ++maxHeight;               // floor(log2 n) + 1
        CHECK(built.height() <= maxHeight);                      // median build is balanced
        checkAgainstBruteForce(built, pts, rng, range + 3);

        KDTree grown;
        for (const Point& p : pts) grown.insert(p);
        CHECK(grown.size() == pts.size());
        checkAgainstBruteForce(grown, pts, rng, range + 3);
    }
    std::cout << "kd-tree: all tests passed\n";
}
