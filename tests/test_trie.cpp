// Randomised comparison of StandardTrie and CompressedTrie against std::set.
#include <algorithm>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include "check.hpp"
#include "trie.hpp"

static std::vector<std::string> expectedWithPrefix(const std::set<std::string>& ref, const std::string& prefix) {
    std::vector<std::string> out;
    for (const std::string& w : ref)
        if (w.compare(0, prefix.size(), prefix) == 0) out.push_back(w);
    return out;                                   // std::set iterates in alphabetical order
}

int main() {
    // the example from the original program
    {
        CompressedTrie t;
        for (const char* w : {"apple", "banana", "app", "application"}) t.insert(w);
        std::vector<std::string> expected = {"app", "apple", "application"};
        CHECK(t.wordsWithPrefix("app") == expected);
        CHECK(t.contains("banana") && !t.contains("ban"));
    }
    // the delete case that crashed the original (double free)
    {
        StandardTrie s;
        CompressedTrie c;
        for (const char* w : {"app", "apple"}) { s.insert(w); c.insert(w); }
        CHECK(s.erase("apple") && c.erase("apple"));
        CHECK(s.contains("app") && c.contains("app"));
        CHECK(!s.contains("apple") && !c.contains("apple"));
    }
    // invalid input
    {
        StandardTrie s;
        bool threw = false;
        try { s.insert("Apple"); } catch (const std::invalid_argument&) { threw = true; }
        CHECK(threw);
        CHECK(!s.contains("Apple") && !s.erase("Apple") && !s.startsWith("A"));
    }

    // random operations over a tiny alphabet so that words share long prefixes
    for (int seed = 0; seed < 300; ++seed) {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> len(1, 7), letter(0, 2), op(0, 9);
        auto randomWord = [&](int maxLen) {
            std::string w;
            int l = std::min(len(rng), maxLen);
            for (int i = 0; i < l; ++i) w += static_cast<char>('a' + letter(rng));
            return w;
        };
        StandardTrie s;
        CompressedTrie c;
        std::set<std::string> ref;
        for (int step = 0; step < 200; ++step) {
            std::string w = randomWord(7);
            if (op(rng) < 6) {
                bool added = ref.insert(w).second;
                CHECK(s.insert(w) == added);
                CHECK(c.insert(w) == added);
            } else {
                bool removed = ref.erase(w) == 1;
                CHECK(s.erase(w) == removed);
                CHECK(c.erase(w) == removed);
            }
            CHECK(s.size() == ref.size() && c.size() == ref.size());
            CHECK(c.isCompressed());
            CHECK(c.nodeCount() <= s.nodeCount());

            std::string probe = randomWord(7);
            CHECK(s.contains(probe) == (ref.count(probe) == 1));
            CHECK(c.contains(probe) == (ref.count(probe) == 1));
            std::string prefix = probe.substr(0, std::uniform_int_distribution<int>(0, (int)probe.size())(rng));
            std::vector<std::string> expected = expectedWithPrefix(ref, prefix);
            CHECK(s.wordsWithPrefix(prefix) == expected);
            CHECK(c.wordsWithPrefix(prefix) == expected);
            CHECK(s.startsWith(prefix) == !expected.empty());
            CHECK(c.startsWith(prefix) == !expected.empty());
        }
        // erase everything: both tries must shrink back to just the root
        std::vector<std::string> all(ref.begin(), ref.end());
        for (const std::string& w : all) { CHECK(s.erase(w)); CHECK(c.erase(w)); CHECK(c.isCompressed()); }
        CHECK(s.size() == 0 && c.size() == 0);
        CHECK(s.nodeCount() == 1 && c.nodeCount() == 1);
    }
    std::cout << "trie: all tests passed\n";
}
