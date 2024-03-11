#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

typedef struct {
  void *allocation;
  int size;
  int capacity;
} ArrayList;

void list_push(ArrayList *list, void *element, int size) {
  int new_size = list->size + size;
  if (new_size > list->capacity) {
    int new_capacity = list->capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 8;
    }
    if (new_capacity < new_size) {
      new_capacity = new_size;
    }
    list->capacity = new_capacity;
    list->allocation = realloc(list->allocation, new_capacity);
  }
  memcpy((char *)list->allocation + list->size, element, size);
  list->size = new_size;
}

void list_free(ArrayList *list) {
  if (list->allocation) {
    free(list->allocation);
  }
}

typedef int NodeIndex;

// AVL tree modified to use arena allocation and allow duplicate nodes
// https://www.geeksforgeeks.org/insertion-in-an-avl-tree/
typedef struct {
  int key;
  NodeIndex left;
  NodeIndex right;
  // the number of child nodecounts + refcount
  int refcount;
  int nodecount;
  int height;
} Node;

void node_init(Node *node, int key) {
  node->key = key;
  node->left = -1;
  node->right = -1;
  node->refcount = 1;
  node->nodecount = 1;
  node->height = 1;
}

Node *get_node(NodeIndex index, ArrayList *arena) {
  if (index < 0) {
    return NULL;
  }
  assert(index * sizeof(Node) < arena->size);
  return ((Node *)arena->allocation) + index;
}

int node_get_height(NodeIndex index, ArrayList *arena) {
  Node *node = get_node(index, arena);
  if (node) {
    return node->height;
  } else {
    return 0;
  }
}

// get balance factor of node
int node_get_balance(NodeIndex index, ArrayList *arena) {
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

void node_update_for_children(Node *node, ArrayList *arena) {
  Node *left = get_node(node->left, arena);
  Node *right = get_node(node->right, arena);

  int left_nodecount = left ? left->nodecount : 0;
  int right_nodecount = right ? right->nodecount : 0;
  node->nodecount = node->refcount + left_nodecount + right_nodecount;

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
NodeIndex node_rotate_left(NodeIndex y_index, ArrayList *arena) {
  Node *y = get_node(y_index, arena);
  NodeIndex x_index = y->right;

  Node *x = get_node(x_index, arena);
  NodeIndex z_index = x->left;

  x->left = y_index;
  y->right = z_index;

  node_update_for_children(y, arena);
  node_update_for_children(x, arena);

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
NodeIndex node_rotate_right(NodeIndex y_index, ArrayList *arena) {
  Node *y = get_node(y_index, arena);
  NodeIndex x_index = y->left;

  Node *x = get_node(x_index, arena);
  NodeIndex z_index = x->right;

  x->right = y_index;
  y->left = z_index;

  node_update_for_children(y, arena);
  node_update_for_children(x, arena);

  return x_index;
}

NodeIndex push_node(int key, ArrayList *arena) {
  Node node;
  node_init(&node, key);

  NodeIndex inserted_index = arena->size / sizeof(Node);
  list_push(arena, &node, sizeof(Node));

  return inserted_index;
}

NodeIndex maybe_rebalance_node(Node *root, NodeIndex index, int key,
                               ArrayList *arena) {
  node_update_for_children(root, arena);

  int balance = node_get_balance(index, arena);

  // if this node becomes unbalanced, then there are 4 cases
  Node *left_node = get_node(root->left, arena);
  Node *right_node = get_node(root->right, arena);

  // Left Left Case
  if (balance > 1 && key < left_node->key) {
    return node_rotate_right(index, arena);
  }

  // Left Right Case
  if (balance > 1 && key > left_node->key) {
    NodeIndex n = node_rotate_left(root->left, arena);
    // root possibly invalidated
    root = get_node(index, arena);
    root->left = n;
    return node_rotate_right(index, arena);
  }

  // Right Left Case
  if (balance < -1 && key < right_node->key) {
    NodeIndex n = node_rotate_right(root->right, arena);
    // root possibly invalidated
    root = get_node(index, arena);
    root->right = n;
    return node_rotate_left(index, arena);
  }

  // Right Right Case
  if (balance < -1 && key > right_node->key) {
    return node_rotate_left(index, arena);
  }

  node_update_for_children(root, arena);
  return index;
}

// Recursive function to insert a key in the subtree rooted
// with node and returns the new root of the subtree.
NodeIndex insert_node(NodeIndex index, int key, ArrayList *arena) {
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
    // the keys are equal, we can just increase the refcount
    assert(root->nodecount > 0);
    root->nodecount += 1;
    return index;
  }

  // we've inserted a node into one of our children, rebalance
  return maybe_rebalance_node(root, index, key, arena);
}

NodeIndex node_get_leftmost(NodeIndex node, ArrayList *arena) {
  NodeIndex current = node;
  Node *next = NULL;
  while ((next = get_node(current, arena))) {
    current = next->left;
  }
  return current;
}

NodeIndex delete_node(NodeIndex index, int key, ArrayList *arena) {
  Node *root = get_node(index, arena);

  if (!root) {
    assert(!"tree doesn't contain key");
    return index;
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
    Node *left_node = get_node(root->left, arena);
    Node *right_node = get_node(root->left, arena);

    root->refcount -= 1;

    // refcount decremented but not zero
    if (root->refcount > 0) {
      return index;
    }

    if (!left_node && !right_node) { // no children
      return (NodeIndex)-1;
    } else if (!right_node) { // only left
      return root->left;
    } else if (!left_node) { // only right
      return root->right;
    } else { // both children
      // Get the inorder successor (smallest in the right subtree)
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
  return maybe_rebalance_node(root, index, key, arena);
}

// Returns the nth element (zero-indexed) in tree
// https://en.wikipedia.org/wiki/Order_statistic_tree#Augmented_search_tree_implementation
NodeIndex tree_select(NodeIndex root, int nth, ArrayList *arena) {
  NodeIndex index = root;
  int i = nth;
  while (true) {
    Node *t = get_node(index, arena);
    if (t == NULL) {
      assert(!"huh");
    }

    printf("nth:%d key:%d\n", i, t->key);

    Node *l = get_node(t->left, arena);
    int p = l ? l->nodecount : 0;
    if (p == i) {
      return index;
    } else if (i < p) {
      nth = i;
      index = t->left;
    } else {
      nth = i - p;
      index = t->right;
    }
  }
}

int find_median(NodeIndex index, ArrayList *arena) {
  Node *root = get_node(index, arena);
  if (root == NULL) {
    return -1;
  }
  printf("nodecount:%d\n", root->nodecount);
  NodeIndex found = tree_select(index, (root->nodecount - 1) / 2, arena);
  return get_node(found, arena)->key;
}

int main() {
  ArrayList arena = {};
  NodeIndex root = -1;

  int arr[] = {1, 1, 1};

  for (int i = 0; i < sizeof(arr) / sizeof(int); i++) {
    printf("%d ", arr[i]);
    root = insert_node(root, arr[i], &arena);
  }
  printf("\n");

  int median = find_median(root, &arena);

  printf("Median: %d\n", median);

  list_free(&arena);
  return 0;
}