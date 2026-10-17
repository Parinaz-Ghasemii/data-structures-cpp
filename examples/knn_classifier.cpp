// k-nearest-neighbour classification on top of the k-d tree.
//
// Part 1: two noisy clusters; classify 200 test points by majority vote of their 5
//         nearest training points, and check the k-d tree finds the same neighbours
//         (same distances) as a brute-force scan.
// Part 2: how much work the tree saves: distance computations per query for 50,000
//         random points, compared with the 50,000 a brute-force scan needs.
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <vector>
#include "kd_tree.hpp"

static int majorityVote(const std::vector<Point>& neighbours) {
    std::map<int, int> votes;
    for (const Point& p : neighbours) ++votes[p.label];
    int best = -1, bestCount = -1;
    for (const auto& kv : votes)
        if (kv.second > bestCount) { best = kv.first; bestCount = kv.second; }
    return best;
}

static std::vector<long long> bruteForceDistances(const std::vector<Point>& train, const Point& q, std::size_t k) {
    std::vector<long long> d;
    for (const Point& p : train) d.push_back(squaredDistance(p, q));
    std::sort(d.begin(), d.end());
    d.resize(std::min(k, d.size()));
    return d;
}

int main() {
    std::mt19937 rng(42);   // fixed seed: the output is the same on every run

    // ---------- Part 1: classification ----------
    std::normal_distribution<double> noise(0.0, 60.0);
    const int centerX[2] = {200, 400}, centerY[2] = {200, 330};
    auto makeData = [&](int count) {
        std::vector<Point> v;
        for (int i = 0; i < count; ++i) {
            int c = i % 2;
            Point p;
            p.x = static_cast<int>(centerX[c] + noise(rng));
            p.y = static_cast<int>(centerY[c] + noise(rng));
            p.label = c;
            v.push_back(p);
        }
        return v;
    };
    std::vector<Point> train = makeData(300), test = makeData(200);
    KDTree tree(train);

    const std::size_t k = 5;
    int correct = 0, sameNeighbours = 0;
    double checks = 0;
    for (const Point& q : test) {
        std::vector<Point> nb = tree.kNearest(q, k);
        checks += static_cast<double>(tree.lastDistanceChecks());
        if (majorityVote(nb) == q.label) ++correct;
        std::vector<long long> d;
        for (const Point& p : nb) d.push_back(squaredDistance(p, q));
        if (d == bruteForceDistances(train, q, k)) ++sameNeighbours;
    }
    std::cout << "k-NN classification (k = 5, 300 training points, 200 test points)\n";
    std::cout << "  accuracy:                       " << correct << " / " << test.size() << '\n';
    std::cout << "  same neighbours as brute force: " << sameNeighbours << " / " << test.size() << '\n';
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  distance computations per query: " << checks / test.size()
              << " (brute force: " << train.size() << ")\n\n";

    // ---------- Part 2: scaling ----------
    std::uniform_int_distribution<int> coord(0, 100000);
    std::vector<Point> big(50000);
    for (Point& p : big) { p.x = coord(rng); p.y = coord(rng); }
    KDTree bigTree(big);
    double total = 0;
    const int queries = 1000;
    for (int i = 0; i < queries; ++i) {
        Point q; q.x = coord(rng); q.y = coord(rng);
        bigTree.nearest(q);
        total += static_cast<double>(bigTree.lastDistanceChecks());
    }
    std::cout << "nearest neighbour among " << big.size() << " random points (" << queries << " queries)\n";
    std::cout << "  distance computations per query: " << total / queries
              << " (brute force: " << big.size() << ")\n";
    std::cout << "  tree height: " << bigTree.height() << '\n';
}
