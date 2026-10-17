#pragma once
// Two string sets for lowercase words (a-z):
//
//   StandardTrie   one character per edge, 26-way nodes.
//   CompressedTrie (radix tree) edges carry whole substrings, so chains of
//                  single-child nodes collapse into one node.
//
// Both support insert / contains / erase in O(L) for a word of length L, and list
// all words with a given prefix in alphabetical order. nodeCount() makes the memory
// difference between the two visible.

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace trie_detail {
inline bool isValidWord(const std::string& w) {
    if (w.empty()) return false;
    for (char c : w) if (c < 'a' || c > 'z') return false;
    return true;
}
inline void requireValidWord(const std::string& w) {
    if (!isValidWord(w)) throw std::invalid_argument("words must be non-empty and use only a-z");
}
}  // namespace trie_detail

// ---------------------------------------------------------------------------
class StandardTrie {
    struct Node {
        std::array<std::unique_ptr<Node>, 26> next;
        bool isWord = false;
    };
    std::unique_ptr<Node> root_ = std::make_unique<Node>();
    std::size_t words_ = 0;

public:
    std::size_t size() const { return words_; }
    std::size_t nodeCount() const { return count(root_.get()); }

    bool insert(const std::string& w) {                    // false if already present
        trie_detail::requireValidWord(w);
        Node* n = root_.get();
        for (char c : w) {
            auto& slot = n->next[c - 'a'];
            if (!slot) slot = std::make_unique<Node>();
            n = slot.get();
        }
        if (n->isWord) return false;
        n->isWord = true;
        ++words_;
        return true;
    }

    bool contains(const std::string& w) const {
        const Node* n = find(w);
        return n && n->isWord;
    }

    // true if at least one stored word starts with `prefix`
    bool startsWith(const std::string& prefix) const {
        const Node* n = find(prefix);
        return n && (n != root_.get() || words_ > 0);
    }

    std::vector<std::string> wordsWithPrefix(const std::string& prefix) const {
        std::vector<std::string> out;
        if (const Node* n = find(prefix)) collect(n, prefix, out);
        return out;
    }

    bool erase(const std::string& w) {                     // false if not present
        if (!trie_detail::isValidWord(w)) return false;
        bool found = false;
        eraseRec(root_.get(), w, 0, found);
        if (found) --words_;
        return found;
    }

private:
    const Node* find(const std::string& s) const {
        const Node* n = root_.get();
        for (char c : s) {
            if (c < 'a' || c > 'z') return nullptr;
            n = n->next[c - 'a'].get();
            if (!n) return nullptr;
        }
        return n;
    }

    static bool hasChildren(const Node* n) {
        for (const auto& c : n->next) if (c) return true;
        return false;
    }

    // Returns true when `node` has become useless (no word ends here, no children),
    // so the caller can free it. Each node is freed exactly once, by its parent.
    bool eraseRec(Node* node, const std::string& w, std::size_t i, bool& found) {
        if (i == w.size()) {
            if (!node->isWord) return false;
            node->isWord = false;
            found = true;
        } else {
            auto& child = node->next[w[i] - 'a'];
            if (!child) return false;
            if (eraseRec(child.get(), w, i + 1, found)) child.reset();
        }
        return found && !node->isWord && !hasChildren(node);
    }

    static void collect(const Node* n, const std::string& path, std::vector<std::string>& out) {
        if (n->isWord) out.push_back(path);
        for (int i = 0; i < 26; ++i)
            if (n->next[i]) collect(n->next[i].get(), path + static_cast<char>('a' + i), out);
    }

    static std::size_t count(const Node* n) {
        std::size_t total = 1;
        for (const auto& c : n->next) if (c) total += count(c.get());
        return total;
    }
};

// ---------------------------------------------------------------------------
class CompressedTrie {
    struct Node {
        std::string label;                                  // text on the edge leading into this node
        bool isWord = false;
        std::map<char, std::unique_ptr<Node>> children;     // keyed by the first char of the child's label
    };
    std::unique_ptr<Node> root_ = std::make_unique<Node>();
    std::size_t words_ = 0;

public:
    std::size_t size() const { return words_; }
    std::size_t nodeCount() const { return count(root_.get()); }

    bool insert(const std::string& w) {                     // false if already present
        trie_detail::requireValidWord(w);
        Node* node = root_.get();
        std::size_t i = 0;
        while (true) {
            if (i == w.size()) {                            // the word ends exactly at `node`
                if (node->isWord) return false;
                node->isWord = true;
                ++words_;
                return true;
            }
            auto it = node->children.find(w[i]);
            if (it == node->children.end()) {               // no edge starts with w[i]: add a leaf
                auto leaf = std::make_unique<Node>();
                leaf->label = w.substr(i);
                leaf->isWord = true;
                node->children[w[i]] = std::move(leaf);
                ++words_;
                return true;
            }
            Node* child = it->second.get();
            std::size_t l = commonPrefix(child->label, w, i);
            if (l == child->label.size()) {                 // the whole edge matches: go down
                node = child;
                i += l;
                continue;
            }
            // The edge matches only partly: split it into [common part] -> [rest of the old edge]
            auto mid = std::make_unique<Node>();
            mid->label = child->label.substr(0, l);
            std::unique_ptr<Node> old = std::move(it->second);
            old->label = old->label.substr(l);
            char key = old->label[0];
            mid->children[key] = std::move(old);
            Node* midRaw = mid.get();
            it->second = std::move(mid);
            node = midRaw;
            i += l;                                         // loop again: mark `mid` as a word or add a leaf
        }
    }

