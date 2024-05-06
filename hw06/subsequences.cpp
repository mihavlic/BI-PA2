#ifndef __PROGTEST__
    #include <algorithm>
    #include <array>
    #include <cassert>
    #include <compare>
    #include <cstdlib>
    #include <deque>
    #include <functional>
    #include <iomanip>
    #include <iostream>
    #include <iterator>
    #include <list>
    #include <map>
    #include <memory>
    #include <set>
    #include <stdexcept>
    #include <string>
    #include <unordered_map>
    #include <unordered_set>
    #include <vector>

using namespace std::literals;

class CDummy {
  public:
    CDummy(char c) : m_C(c) {}

    bool operator==(const CDummy& other) const = default;

    friend std::ostream& operator<<(std::ostream& os, const CDummy& x) {
        return os << '\'' << x.m_C << '\'';
    }

  private:
    char m_C;
};
#endif /* __PROGTEST__ */

#define TEST_EXTRA_INTERFACE

using SequenceIndex = int;
using NodeHandle = int;

constexpr SequenceIndex SEQUENCE_END = -1;
constexpr NodeHandle ROOT_NODE = 0;

struct Node {
    SequenceIndex start;
    SequenceIndex end;
    NodeHandle suffix_link = ROOT_NODE;
    std::vector<NodeHandle> children = {};

    Node(SequenceIndex start, SequenceIndex end) : start(start), end(end) {}
};

class NewType {
    char inner;

  public:
    NewType(char c) : inner(c) {}

    NewType& operator=(const NewType& other) = default;

    std::strong_ordering operator<=>(const NewType& other) const = default;

    friend std::ostream& operator<<(std::ostream& os, const NewType& t) {
        return os << t.inner;
    }
};

template<typename T>
struct SequenceItem {
    const T* ptr = nullptr;

    SequenceItem() {}

    SequenceItem(T* ptr) : ptr(ptr) {}

    bool operator==(const SequenceItem<T>& other) const {
        if (!ptr || !other.ptr) {
            return ptr == other.ptr;
        }
        return *ptr == *other.ptr;
    }
};

// https://www.geeksforgeeks.org/print-all-possible-combinations-of-r-elements-in-a-given-array-of-size-n/
void make_combinations_(
    int input[],
    int scratch[],
    std::vector<int>& output,
    int start,
    int end,
    int index,
    int r
) {
    if (index == r) {
        for (int j = 0; j < r; j++) {
            output.push_back(scratch[j]);
        }
        return;
    }

    for (int i = start; i <= end && end - i + 1 >= r - index; i++) {
        scratch[index] = input[i];
        make_combinations_(input, scratch, output, i + 1, end, index + 1, r);
    }
}

void make_combinations(
    int arr[],
    int n,
    int r,
    std::vector<int>& scratch,
    std::vector<int>& output
) {
    scratch.clear();
    scratch.resize(r, 0);

    make_combinations_(arr, scratch.data(), output, 0, n - 1, 0, r);
}

template<typename T>
class CSelfMatch {
    std::vector<T> items {};
    mutable std::vector<Node> nodes {};

    mutable int remainder = 0;
    mutable int active_len = 0;
    mutable SequenceIndex active_edge_prefix_index = 0;
    mutable NodeHandle active_node = ROOT_NODE;

  public:
    CSelfMatch() {
        build();
    }

    template<typename I>
    CSelfMatch(I start, I end) : items(start, end) {
        build();
    }

    CSelfMatch(std::initializer_list<T> items) : items(items) {
        auto a = std::vector(items);
        build();
    }

    template<typename C>
    CSelfMatch(const C& collection) :
        CSelfMatch(collection.begin(), collection.end()) {}

    void build() {
        active_node = new_node(0, 0);
        for (SequenceIndex pos = 0; pos < (SequenceIndex)items.size(); pos++) {
            extend_suffix(pos);
        }
    }

    SequenceItem<T> get(SequenceIndex item) {
        if (item == SEQUENCE_END) {
            return &items.back();
        } else if (item >= (SequenceIndex)items.size()) {
            return nullptr;
        } else {
            return &items[item];
        }
    }

