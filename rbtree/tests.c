#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"

static int tests_passed = 0;
static int tests_failed = 0;

void check(const char *name, int cond){
    if (cond){
        printf("[OK] %s\n", name);
        tests_passed++;
    } else {
        printf("[FAIL] %s\n", name);
        tests_failed++;
    }
}

void test_tree_create(){
    RBTree* tree = createRBTree();

    check("createRBTree: root == nil", tree->root == tree->nil);
    check("createRBTree: size == 0", tree->size == 0);
    check("createRBTree: nil is black", tree->nil->color == RB_BLACK);

    freeRBTree(tree);
}

void test_insert_single(){
    RBTree* tree = createRBTree();

    rbInsert(tree, "python", 1, "doc1");

    check("rbInsert: root != nil после вставки", tree->root != tree->nil);
    check("rbInsert: size == 1", tree->size == 1);
    check("rbInsert: root->key == python", strcmp(tree->root->key, "python") == 0);
    check("rbInsert: root is black", tree->root->color == RB_BLACK);

    freeRBTree(tree);
}

void test_insert_multiple(){
    RBTree* tree = createRBTree();

    rbInsert(tree, "python", 1, "doc1");
    rbInsert(tree, "java", 2, "doc2");
    rbInsert(tree, "c", 3, "doc3");

    check("rbInsert: size == 3 после трёх вставок", tree->size == 3);
    check("rbInsert: root is black после балансировки", tree->root->color == RB_BLACK);

    freeRBTree(tree);
}

void test_insert_duplicate(){
    RBTree* tree = createRBTree();

    rbInsert(tree, "python", 1, "doc1");
    rbInsert(tree, "python", 2, "doc2");

    check("rbInsert: size не растёт при дубликате", tree->size == 1);

    freeRBTree(tree);
}

void test_search(){
    RBTree* tree = createRBTree();

    rbInsert(tree, "python", 1, "doc1");
    rbInsert(tree, "java", 2, "doc2");
    rbInsert(tree, "rust", 3, "doc3");

    const Vector* result = rbSearch(tree, "python");
    check("rbSearch: существующий ключ не NULL", result != NULL);

    const Vector* missing = rbSearch(tree, "go");
    check("rbSearch: несуществующий ключ NULL", missing == NULL);

    freeRBTree(tree);
}

typedef struct { char keys[10][64]; int count; } TraverseResult;

static void collectKey(const char* key, Vector* postings, void* ctx){
    (void)postings;
    TraverseResult* r = ctx;
    strcpy(r->keys[r->count++], key);
}

void test_traverse(){
    RBTree* tree = createRBTree();

    rbInsert(tree, "python", 1, "doc1");
    rbInsert(tree, "java", 2, "doc2");
    rbInsert(tree, "rust", 3, "doc3");

    TraverseResult r = { .count = 0 };
    rbTraverse(tree, collectKey, &r);

    check("rbTraverse: все 3 узла обойдены", r.count == 3);
    check("rbTraverse: in-order порядок (алфавитный)",
          strcmp(r.keys[0], r.keys[1]) < 0 && strcmp(r.keys[1], r.keys[2]) < 0);

    freeRBTree(tree);
}

int main(){
    test_tree_create();
    test_insert_single();
    test_insert_multiple();
    test_insert_duplicate();
    test_search();
    test_traverse();

    printf("\n%d успешно, %d провалилось\n", tests_passed, tests_failed);

    return 0;
}
