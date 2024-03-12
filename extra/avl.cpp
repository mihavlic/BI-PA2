#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

typedef int NodeIndex;

template <typename T> class Box {
  T *ptr;

public:
  Box() : ptr(nullptr) {}
  Box(T value) : ptr(new T(value)) {}
  ~Box() {
    if (this->ptr) {
      delete this->ptr;
    }
  }
  T *get() { return this->ptr; }
};

// AVL tree modified to use arena allocation
// https://www.geeksforgeeks.org/insertion-in-an-avl-tree/
struct Node {
  int key;
  Node *left = nullptr;
  Node *right = nullptr;
  int height = 1;

  Node(int key_) : key(key_) {}
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
    int right_height = left ? left->height : 0;
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
  static void node_rotate_left(Node **node) {
    Node *y = *node;
    Node *x = y->right;
    Node *z = x->left;

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
  static void node_rotate_right(Node **node) {
    Node *y = *node;
    Node *x = y->left;
    Node *z = x->right;

    x->right = y;
    y->left = z;

    y->update_height();
    x->update_height();

    *node = x;
  }
  static void maybe_rebalance_node(Node **node) {
    Node *root = *node;

    root->update_height();
    int balance = root->get_balance();

    if (balance > 1) { // left heavy
      if (root->left->get_balance() == -1) {
        node_rotate_left(&root->left);
      }

      node_rotate_right(node);
    } else if (balance < -1) { // right heavy
      if (root->right->get_balance() == 1) {
        node_rotate_right(&root->right);
      }

      node_rotate_left(node);
    }
  }
  // Recursive function to insert a key in the subtree rooted
  // with node and returns the new root of the subtree.
  static void insert_node(Node **node, int key) {
    Node *root = *node;

    // if inserting into empty, create new node
    if (root == NULL) {
      *node = new Node(key);
    }

    if (key < root->key) {
      insert_node(&root->left, key);
    } else if (key > root->key) {
      insert_node(&root->right, key);
    } else {
      root->key = key;
      return;
    }

    // we've inserted a node into one of our children, rebalance
    maybe_rebalance_node(node);
  }
  Node *node_get_leftmost() {
    Node *prev = nullptr;
    Node *next = this;
    while (next) {
      prev = next;
      next = next->left;
    }
    return prev;
  }
  static void delete_node(Node **node, int key) {
    Node *root = *node;

    if (root == NULL) {
      return;
    }

    if (key < root->key) {
      delete_node(&root->left, key);
    } else if (key > root->key) {
      delete_node(&root->right, key);
    } else {
      if (!root->left && !root->right) { // no children
        *node = nullptr;
      } else if (!root->left) { // only left
        *node = root->right;
        return;
      } else if (!root->right) { // only right
        *node = root->left;
        return;
      } else { // both children
        // get the inorder successor (smallest in the right subtree)
        Node *leftmost = root->node_get_leftmost();
        // reparent the leftmost into root
        root->key = leftmost->key;
        // this is a bit inelegant
        delete_node(&root->right, leftmost->key);
      }
    }

    // we've removed one of our children, rebalance
    return maybe_rebalance_node(node);
  }
  void print() const { _node_print(this, 0); }
  static void _node_print(const Node *node, int indent) {
    if (node) {
      for (int i = 0; i < indent; i++) {
        printf(" ");
      }
      printf("%d\n", node->key);
      _node_print(node->left, indent + 1);
      _node_print(node->right, indent + 1);
    }
  }
  void print_order() const { _node_print_order(this, 0); }
  static void _node_print_order(const Node *node, int indent) {
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

int main() {
  Node *root = nullptr;

  for (int i = 0; i < 64; i++) {
    int value = (i * i + ~i) % 64;
    Node::insert_node(&root, value);
  }

  root->print_order();

  for (int i = 0; i < 64; i++) {
    int value = (i * i + ~i) % 64;
    Node::delete_node(&root, value);
  }

  printf("\n");
  root->print_order();

  delete root;
  return 0;
}