    NodeHandle* get_active_edge() {
        auto prefix = get(active_edge_prefix_index);
        return get_edge(active_node, prefix);
    }

    NodeHandle* get_edge(NodeHandle node, SequenceItem<T> edge_prefix) {
        auto& children = nodes[active_node].children;

        for (unsigned i = 0; i < children.size(); i++) {
            const Node& child = nodes[children[i]];
            if (get(child.start) == edge_prefix) {
                return &children[i];
            }
        }

        return nullptr;
    }

    NodeHandle* insert_edge(NodeHandle from, NodeHandle to) {
        auto& children = nodes[from].children;
        children.push_back(to);
        return &children.back();
    }

    NodeHandle* set_or_insert_edge(NodeHandle from, NodeHandle to) {
        auto prefix = get(nodes[to].start);
        NodeHandle* active = get_edge(from, prefix);

        if (active) {
            *active = to;
            return active;
        }

        return insert_edge(from, to);
    }

    NodeHandle new_node(SequenceIndex start, SequenceIndex end) {
        NodeHandle index = (NodeHandle)nodes.size();
        nodes.push_back(Node(start, end));
        return index;
    }

    void add_suffix_link(NodeHandle* previous, NodeHandle next) {
        if (*previous != -1) {
            nodes[*previous].suffix_link = next;
        }
        *previous = next;
    }

    SequenceIndex edge_length(NodeHandle handle, SequenceIndex len) const {
        Node& node = nodes[handle];
        SequenceIndex start = node.start;
        SequenceIndex end = node.end;
        if (end == -1) {
            end = len;
        }
        return end - start;
    }

    bool walk_down(NodeHandle node, SequenceIndex pos) {
        SequenceIndex edge_len = edge_length(node, pos + 1);
        if (active_len >= edge_len) {
            active_edge_prefix_index += edge_len;
            active_len -= edge_len;
            active_node = node;
            return true;
        }
        return false;
    }

    // resources used:
    //   https://stackoverflow.com/a/9513423
    //   https://marknelson.us/posts/1996/08/01/suffix-trees.html
    //   https://gist.github.com/makagonov/f7ed8ce729da72621b321f0ab547debb
    void extend_suffix(SequenceIndex pos) {
        NodeHandle previous = -1;
        remainder++;
        auto item = get(pos);

        while (remainder > 0) {
            if (active_len == 0) {
                active_edge_prefix_index = pos;
            }

            NodeHandle* active_edge_handle = get_active_edge();
            if (!active_edge_handle) {
                NodeHandle leaf = new_node(pos, SEQUENCE_END);
                insert_edge(active_node, leaf);
                add_suffix_link(&previous, active_node);
            } else {
                if (walk_down(*active_edge_handle, pos)) {
                    continue;
                }

                Node& active_edge = nodes[*active_edge_handle];
                auto current_item = get(active_edge.start + active_len);

                if (current_item == item) {
                    add_suffix_link(&previous, active_node);
                    active_len++;
                    break;
                }

                SequenceIndex start = active_edge.start;
                SequenceIndex middle = active_edge.start + active_len;

                active_edge.start = middle;
                NodeHandle split = new_node(start, middle);
                NodeHandle leaf = new_node(pos, SEQUENCE_END);

                insert_edge(split, leaf);
                insert_edge(split, *active_edge_handle);
                *active_edge_handle = split;

                add_suffix_link(&previous, split);
            }

            remainder--;

            if (active_node == ROOT_NODE) {
                if (active_len > 0) {
                    active_len--;
                    active_edge_prefix_index = pos - remainder + 1;
                }
            } else {
                active_node = nodes[active_node].suffix_link;
            }
        }
    }

    void flush() {
        extend_suffix((SequenceIndex)items.size());
    }

    void print() {
        print_(ROOT_NODE, 0);
    }

