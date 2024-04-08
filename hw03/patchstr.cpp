#ifndef __PROGTEST__
    #include <cassert>
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <iostream>
    #include <memory>
    #include <stdexcept>
#endif /* __PROGTEST__ */

class Part {
    size_t offset;
    size_t size;
    std::shared_ptr<char[]> data;

  public:
    Part() : offset(0), size(0), data(nullptr) {}

    Part(const Part& copy) :
        offset(copy.offset),
        size(copy.size),
        data(copy.data) {}

    Part(const char* str) : offset(0), size(strlen(str)), data(nullptr) {
        data = std::shared_ptr<char[]>(new char[size + 1]);
        memcpy(data.get(), str, size);
        data[size] = 0;
    }

    // return the left part of the split
    Part split_left(size_t at) {
        assert(at > 0);

        Part copy(*this);
        copy.size = at;

        return copy;
    }

    // return the right part of the split
    Part split_right(size_t at) {
        assert(at < size);

        Part copy(*this);
        copy.offset += at;
        copy.size -= at;

        return copy;
    }

    bool empty() const {
        return size == 0;
    }

    size_t len() const {
        return size;
    }

    const char* str() const {
        return data.get() + offset;
    }
};

struct Node;
using SharedNode = std::shared_ptr<Node>;

void make_unique(SharedNode* ptr);

struct Node {
    SharedNode left = nullptr;
    SharedNode right = nullptr;
    Part leaf_data {};
    unsigned left_size = 0;
    unsigned right_size = 0;

    Node(Part data) : leaf_data(data), left_size(data.len()) {}

    Node(const char* str) : Node(Part(str)) {}

    Node(SharedNode left_, SharedNode right_) :
        left(left_),
        right(right_),
        left_size(left->size()),
        right_size(right->size()) {}

    unsigned size() const {
        return left_size + right_size;
    }
};

SharedNode concat2(SharedNode left, SharedNode right) {
    if (!left) {
        return right;
    }
    if (!right) {
        return left;
    }
    return std::make_shared<Node>(left, right);
}

SharedNode concat3(SharedNode left, SharedNode middle, SharedNode right) {
    return concat2(concat2(left, middle), right);
}

// find the node that covers the entire range
const SharedNode*
node_covering(const SharedNode& self, size_t start, size_t end) {
    unsigned total_len = self->size();
    if (start < total_len && end <= total_len) {
        const SharedNode* child = nullptr;
        if (self->left && (child = node_covering(self->left, start, end))) {
            return child;
        }
        start -= self->left_size;
        end -= self->left_size;
        if (self->right && (child = node_covering(self->right, start, end))) {
            return child;
        }
        return &self;
    } else {
        return nullptr;
    }
}

SharedNode split(const SharedNode& self, unsigned at, bool keep_left) {
    unsigned len = self->left_size;

    if (!self->left && !self->right) {
        SharedNode left = nullptr;
        if (keep_left && at > 0) {
            left = std::make_shared<Node>(Node(self->leaf_data.split_left(at)));
        }

        SharedNode right = nullptr;
        if (!keep_left && at < len) {
            right =
                std::make_shared<Node>(Node(self->leaf_data.split_right(at)));
        }

        if (keep_left) {
            return left;
        } else {
            return right;
        }
    }

    if (at < len) {
        SharedNode child = split(self->left, at, keep_left);
        if (keep_left) {
            return child;
        } else {
            return concat2(child, self->right);
        }
    } else {
        SharedNode child = split(self->right, at - len, keep_left);
        if (keep_left) {
            return concat2(self->left, child);
        } else {
            return child;
        }
    }
}

SharedNode insert_node(const SharedNode& self, unsigned at, SharedNode what) {
    unsigned len = self->left_size;

    if (!self->left && !self->right) {
        SharedNode left = nullptr;
        if (at > 0) {
            left = std::make_shared<Node>(Node(self->leaf_data.split_left(at)));
        }

        SharedNode right = nullptr;
        if (at < len) {
            right =
                std::make_shared<Node>(Node(self->leaf_data.split_right(at)));
        }

        return concat3(left, what, right);
    }

    if (at < len) {
        SharedNode child = insert_node(self->left, at, what);
        return concat2(child, self->right);
    } else {
        SharedNode child = insert_node(self->right, at - len, what);
        return concat2(self->left, child);
    }
}

template<typename F>
void visit_leafs(const SharedNode& self, F fun) {
    if (!self) {
        return;
    }
    if (!self->left && !self->right) {
        if (!self->leaf_data.empty()) {
            fun(self);
        }
    } else {
        visit_leafs<F>(self->left, fun);
        visit_leafs<F>(self->right, fun);
    }
}

void indent(int n) {
    for (int i = 0; i < n; i++) {
        printf(" ");
    }
}

void debug_node(const SharedNode& self, int level) {
    indent(level);
    if (!self) {
        printf("null\n");
    } else if (!self->left && !self->right) {
        printf("'%s'\n", self->leaf_data.str());
    } else {
        printf("left:\n");
        debug_node(self->left, level + 2);

        indent(level);
        printf("right:\n");
        debug_node(self->right, level + 2);
    }
}

class CPatchStr {
    SharedNode root = nullptr;

  public:
    CPatchStr() {}

