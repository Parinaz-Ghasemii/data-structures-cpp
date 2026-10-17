// Reads:  n, then n points ("x y"), then a query point.
// Prints: the tree in level order (one point per line), then the nearest point to the query.
#include <iostream>
#include <vector>
#include "kd_tree.hpp"

int main() {
    int n;
    if (!(std::cin >> n) || n <= 0) {
        std::cout << "No points.\n";
        return 0;
    }
    std::vector<Point> points(n);
    for (Point& p : points) std::cin >> p.x >> p.y;
    Point target;
    std::cin >> target.x >> target.y;

    KDTree tree(points);
    for (const Point& p : tree.levelOrder()) std::cout << p.x << ' ' << p.y << '\n';
    Point nn = tree.nearest(target);
    std::cout << nn.x << ' ' << nn.y << '\n';
}
