#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

typedef int NodeIndex;

// AVL tree modified to use arena allocation
// https://www.geeksforgeeks.org/insertion-in-an-avl-tree/
typedef struct {
  int key;
  NodeIndex left;
  NodeIndex right;
  int height;
} Node;

void node_init(Node *node, int key) {
  node->key = key;
  node->left = -1;
  node->right = -1;
  node->height = 1;
}

Node *get_node(NodeIndex index, std::vector<Node> &arena) {
  if (index < 0) {
    return NULL;
  }
  return &arena[index];
}

int node_get_height(NodeIndex index, std::vector<Node> &arena) {
  Node *node = get_node(index, arena);
  if (node) {
    return node->height;
  } else {
    return 0;
  }
}

int node_get_balance(NodeIndex index, std::vector<Node> &arena) {
  Node *node = get_node(index, arena);
  if (node) {
    int left_height = node_get_height(node->left, arena);
    int right_height = node_get_height(node->right, arena);
    return left_height - right_height;
  } else {
    return 0;
  }
}

int max(int a, int b) { return (a > b) ? a : b; }

void node_update_height(Node *node, std::vector<Node> &arena) {
  Node *left = get_node(node->left, arena);
  Node *right = get_node(node->right, arena);

  int left_height = left ? left->height : 0;
  int right_height = right ? right->height : 0;
  node->height = max(left_height, right_height) + 1;
}

/*

  left rotate subtree rooted with y

     y               x
    / \             / \
   T1  x    -->    y  T2
      / \         / \
     z  T2       T1  z

*/
NodeIndex node_rotate_left(NodeIndex y_index, std::vector<Node> &arena) {
  Node *y = get_node(y_index, arena);
  NodeIndex x_index = y->right;

  Node *x = get_node(x_index, arena);
  NodeIndex z_index = x->left;

  x->left = y_index;
  y->right = z_index;

  node_update_height(y, arena);
  node_update_height(x, arena);

  return x_index;
}

/*

  right rotate subtree rooted with y

       y               x
      / \             / \
     x  T2    -->    T1  y
    / \                 / \
   T1  z               z  T2

*/
NodeIndex node_rotate_right(NodeIndex y_index, std::vector<Node> &arena) {
  Node *y = get_node(y_index, arena);
  NodeIndex x_index = y->left;

  Node *x = get_node(x_index, arena);
  NodeIndex z_index = x->right;

  x->right = y_index;
  y->left = z_index;

  node_update_height(y, arena);
  node_update_height(x, arena);

  return x_index;
}

NodeIndex push_node(int key, std::vector<Node> &arena) {
  Node node;
  node_init(&node, key);

  NodeIndex inserted_index = arena.size();
  arena.push_back(node);

  return inserted_index;
}

NodeIndex maybe_rebalance_node(Node *root, NodeIndex index,
                               std::vector<Node> &arena) {
  node_update_height(root, arena);

  int balance = node_get_balance(index, arena);

  // left heavy
  if (balance > 1) {
    if (node_get_balance(root->left, arena) == -1) {
      NodeIndex n = node_rotate_left(root->left, arena);

      // root possibly invalidated
      root = get_node(index, arena);
      root->left = n;
    }

    return node_rotate_right(index, arena);
  }

  // right heavy
  if (balance < -1) {
    if (node_get_balance(root->right, arena) == 1) {
      NodeIndex n = node_rotate_right(root->right, arena);

      // root possibly invalidated
      root = get_node(index, arena);
      root->right = n;
    }

    return node_rotate_left(index, arena);
  }

  // height is otherwise updated when rotating
  node_update_height(root, arena);
  return index;
}

// Recursive function to insert a key in the subtree rooted
// with node and returns the new root of the subtree.
NodeIndex insert_node(NodeIndex index, int key, std::vector<Node> &arena) {
  Node *root = get_node(index, arena);

  // if inserting into empty, create new node
  if (root == NULL) {
    return push_node(key, arena);
  }

  if (key < root->key) {
    NodeIndex n = insert_node(root->left, key, arena);
    // need to lookup the root gain, in case the root got resized
    root = get_node(index, arena);
    root->left = n;
  } else if (key > root->key) {
    NodeIndex n = insert_node(root->right, key, arena);
    // need to lookup the root gain, in case the root got resized
    root = get_node(index, arena);
    root->right = n;
  } else {
    root->key = key;
    return index;
  }

  // we've inserted a node into one of our children, rebalance
  return maybe_rebalance_node(root, index, arena);
}

NodeIndex node_get_leftmost(NodeIndex index, std::vector<Node> &arena) {
  NodeIndex prev = -1;
  NodeIndex next = index;
  while (Node *node = get_node(next, arena)) {
    prev = next;
    next = node->left;
  }
  return prev;
}

NodeIndex delete_node(NodeIndex index, int key, std::vector<Node> &arena) {
  Node *root = get_node(index, arena);

  if (root == NULL) {
    return -1;
  }

  if (key < root->key) {
    NodeIndex n = delete_node(root->left, key, arena);
    // root possibly invalidated
    root = get_node(index, arena);
    root->left = n;
  } else if (key > root->key) {
    NodeIndex n = delete_node(root->right, key, arena);
    // root possibly invalidated
    root = get_node(index, arena);
    root->right = n;
  } else {
    NodeIndex has_left = root->left >= 0;
    NodeIndex has_right = root->right >= 0;

    if (!has_left && !has_right) { // no children
      return -1;
    } else if (!has_right) { // only left
      return root->left;
    } else if (!has_left) { // only right
      return root->right;
    } else { // both children
      // get the inorder successor (smallest in the right subtree)
      NodeIndex leftmost_index = node_get_leftmost(root->right, arena);
      Node *leftmost = get_node(leftmost_index, arena);
      // reparent the leftmost into root
      root->key = leftmost->key;
      // this is a bit inelegant
      NodeIndex n = delete_node(root->right, leftmost->key, arena);
      // root possibly invalidated
      root = get_node(index, arena);
      root->right = n;
    }
  }

  // we've removed one of our children, rebalance
  return maybe_rebalance_node(root, index, arena);
}

void node_print(NodeIndex index, std::vector<Node> &arena, int indent) {
  Node *node = get_node(index, arena);
  if (node) {
    for (int i = 0; i < indent; i++) {
      printf(" ");
    }
    printf("%d\n", node->key);
    node_print(node->left, arena, indent + 1);
    node_print(node->right, arena, indent + 1);
  }
}

void node_print_order(NodeIndex index, std::vector<Node> &arena, int indent) {
  Node *node = get_node(index, arena);
  if (node) {
    node_print_order(node->left, arena, indent + 1);
    printf("%d ", node->key);
    node_print_order(node->right, arena, indent + 1);
  }
  if (indent == 0) {
    printf("\n");
  }
}

int main() {
  std::vector<Node> arena{};
  NodeIndex root = -1;

  for (int i = 0; i < 64; i++) {
    int value = (i * i + ~i) % 64;
    root = insert_node(root, value, arena);
  }

  node_print_order(root, arena, 0);

  for (int i = 0; i < 64; i++) {
    int value = (i * i + ~i) % 64;
    root = delete_node(root, value, arena);
  }

  printf("\n");
  node_print_order(root, arena, 0);

  return 0;
}