    void print_(NodeHandle handle, int indent) {
        Node& node = nodes[handle];
        if (handle == ROOT_NODE) {
            std::cout << "STree:\n";
        } else {
            SequenceIndex start = node.start;
            SequenceIndex end = node.end;
            if (end == SEQUENCE_END) {
                end = (SequenceIndex)items.size();
            }

            for (int i = 0; i < indent; i++) {
                std::cout << "  ";
            }

            std::cout << "'";
            for (SequenceIndex i = start; i < end; i++) {
                std::cout << items[i];
            }
            std::cout << "'";

            std::cout << "\t " << start << ".." << end << "\n";
        }

        for (auto child : node.children) {
            print_(child, indent + 1);
        }
    }

#ifdef TEST_EXTRA_INTERFACE
    void push_back(T element) {
        items.push_back(element);
        extend_suffix((SequenceIndex)(items.size() - 1));
    }

    template<typename... Aargs>
    void push_back(T first, Aargs... rest) {
        push_back(first);
        push_back(rest...);
    }
#endif

    template<typename F>
    int collect_leaf_counts(NodeHandle handle, SequenceIndex start, F fun) {
        Node& node = nodes[handle];

        SequenceIndex offset = start + edge_length(handle, items.size());

        if (node.children.empty()) {
            node.suffix_link = 1;
        } else {
            node.suffix_link = 0;
            for (auto child : node.children) {
                node.suffix_link += collect_leaf_counts(child, offset, fun);
            }
        }

        fun(handle, node, start, offset);

        return node.suffix_link;
    }

    template<typename F>
    void inspect_annotated_tree(F fun) const {
        CSelfMatch<T>* crimes = const_cast<CSelfMatch<T>*>(this);

        std::vector<Node> _nodes = crimes->nodes;
        int _remainder = crimes->remainder;
        int _active_len = crimes->active_len;
        SequenceIndex _active_edge_prefix_index =
            crimes->active_edge_prefix_index;
        NodeHandle _active_node = crimes->active_node;

        crimes->flush();
        fun(*crimes);

        std::swap(crimes->nodes, _nodes);
        std::swap(crimes->remainder, _remainder);
        std::swap(crimes->active_len, _active_len);
        std::swap(crimes->active_edge_prefix_index, _active_edge_prefix_index);
        std::swap(crimes->active_node, _active_node);
    }

    void collect_leaf_indices(
        NodeHandle handle,
        SequenceIndex path_length,
        std::vector<SequenceIndex>& indices
    ) const {
        Node& node = nodes[handle];

        if (node.children.empty()) {
            indices.push_back(node.start - path_length);
            return;
        }

        SequenceIndex offset = path_length + edge_length(handle, items.size());
        for (auto child : node.children) {
            collect_leaf_indices(child, offset, indices);
        }
    }

    // template<typename F>
    // void visit_nodes(NodeHandle handle, F fun) const {
    //     const Node& node = nodes[handle];
    //     fun(node);
    //     for (auto child : node.children) {
    //         visit_nodes(child, fun);
    //     }
    // }

    size_t sequenceLen(size_t n) const {
        if (n == 0) {
            throw std::invalid_argument("n must be nonzero");
        }

        NodeHandle max_len = 0;

        inspect_annotated_tree([&](CSelfMatch<T>& tree) {
            tree.collect_leaf_counts(
                ROOT_NODE,
                0,
                [&](NodeHandle handle,
                    const Node& node,
                    SequenceIndex start,
                    SequenceIndex end) {
                    if (node.suffix_link >= (NodeHandle)n) {
                        max_len = std::max(max_len, end);
                    }
                }
            );
        });

        return (size_t)max_len;
    }