    CPatchStr(SharedNode node) : root(node) {}

    CPatchStr(const CPatchStr& copy) {
        root = SharedNode(copy.root);
    }

    CPatchStr(const char* str) : root(std::make_shared<Node>(Node(str))) {}

    void operator=(const CPatchStr& other) {
        this->root = other.root;
    }

    void operator=(const char* str) {
        *this = CPatchStr(str);
    }

    bool empty() const {
        return !root;
    }

    unsigned size() const {
        if (empty()) {
            return 0;
        }
        return root->size();
    }

    CPatchStr subStr(size_t from, size_t len) const {
        unsigned total_size = size();
        if (from + len > total_size) {
            throw std::out_of_range("bungus");
        }

        if (len == 0) {
            return CPatchStr();
        }

        SharedNode copy;
        copy = split(root, from, false);
        copy = split(copy, len, true);

        return copy;
    }

    CPatchStr& append(const CPatchStr& src) {
        if (src.empty()) {
            return *this;
        }

        if (empty()) {
            *this = src;
        } else {
            Node new_root(std::move(root), src.root);
            root = std::make_shared<Node>(new_root);
        }

        return *this;
    }

    CPatchStr& insert(size_t pos, const CPatchStr& src) {
        unsigned total_size = size();
        if (pos > total_size) {
            throw std::out_of_range("bungus");
        }

        *this = insert_node(root, pos, src.root);

        return *this;
    }

    CPatchStr& remove(size_t from, size_t len) {
        unsigned total_size = size();
        if (from + len > total_size) {
            throw std::out_of_range("bungus");
        }

        if (len == 0) {
            return *this;
        }

        SharedNode left = split(root, from, true);
        SharedNode right = split(root, from + len, false);
        *this = concat2(left, right);

        return *this;
    }

    char* toStr() const {
        size_t size = 0;
        visit_leafs(root, [&size](const SharedNode& leaf) {
            const Part& data = leaf->leaf_data;
            size += data.len();
        });

        char* str = new char[size + 1];

        size_t offset = 0;
        visit_leafs(root, [&offset, &str](const SharedNode& leaf) {
            const Part& data = leaf->leaf_data;
            memcpy(str + offset, data.str(), data.len());
            offset += data.len();
        });
        str[offset] = 0;

        return str;
    }

    void debug() {
        debug_node(root, 0);
    }
};

#ifndef __PROGTEST__

bool stringMatch(const CPatchStr& pstr, const char* expected) {
    char* str = pstr.toStr();
    bool res = std::strcmp(str, expected) == 0;
    if (!res) {
        std::cout << "expected: " << expected << "\ngot:      " << str
                  << std::endl;
    }

    delete[] str;
    return res;
}

// #define FUZZ

    #ifdef FUZZ
        #include "../out/fuzz.cpp"
    #endif

int main() {
    #ifdef FUZZ
    fuzz();
    // return 0;
    #endif

    char tmpStr[100];

    CPatchStr g("aaaa");
    g = g;
    g = "aaaa";
    g.remove(0, 0);
    g.remove(4, 0);
    assert(stringMatch(g, "aaaa"));

    g.insert(4, "bbb");
    assert(stringMatch(g, "aaaabbb"));

    CPatchStr a("test");
    assert(stringMatch(a, "test"));
    std::strncpy(tmpStr, " da", sizeof(tmpStr) - 1);
    a.append(tmpStr);
    assert(stringMatch(a, "test da"));
    std::strncpy(tmpStr, "ta", sizeof(tmpStr) - 1);
    a.append(tmpStr);
    assert(stringMatch(a, "test data"));
    std::strncpy(tmpStr, "foo text", sizeof(tmpStr) - 1);
    CPatchStr b(tmpStr);
    assert(stringMatch(b, "foo text"));
    CPatchStr c(a);
    assert(stringMatch(c, "test data"));

    assert(stringMatch(a, "test data"));

    CPatchStr d(a.subStr(3, 5));
    assert(stringMatch(d, "t dat"));
    d.append(b);
    assert(stringMatch(d, "t datfoo text"));
    d.append(b.subStr(3, 4));
    assert(stringMatch(d, "t datfoo text tex"));
    c.append(d);
    assert(stringMatch(c, "test datat datfoo text tex"));
    c.append(c);
    assert(stringMatch(
        c,
        "test datat datfoo text textest datat datfoo text tex"
        //            atat datf
    ));
    assert(stringMatch(c.subStr(6, 9), "atat datf"));
    d.insert(2, c.subStr(6, 9));
    assert(stringMatch(d, "t atat datfdatfoo text tex"));
    b = "abcdefgh";
    assert(stringMatch(b, "abcdefgh"));
    assert(stringMatch(d, "t atat datfdatfoo text tex"));
    assert(stringMatch(d.subStr(4, 8), "at datfd"));
    assert(stringMatch(b.subStr(2, 6), "cdefgh"));
    try {
        b.subStr(2, 7);
        assert("Exception not thrown" == nullptr);
    } catch (const std::out_of_range& e) {
    } catch (...) {
        assert("Invalid exception thrown" == nullptr);
    }
    a.remove(3, 5);
    assert(stringMatch(a, "tesa"));
    return EXIT_SUCCESS;
}
#endif /* __PROGTEST__ */
