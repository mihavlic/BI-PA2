#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

template<typename T>
using KeyOrdering = int(T*, T*);
template<typename T>
using KeySearch = int(T*);
template<typename T>
using KeySearchPredicate = bool(T*);

// AVL tree modified to use arena allocation
// https://www.geeksforgeeks.org/insertion-in-an-avl-tree/
template<typename T>
struct Node {
    T key;
    Node* left = nullptr;
    Node* right = nullptr;
    int height = 1;

    Node(T key_) : key(key_) {}

    ~Node() {
        if (left) {
            delete left;
        }
        if (right) {
            delete right;
        }
    }

    int get_balance() const {
        int left_height = left ? left->height : 0;
        int right_height = right ? right->height : 0;
        return left_height - right_height;
    }

    void update_height() {
        int left_height = left ? left->height : 0;
        int right_height = right ? right->height : 0;
        height = std::max(left_height, right_height) + 1;
    }

    /*

    left rotate subtree rooted with y

       y               x
      / \             / \
     T1  x    -->    y  T2
        / \         / \
       z  T2       T1  z

  */
    static void node_rotate_left(Node** node) {
        Node* y = *node;
        Node* x = y->right;
        Node* z = x->left;

        x->left = y;
        y->right = z;

        y->update_height();
        x->update_height();

        *node = x;
    }

    /*

    right rotate subtree rooted with y

         y               x
        / \             / \
       x  T2    -->    T1  y
      / \                 / \
     T1  z               z  T2

  */
    static void node_rotate_right(Node** node) {
        Node* y = *node;
        Node* x = y->left;
        Node* z = x->right;

        x->right = y;
        y->left = z;

        y->update_height();
        x->update_height();

        *node = x;
    }

    static void maybe_rebalance_node(Node** node) {
        Node* root = *node;

        if (root == nullptr) {
            return;
        }

        root->update_height();
        int balance = root->get_balance();

        if (balance > 1) {  // left heavy
            if (root->left->get_balance() == -1) {
                node_rotate_left(&root->left);
            }

            node_rotate_right(node);
        } else if (balance < -1) {  // right heavy
            if (root->right->get_balance() == 1) {
                node_rotate_right(&root->right);
            }

            node_rotate_left(node);
        }
    }

    static void find_min_by(
        Node<T>** node,
        KeySearchPredicate<T> search,
        std::vector<Node<T>**>& path
    ) {
        while (true) {
            Node<T>* root = *node;

            if (root == NULL || !search(&root->left->key)) {
                return;
            }

            path.push_back(node);
            node = &root->left;
        }
    }

    static void find_max_by(
        Node<T>** node,
        KeySearchPredicate<T> search,
        std::vector<Node<T>**>& path
    ) {
        while (true) {
            Node<T>* root = *node;

            if (root == NULL || !search(&root->right->key)) {
                return;
            }

            path.push_back(node);
            node = &root->right;
        }
    }

    // Recursive function to insert a key in the subtree rooted
    // with node and returns the new root of the subtree.
    static void
    find(Node<T>** node, KeySearch<T> search, std::vector<Node<T>**>& path) {
        while (true) {
            path.push_back(node);

            Node<T>* root = *node;

            if (root == NULL) {
                return;
            }

            int ordering = search(&root->key);
            if (ordering < 0) {
                node = &root->left;
            } else if (ordering > 0) {
                node = &root->right;
            } else {
                return;
            }
        }
    }

    static void rebalance_path(std::vector<Node<T>**>& path) {
        for (auto it = path.rbegin(); it != path.rend(); ++it) {
            maybe_rebalance_node(it);
        }
    }

    // Recursive function to insert a key in the subtree rooted
    // with node and returns the new root of the subtree.
    static void insert(Node<T>** node, T key, KeyOrdering<T> compare) {
        Node<T>* root = *node;

        // if inserting into empty, create new node
        if (root == NULL) {
            *node = new Node(key);
            return;
        }

        int ordering = compare(&key, &root->key);
        if (ordering < 0) {
            insert(&root->left, key, compare);
        } else if (ordering > 0) {
            insert(&root->right, key, compare);
        } else {
            root->key = key;
            return;
        }

        // we've inserted a node into one of our children, rebalance
        maybe_rebalance_node(node);
    }