    bool contains(const std::string& w) const {
        const Node* n = findExact(w);
        return n && n->isWord;
    }

    // true if at least one stored word starts with `prefix`
    bool startsWith(const std::string& prefix) const {
        std::string built;
        const Node* n = locate(prefix, built);
        return n && (n != root_.get() || words_ > 0);
    }

    std::vector<std::string> wordsWithPrefix(const std::string& prefix) const {
        std::vector<std::string> out;
        std::string built;
        if (const Node* n = locate(prefix, built)) collect(n, built, out);
        return out;
    }

    bool erase(const std::string& w) {                      // false if not present
        if (!trie_detail::isValidWord(w)) return false;
        std::vector<std::pair<Node*, char>> path;           // (parent, key of the edge we took)
        Node* node = root_.get();
        std::size_t i = 0;
        while (i < w.size()) {
            auto it = node->children.find(w[i]);
            if (it == node->children.end()) return false;
            Node* child = it->second.get();
            if (w.compare(i, child->label.size(), child->label) != 0) return false;
            path.push_back({node, w[i]});
            node = child;
            i += child->label.size();
        }
        if (!node->isWord) return false;
        node->isWord = false;
        --words_;

        if (node->children.empty() && !path.empty()) {      // it was a leaf: unlink it
            Node* parent = path.back().first;
            parent->children.erase(path.back().second);
            node = parent;
        }
        // A non-root, non-word node with a single child is redundant: merge the two.
        if (node != root_.get() && !node->isWord && node->children.size() == 1) merge(node);
        return true;
    }

    // Structural check used by the tests: every non-root node has a non-empty label,
    // and no non-root, non-word node has fewer than two children (it would not be compressed).
    bool isCompressed() const { return checkShape(root_.get(), true); }

private:
    static std::size_t commonPrefix(const std::string& label, const std::string& w, std::size_t i) {
        std::size_t k = 0;
        while (k < label.size() && i + k < w.size() && label[k] == w[i + k]) ++k;
        return k;
    }

    const Node* findExact(const std::string& w) const {
        const Node* node = root_.get();
        std::size_t i = 0;
        while (i < w.size()) {
            auto it = node->children.find(w[i]);
            if (it == node->children.end()) return nullptr;
            const Node* child = it->second.get();
            if (w.compare(i, child->label.size(), child->label) != 0) return nullptr;
            node = child;
            i += child->label.size();
        }
        return node;
    }

    // Finds the node whose subtree holds exactly the words that start with `prefix`
    // (the prefix may end in the middle of an edge). `built` receives the full text
    // from the root down to the returned node.
    const Node* locate(const std::string& prefix, std::string& built) const {
        const Node* node = root_.get();
        built.clear();
        std::size_t i = 0;
        while (i < prefix.size()) {
            auto it = node->children.find(prefix[i]);
            if (it == node->children.end()) return nullptr;
            const Node* child = it->second.get();
            std::size_t l = commonPrefix(child->label, prefix, i);
            std::size_t remaining = prefix.size() - i;
            if (l < child->label.size() && l < remaining) return nullptr;   // mismatch inside the edge
            built += child->label;
            node = child;
            i += child->label.size();       // may pass the end of the prefix: that is fine, the loop ends
        }
        return node;
    }

    static void merge(Node* n) {
        std::unique_ptr<Node> only = std::move(n->children.begin()->second);
        n->label += only->label;
        n->isWord = only->isWord;
        n->children = std::move(only->children);
    }

    static void collect(const Node* n, const std::string& path, std::vector<std::string>& out) {
        if (n->isWord) out.push_back(path);
        for (const auto& kv : n->children) collect(kv.second.get(), path + kv.second->label, out);
    }

    static std::size_t count(const Node* n) {
        std::size_t total = 1;
        for (const auto& kv : n->children) total += count(kv.second.get());
        return total;
    }

    static bool checkShape(const Node* n, bool isRoot) {
        if (!isRoot) {
            if (n->label.empty()) return false;
            if (!n->isWord && n->children.size() < 2) return false;
        }
        for (const auto& kv : n->children) {
            if (kv.second->label.empty() || kv.second->label[0] != kv.first) return false;
            if (!checkShape(kv.second.get(), false)) return false;
        }
        return true;
    }
};
