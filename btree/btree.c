#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btree.h"


// функция для выделения памяти под одинз узел
static BTreeNode* createBTreeNode(int is_leaf) 
{
    // выделяем память под структуру
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    if (node == NULL)
     {
        printf("Не удалось выделить память под узел\n");
        return NULL;
    }

    // инициализируем поля
    node->is_leaf = is_leaf;
    node->n = 0; 


    for (int i = 0; i < BTREE_MAX_KEYS; i++) 
    {
        node->keys[i] = NULL;
        node->postings[i] = NULL;
    }

    // детей всегда на 1 больше, чем ключей
    for (int i = 0; i < BTREE_MAX_CH; i++) 
    {
        node->children[i] = NULL;
    }

    return node;
}


BTree* createBTree(void) 
{
    // выделяем память под дерево
    BTree* tree = malloc(sizeof(BTree));
    if (tree == NULL) {
        printf("Не удалось выделить память под дерево\n");
        return NULL;
    }

    // инициализируем поля
    tree->size = 0;

    // создаем корень, детей нет, лист 
    tree->root = createBTreeNode(1);

    return tree;
}


static void freeBTreeNode(BTreeNode* node) 
{
    if (node == NULL) 
    {
        return;
    }


    // удаляем детей (их на 1 больше чем ключей)
    if (node->is_leaf == 0) 
    {
        for (int i = 0; i <= node->n; i++) 
        {
            freeBTreeNode(node->children[i]);
        }
    }

    // очищаем ключии и векторы 
    for (int i = 0; i < node->n; i++) 
    {
        if (node->keys[i] != NULL) 
        {
            free((void*)node->keys[i]);
        }
        
        // очищаем Posting List
        if (node->postings[i] != NULL) 
        {
            vectorFree(node->postings[i]);
        }
    }

    // чистим структуру узла 
    free(node);
}


void freeBTree(BTree* tree) 
{
    if (tree == NULL)
    {
        return;
    }
    

    if (tree->root != NULL) 
    {
        freeBTreeNode(tree->root);
    }

    // чистим саму структуру
    free(tree);
}


static const Vector* btreeNodeSearch(BTreeNode* node, const char* key)
{
    if (node == NULL)
    {
        return NULL;
    }


    // поиск внутри узла 
    int i = 0; 
    while (i < node->n && strcmp(key, node->keys[i]) > 0)
    {
        i++;
    }


    // проверяем на совпадение 
    if (i < node->n && strcmp(key, node->keys[i]) == 0)
    {
        return node->postings[i];
    }


    // если лист - выходим 
    if (node->is_leaf == 1)
    {
        return NULL;
    }

    // рекурсивный спуск
    return btreeNodeSearch(node->children[i], key);
}


const Vector* btreeSearch(const BTree* tree, const char* key)
{
    if (tree == NULL || tree->root == NULL)
    {
        return NULL;
    }


    return btreeNodeSearch(tree->root, key);
}