    Node* node_get_leftmost() {
        Node* prev = nullptr;
        Node* next = this;
        while (next) {
            prev = next;
            next = next->left;
        }
        return prev;
    }

    static void remove(Node** node, T& key, KeyOrdering<T> compare) {
        Node* root = *node;

        if (root == NULL) {
            return;
        }

        int ordering = compare(&key, &root->key);
        if (ordering < 0) {
            remove(&root->left, key, compare);
        } else if (ordering > 0) {
            remove(&root->right, key, compare);
        } else {
            if (root->left && root->right) {
                // get the inorder successor (smallest in the right subtree)
                Node* leftmost = root->right->node_get_leftmost();
                // leftmost is now root
                root->key = leftmost->key;
                // delete the old leftmost, this is a bit inelegant
                remove(&root->right, leftmost->key, compare);
            } else {
                if (root->left) {
                    *node = root->left;
                    root->left = nullptr;
                } else {
                    *node = root->right;
                    root->right = nullptr;
                }
                delete root;
                return;
            }
        }

        // we've removed one of our children, rebalance
        maybe_rebalance_node(node);
    }

    static void _node_print(const Node* node, int indent) {
        if (node) {
            for (int i = 0; i < indent; i++) {
                printf(" ");
            }
            printf("%d\n", node->key);
            _node_print(node->left, indent + 1);
            _node_print(node->right, indent + 1);
        }
    }

    static void _node_print_order(const Node* node, int indent) {
        if (node) {
            _node_print_order(node->left, indent + 1);
            printf("%d ", node->key);
            _node_print_order(node->right, indent + 1);
        }
        if (indent == 0) {
            printf("\n");
        }
    }
};

template<typename T>
class AvlTree {
    Node<T>* root;
    KeyOrdering<T>* compare;

  public:
    AvlTree(KeyOrdering<T> compare_) : root(nullptr), compare(compare_) {}

    ~AvlTree() {
        if (root) {
            delete root;
        }
    }

    void find(KeySearch<T> search, std::vector<Node<T>**>& path) {
        path.clear();
        Node<T>::find(&this->root, search, path);
    }

    void rebalance_path(std::vector<Node<T>**>& path) {
        Node<T>::rebalance_path(&this->root, path);
    }

    void insert(T key) {
        Node<T>::insert(&this->root, key, this->compare);
    }

    void remove(T key) {
        Node<T>::remove(&this->root, key, this->compare);
    }

    void print() const {
        _node_print(root, 0);
    }

    void print_order() const {
        Node<T>::_node_print_order(root, 0);
    }
};

// stolen from
// https://www.geeksforgeeks.org/binary-tree-iterator-for-inorder-traversal/
template<typename T>
class AvlIterator {
  private:
    std::vector<Node<T>*> stack;

  public:
    AvlIterator() {}

    AvlIterator(Node<T>* root) {
        moveLeft(root);
    }

    void moveLeft(Node<T>* current) {
        while (current) {
            stack.push_back(current);
            current = current->left;
        }
    }

    bool atEnd() const {
        return stack.empty();
    }

    Node<T>* next() {
        if (atEnd()) {
            return nullptr;
        }

        Node<T>& current = stack.back();
        stack.pop_back();

        if (current.right) {
            moveLeft(current.right);
        }

        return current;
    }
};

int make_value(int i) {
    return (i * i * 127 + ~i) % 64;
}

#if false
int main() {
  auto compare = [](int *a, int *b) -> int { return *a - *b; };
  AvlTree<int> root(compare);

  int len = 4;
  for (int i = 0; i < len; i++) {
    int value = make_value(i);
    root.insert(value);
  }

  root.print_order();

  for (int i = 0; i < len; i++) {
    int value = make_value(i);
    root.remove(value);
  }

  root.print_order();
  return 0;
}
#endif
