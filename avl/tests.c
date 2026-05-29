#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avl.h"

static int tests_passed = 0;
static int tests_failed = 0;

void check(const char *name, int cond) {
    if (cond){
        printf("[OK] %s\n", name);
        tests_passed++;
    } else {
        printf("[FAIL] %s\n", name);
        tests_failed++;
    }
}

void test_make_node() {
    AVLNode *node = createAVLNode("python");

    check("createAVLNode: key скопирован корректно", strcmp(node->key, "python") == 0);
    check("createAVLNode: height == 0", node->height == 0);
    check("createAVLNode: left == NULL", node->left == NULL);
    check("createAVLNode: right == NULL", node->right == NULL);
    check("createAVLNode: postings != NULL", node->postings != NULL);

    freeAVLNode(node);
}

void test_tree_create() {
    AVLTree *tree = createAVLTree();

    check("createAVLTree: root == NULL", tree->root == NULL);
    check("createAVLTree: size == 0", tree->size == 0);

    freeAVLTree(tree);
}

void test_get_height() {
    check("avlGetHeight: NULL возвращает -1", avlGetHeight(NULL) == -1);

    AVLNode *node = createAVLNode("go");
    check("avlGetHeight: листовой узел == 0", avlGetHeight(node) == 0);

    freeAVLNode(node);
}

void test_get_balance() {
    check("avlGetBalance: NULL возвращает 0", avlGetBalance(NULL) == 0);

    AVLNode *node = createAVLNode("go");
    check("avlGetBalance: листовой узел == 0", avlGetBalance(node) == 0);

    freeAVLNode(node);
}

void test_insert_single() {
    AVLTree *tree = createAVLTree();

    avlInsert(tree, "python", 1, "doc1");

    check("avlInsert: root != NULL после вставки", tree->root != NULL);
    check("avlInsert: size == 1 после первой вставки", tree->size == 1);
    check("avlInsert: root->key == python", strcmp(tree->root->key, "python") == 0);

    freeAVLTree(tree);
}

void test_insert_multiple() {
    AVLTree *tree = createAVLTree();

    avlInsert(tree, "python", 1, "doc1");
    avlInsert(tree, "java", 2, "doc2");
    avlInsert(tree, "c", 3, "doc3");

    check("avlInsert: size == 3 после трёх вставок", tree->size == 3);

    freeAVLTree(tree);
}

void test_insert_duplicate() {
    AVLTree *tree = createAVLTree();

    avlInsert(tree, "python", 1, "doc1");
    avlInsert(tree, "python", 2, "doc2");

    check("avlInsert: size не растёт при дубликате", tree->size == 1);

    freeAVLTree(tree);
}

void test_bst_order() {
    AVLTree *tree = createAVLTree();

    avlInsert(tree, "python", 1, "doc1");
    avlInsert(tree, "java", 2, "doc2");
    avlInsert(tree, "rust", 3, "doc3");

    // java < python < rust — java слева, rust справа
    check("avlInsert: BST порядок слева", strcmp(tree->root->left->key, tree->root->key) < 0);
    check("avlInsert: BST порядок справа", strcmp(tree->root->right->key, tree->root->key) > 0);

    freeAVLTree(tree);
}

void test_search() {
    AVLTree *tree = createAVLTree();

    avlInsert(tree, "python", 1, "doc1");
    avlInsert(tree, "java", 2, "doc2");
    avlInsert(tree, "rust", 3, "doc3");

    const Vector *result = avlSearch(tree, "python");
    check("avlSearch: search по ключу не NULL", result != NULL);

    const Vector *missing = avlSearch(tree, "go");
    check("avlSearch: несуществующий ключ возвращает NULL", missing == NULL);

    freeAVLTree(tree);
}


void test_balance(){
    AVLTree *tree = createAVLTree();

    // вставка по возрастанию — без балансировки дало бы вырожденное дерево высотой 9
    const char* keys[] = {"a","b","c","d","e","f","g","h","i","j"};
    for (int i = 0; i < 10; i++)
        avlInsert(tree, keys[i], i, "doc");

    check("avlBalance: высота сбалансированного дерева <= 4", avlGetHeight(tree->root) <= 4);

    freeAVLTree(tree);
}

typedef struct { char keys[10][64]; int count; } TraverseResult;

static void collectKey(const char* key, Vector* postings, void* ctx){
    (void)postings;
    TraverseResult* r = ctx;
    strcpy(r->keys[r->count++], key);
}

void test_traverse() {
    AVLTree *tree = createAVLTree();

    avlInsert(tree, "python", 1, "doc1");
    avlInsert(tree, "java", 2, "doc2");
    avlInsert(tree, "rust", 3, "doc3");

    TraverseResult r = { .count = 0 };
    avlTraverse(tree, collectKey, &r);

    check("avlTraverse: все 3 узла обойдены", r.count == 3);
    check("avlTraverse: in-order порядок (алфавитный)",
          strcmp(r.keys[0], r.keys[1]) < 0 && strcmp(r.keys[1], r.keys[2]) < 0);

    freeAVLTree(tree);
}

int main() {
    test_make_node();
    test_tree_create();
    test_get_height();
    test_get_balance();
    test_insert_single();
    test_insert_multiple();
    test_insert_duplicate();
    test_bst_order();
    test_search();
    test_balance();
    test_traverse();

    printf("\n%d успешно, %d провалилось\n", tests_passed, tests_failed);

    return 0;
}
