// Inserts a few words into a standard trie and a compressed trie, lists the words that
// start with "app", compares the number of nodes, then deletes a word.
#include <iostream>
#include "trie.hpp"

int main() {
    const char* words[] = {"apple", "banana", "app", "application", "apply", "band"};
    StandardTrie standard;
    CompressedTrie compressed;
    for (const char* w : words) {
        standard.insert(w);
        compressed.insert(w);
    }

    std::cout << "words starting with \"app\":";
    for (const std::string& w : compressed.wordsWithPrefix("app")) std::cout << ' ' << w;
    std::cout << "\n\n";

    std::cout << "nodes (standard trie):   " << standard.nodeCount() << '\n';
    std::cout << "nodes (compressed trie): " << compressed.nodeCount() << "\n\n";

    standard.erase("apple");
    compressed.erase("apple");
    std::cout << "after erasing \"apple\":\n";
    std::cout << "  contains apple? " << compressed.contains("apple")
              << ", contains app? " << compressed.contains("app") << '\n';
    std::cout << "  nodes (standard):   " << standard.nodeCount() << '\n';
    std::cout << "  nodes (compressed): " << compressed.nodeCount() << '\n';
}