    template<size_t N>
    std::vector<std::array<size_t, N>> findSequences() const {
        if (N == 0) {
            throw std::invalid_argument("n must be nonzero");
        }

        std::vector<int> scratch;
        std::vector<int> indices;
        std::vector<int> combinations;
        std::vector<std::array<size_t, N>> output;

        inspect_annotated_tree([&](CSelfMatch<T>& tree) {
            NodeHandle max_len = 1;
            std::vector<std::pair<NodeHandle, SequenceIndex>> nodes;

            tree.collect_leaf_counts(
                ROOT_NODE,
                0,
                [&](NodeHandle handle,
                    const Node& node,
                    SequenceIndex start,
                    SequenceIndex end) {
                    if (node.suffix_link >= (SequenceIndex)N) {
                        if (end > max_len) {
                            max_len = end;
                            nodes.clear();
                        }
                        if (end >= max_len) {
                            nodes.push_back({handle, start});
                        }
                    }
                }
            );

            for (size_t i = 0; i < nodes.size(); i++) {
                indices.clear();
                scratch.clear();
                combinations.clear();

                collect_leaf_indices(nodes[i].first, nodes[i].second, indices);

                make_combinations(
                    indices.data(),
                    (int)indices.size(),
                    N,
                    scratch,
                    combinations
                );

                for (size_t i = 0; i < combinations.size(); i += N) {
                    std::array<size_t, N> arr {};
                    for (size_t j = 0; j < N; j++) {
                        arr[j] = (size_t)combinations[i + j];
                    }

                    std::sort(arr.begin(), arr.end());
                    output.push_back(arr);
                }
            }
        });

        return output;
    }
};

template<typename I>
CSelfMatch(I start, I end)
    -> CSelfMatch<typename std::iterator_traits<I>::value_type>;

template<typename C>
CSelfMatch(const C& collection) -> CSelfMatch<typename C::value_type>;

#ifndef __PROGTEST__

template<typename T>
void assert_eq(const T& a, const T& b) {
    if (a != b) {
        std::cout << "Expected: ";
        std::cout << b;
        std::cout << std::endl;

        std::cout << "Got:      ";
        std::cout << a;
        std::cout << std::endl;
        assert(false);
    }
}

template<size_t N>
bool assert_arr_eq(
    std::vector<std::array<size_t, N>> a,
    std::vector<std::array<size_t, N>> b
) {
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    if (a != b) {
        std::cout << "Expected:\n";
        for (size_t i = 0; i < b.size(); i++) {
            for (size_t j = 0; j < N; j++) {
                std::cout << " " << b[i][j];
            }
            std::cout << std::endl;
        }

        std::cout << "Got:\n";
        for (size_t i = 0; i < a.size(); i++) {
            for (size_t j = 0; j < N; j++) {
                std::cout << " " << a[i][j];
            }
            std::cout << std::endl;
        }

        return false;
    }
    return true;
}