static void btreeSplitChild(BTreeNode* parent, int index, BTreeNode* child)
{
    // создаем новый узел 
    BTreeNode* new_child = createBTreeNode(child->is_leaf);
    new_child->n = BTREE_T - 1;


    // переносим правую половину ключуй и векторов в новый узел (3, 4) -> (0, 1)
    for (int j = 0; j < BTREE_T - 1; j++) 
    {
        new_child->keys[j] = child->keys[j + BTREE_T];
        new_child->postings[j] = child->postings[j + BTREE_T];
        child->keys[j + BTREE_T] = NULL;
        child->postings[j + BTREE_T] = NULL;
    }

    // переносим детей 3, 4, 5 -> 0, 1, 2
    if (child->is_leaf == 0) 
    {
        for (int j = 0; j < BTREE_T; j++) 
        {
            new_child->children[j] = child->children[j + BTREE_T];
            child->children[j + BTREE_T] = NULL;
        }
    }


    // обрезаем старый узел
    child->n = BTREE_T - 1;

    
    // освобождаем место под нового ребенка
    // сдвигаем указатели на детей в родителе вправо
    for (int j = parent->n; j >= index + 1; j--) 
    {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[index + 1] = new_child;


    // сдвигаем ключи и векторы в parent на одну ячейку вправо
    for (int j = parent->n - 1; j >= index; j--) 
    {
        parent->keys[j + 1] = parent->keys[j];
        parent->postings[j + 1] = parent->postings[j];
    }

    
    // вставляем центральный ключ из старого ребенка на освободившееся место
    parent->keys[index] = child->keys[BTREE_T - 1];
    parent->postings[index] = child->postings[BTREE_T - 1];
    child->keys[BTREE_T - 1] = NULL;
    child->postings[BTREE_T - 1] = NULL;

    
    // увеличиваем счетчик ключей в родителе
    parent->n++;
}


static void btreeInsertNonFull(BTreeNode* node, const char* key, int doc_id, const char* title, int* tree_size) {
    // проверяем существует ли такое слово в узле
    for (int j = 0; j < node->n; j++) 
    {
        // если слово найдено - докидываем документ в его вектор
        if (strcmp(key, node->keys[j]) == 0)
        {
            appendPosting(node->postings[j], doc_id, title);
            return;
        }
    }

    int index = node->n - 1;  // последний элемент в массиве


    // если лист 
    if (node->is_leaf) 
    {
        // двигаем ключи и векторы вправо
        while (index >= 0 && strcmp(key, node->keys[index]) < 0) 
        {
            node->keys[index + 1] = node->keys[index];
            node->postings[index + 1] = node->postings[index];
            index--;
        }


        // вставляем новое слово и создаем его вектора
        node->keys[index + 1] = strdup(key); 
        node->postings[index + 1] = createPostingList();
        appendPosting(node->postings[index + 1], doc_id, title);
        
        (*tree_size)++;

        node->n++; 
    } 


    // внутренний узел
    else 
    {
        // ищем индекс ребенка
        while (index >= 0 && strcmp(key, node->keys[index]) < 0) 
        {
            index--;
        }
        index++; 


        // если узел забит - вызываем сплит
        if (node->children[index]->n == BTREE_MAX_KEYS) 
        {
            btreeSplitChild(node, index, node->children[index]);


            // проверяем медиану на дубликат и добавляем айдишник документа 
            if (strcmp(key, node->keys[index]) == 0) 
            {
                appendPosting(node->postings[index], doc_id, title);
                return;
            }


            // если слово больше медианы - идем в нового правого ребенка
            if (strcmp(key, node->keys[index]) > 0) 
            {
                index++;
            }
        }

        // рекурсивно вызываем для спуска дальше
        btreeInsertNonFull(node->children[index], key, doc_id, title, tree_size);
    }
}


void btreeInsert(BTree* tree, const char* key, int doc_id, const char* title) 
{
    if (tree == NULL || tree->root == NULL) 
    {
        return;
    }


    BTreeNode* root = tree->root;

    // если в корне 5 ключей - создаем новый корень
    if (root->n == BTREE_MAX_KEYS) 
    {
        BTreeNode* new_root = createBTreeNode(0); 
        tree->root = new_root; 
        new_root->children[0] = root;

        
        // сплитуем старый корень и запускаем вставку
        btreeSplitChild(new_root, 0, root);
        btreeInsertNonFull(new_root, key, doc_id, title, &tree->size);
    } 
    
    
    // если корень не забит - запускаем вставку 
    else 
    {
        btreeInsertNonFull(root, key, doc_id, title, &tree->size);
    }
}


static void btreeNodeTraverse(BTreeNode* node, void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) 
{
    if (node == NULL) 
    {
        return;
    }


    int index;


    // идем по всем ключам узла
    for (index = 0; index < node->n; index++) 
    {
        // cначала спускаемся в левого ребенка
        if (node->is_leaf == 0) 
        {
            btreeNodeTraverse(node->children[index], visit, ctx);
        }
        

        // обрабатываем текущий ключ
        visit(node->keys[index], node->postings[index], ctx);
    }

    // обрабатываем самого правого ребенка
    if (node->is_leaf == 0) 
    {
        btreeNodeTraverse(node->children[index], visit, ctx);
    }
}


void btreeTraverse(const BTree* tree, void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) 
{
    if (tree == NULL || tree->root == NULL || visit == NULL) 
    {
        return;
    }


    // запускаем рекурсию от корня
    btreeNodeTraverse(tree->root, visit, ctx);
}