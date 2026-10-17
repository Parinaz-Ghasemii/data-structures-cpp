CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Iinclude -Itests
BUILD    = build

DEMOS = $(BUILD)/kd_tree_demo $(BUILD)/min_max_heap_demo $(BUILD)/trie_demo $(BUILD)/knn_classifier
TESTS = $(BUILD)/test_kd_tree $(BUILD)/test_min_max_heap $(BUILD)/test_trie

all: $(DEMOS) $(TESTS)

$(BUILD)/%: src/%.cpp $(wildcard include/*.hpp) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD)/%: examples/%.cpp $(wildcard include/*.hpp) | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD)/%: tests/%.cpp $(wildcard include/*.hpp) tests/check.hpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

test: $(TESTS)
	@for t in $(TESTS); do ./$$t || exit 1; done

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