int main() {
    CSelfMatch<char> x0("aaaaaaaaaaa"s);
    assert(x0.sequenceLen(2) == 10);
    assert(assert_arr_eq(
        x0.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {{0, 1}}
    ));

    CSelfMatch<char> x1("abababababa"s);
    assert(x1.sequenceLen(2) == 9);
    assert(assert_arr_eq(
        x1.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {{0, 2}}
    ));
    CSelfMatch<char> x2("abababababab"s);
    assert(x2.sequenceLen(2) == 10);
    assert(assert_arr_eq(
        x2.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {{0, 2}}
    ));
    CSelfMatch<char> x3("aaaaaaaaaaa"s);
    assert(x3.sequenceLen(3) == 9);
    assert(assert_arr_eq(
        x3.findSequences<3>(),
        std::vector<std::array<size_t, 3>> {{0, 1, 2}}
    ));
    CSelfMatch<char> x4("abababababa"s);
    assert(x4.sequenceLen(3) == 7);
    assert(assert_arr_eq(
        x4.findSequences<3>(),
        std::vector<std::array<size_t, 3>> {{0, 2, 4}}
    ));
    CSelfMatch<char> x5("abababababab"s);
    assert(x5.sequenceLen(3) == 8);
    assert(assert_arr_eq(
        x5.findSequences<3>(),
        std::vector<std::array<size_t, 3>> {{0, 2, 4}}
    ));
    CSelfMatch<char> x6("abcdXabcd"s);
    assert(x6.sequenceLen(1) == 9);
    assert(assert_arr_eq(
        x6.findSequences<1>(),
        std::vector<std::array<size_t, 1>> {{0}}
    ));
    CSelfMatch<char> x7("abcdXabcd"s);
    assert(x7.sequenceLen(2) == 4);
    assert(assert_arr_eq(
        x7.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {{0, 5}}
    ));
    CSelfMatch<char> x8("abcdXabcdeYabcdZabcd"s);
    assert(x8.sequenceLen(2) == 4);
    assert(assert_arr_eq(
        x8.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {
            {0, 5},
            {0, 11},
            {0, 16},
            {5, 11},
            {5, 16},
            {11, 16}
        }
    ));
    CSelfMatch<char> x9("abcdXabcdYabcd"s);
    assert(x9.sequenceLen(3) == 4);
    assert(assert_arr_eq(
        x9.findSequences<3>(),
        std::vector<std::array<size_t, 3>> {{0, 5, 10}}
    ));
    CSelfMatch<char> x10("abcdefghijklmn"s);
    assert(x10.sequenceLen(2) == 0);
    assert(assert_arr_eq(
        x10.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {}
    ));
    CSelfMatch<char> x11("abcXabcYabcZdefXdef"s);
    assert(x11.sequenceLen(2) == 3);
    assert(assert_arr_eq(
        x11.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {{0, 4}, {0, 8}, {4, 8}, {12, 16}}
    ));
    CSelfMatch<int> x12 {1, 2, 3, 1, 2, 4, 1, 2};
    assert(x12.sequenceLen(2) == 2);
    assert(assert_arr_eq(
        x12.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {{0, 3}, {0, 6}, {3, 6}}
    ));
    assert(x12.sequenceLen(3) == 2);
    assert(assert_arr_eq(
        x12.findSequences<3>(),
        std::vector<std::array<size_t, 3>> {{0, 3, 6}}
    ));
    std::initializer_list<CDummy> init13 {
        'a',
        'b',
        'c',
        'd',
        'X',
        'a',
        'b',
        'c',
        'd',
        'Y',
        'a',
        'b',
        'c',
        'd'
    };
    CSelfMatch<CDummy> x13(init13.begin(), init13.end());
    assert(x13.sequenceLen(2) == 4);
    assert(assert_arr_eq(
        x13.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {{0, 5}, {0, 10}, {5, 10}}
    ));
    std::initializer_list<int> init14 {1, 2, 1, 1, 2, 1, 0, 0, 1, 2,
                                       1, 0, 1, 2, 0, 1, 2, 0, 1, 1,
                                       1, 2, 0, 2, 0, 1, 2, 1, 0};
    CSelfMatch<int> x14(init14.begin(), init14.end());
    assert(x14.sequenceLen(2) == 5);
    assert(assert_arr_eq(
        x14.findSequences<2>(),
        std::vector<std::array<size_t, 2>> {{11, 14}, {7, 24}}
    ));
    std::initializer_list<int> init15 {1, 2, 1, 1, 2, 1, 0, 0, 1, 2,
                                       1, 0, 1, 2, 0, 1, 2, 0, 1, 1,
                                       1, 2, 0, 2, 0, 1, 2, 1, 0};
    CSelfMatch<int> x15(init15.begin(), init15.end());
    assert(x15.sequenceLen(3) == 4);
    assert(assert_arr_eq(
        x15.findSequences<3>(),
        std::vector<std::array<size_t, 3>> {{3, 8, 25}}
    ));

    #ifdef TEST_EXTRA_INTERFACE
    CSelfMatch y0("aaaaaaaaaaa"s);
    assert(y0.sequenceLen(2) == 10);

    std::string s1("abcd");
    CSelfMatch y1(s1.begin(), s1.end());
    assert(y1.sequenceLen(2) == 0);

    CSelfMatch y2(""s);
    y2.push_back('a', 'b', 'c', 'X');
    y2.push_back('a');
    y2.push_back('b', 'c');
    assert(y2.sequenceLen(2) == 3);
    #endif /* TEST_EXTRA_INTERFACE */

    return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
