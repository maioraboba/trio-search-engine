#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "btree.h"
#ifdef _WIN32
#include <windows.h>
#endif


static int traverse_count = 0;

static void assert_visit(const char* key, Vector* postings, void* ctx) {
    traverse_count++;
}


// Тест 1: инициализация дерева
BTree* test_1_init() 
{
    BTree* tree = createBTree();
    
    assert(tree != NULL);
    assert(tree->root != NULL);
    assert(tree->root->n == 0);
    assert(tree->root->is_leaf == 1);
    
    printf("Тест 1 пройден\n");
    return tree;
}


// Тест 2: вставка и поиск
void test_2_single_insert(BTree* tree) 
{
    btreeInsert(tree, "linux", 101, "Intro");
    
    Vector* result = btreeSearch(tree, "linux");
    assert(result != NULL);
    assert(result->size == 1);
    
    PostingEntry* entry = (PostingEntry*)getVectorItem(result, 0);
    assert(entry->doc_id == 101);
    
    printf("Тест 2 пройден\n");
}


// Тест 3: обработка дубликатов
void test_3_duplicates(BTree* tree) 
{
    btreeInsert(tree, "linux", 102, "Advanced");
    btreeInsert(tree, "linux", 102, "Advanced Clone");

    Vector* result = btreeSearch(tree, "linux");
    assert(result != NULL);
    assert(result->size == 2);
    assert(tree->root->n == 1); 
    
    printf("Тест 3 пройден\n");
}


// Тест 4: сплит корня 
void test_4_split(BTree* tree)
{
    btreeInsert(tree, "apple", 10, "doc");
    btreeInsert(tree, "banana", 11, "doc");
    btreeInsert(tree, "cherry", 12, "doc");
    btreeInsert(tree, "date", 13, "doc");
    btreeInsert(tree, "elderberry", 14, "doc");

    assert(tree->root->is_leaf == 0); 
    
    
    assert(btreeSearch(tree, "apple") != NULL);
    assert(btreeSearch(tree, "elderberry") != NULL);
    assert(btreeSearch(tree, "cherry") != NULL);
    
    printf("Тест 4 пройден\n");
}


// Тест 5: поиск несуществующих слов
void test_5_not_found(BTree* tree) 
{
    assert(btreeSearch(tree, "zebra") == NULL);
    assert(btreeSearch(tree, "albatross") == NULL);
    
    printf("Тест 5 пройден\n");
}


// Тест 6: симметричный обход
void test_6_traverse(BTree* tree) 
{
    traverse_count = 0;
    btreeTraverse(tree, assert_visit, NULL);
    
    assert(traverse_count == 6); 
    
    printf("Тест 6 пройден\n");
}


// Тест 7: очистка
void test_7_free(BTree* tree) 
{
    freeBTree(tree);
    
    printf("Тест 7 пройден\n");
}


int main() 
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif


    BTree* tree = test_1_init();
    
    test_2_single_insert(tree);
    test_3_duplicates(tree);
    test_4_split(tree);
    test_5_not_found(tree);
    test_6_traverse(tree);
    
    test_7_free(tree);

    printf("\n=== Все тесты успешно пройдены ===\n");
    return 0;
}