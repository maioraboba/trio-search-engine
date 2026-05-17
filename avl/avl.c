#include <stdio.h>
#include "avl.h"

static int maxHeight(int h1, int h2){
    return (h1 > h2) ? h1 : h2;
}

AVLNode* createAVLNode(const char* key){
    AVLNode *avl_node = malloc(sizeof(AVLNode));
    if (avl_node == NULL){
        printf("error malloc AVLNode");
        exit(EXIT_FAILURE);
    }

    avl_node->height = 0;
    avl_node->key = strdup(key);
    avl_node->postings = createPostingList();
    avl_node->left = NULL;
    avl_node->right = NULL;

    return avl_node;
}

AVLTree* createAVLTree(void){
    AVLTree *avl_tree = malloc(sizeof(AVLTree));
    if (avl_tree == NULL){
        printf("error malloc AVLTree");
        exit(EXIT_FAILURE);
    }

    avl_tree->size = 0;
    avl_tree->root = NULL;

    return avl_tree;
}

void freeAVLNode(AVLNode* node){
    if (node == NULL) return;

    freeAVLNode(node->left); // рекурсия в левую ветку
    freeAVLNode(node->right); // рекурсия в правую ветку
    free(node->key);
    vectorFree(node->postings);
    free(node);
}

void freeAVLTree(AVLTree* tree){
    if (tree == NULL) return;

    freeAVLNode(tree->root); // запуск рекурсии вниз по вершинам
    free(tree);
}

int avlGetHeight(AVLNode* node){
    return node == NULL ? -1 : node->height;
}

static void avlUpdateHeight(AVLNode* node){
    node->height = maxHeight(avlGetHeight(node->left), avlGetHeight(node->right)) + 1;
}

int avlGetBalance(AVLNode* node){
    return node == NULL ? 0 : avlGetHeight(node->right) - avlGetHeight(node->left);
}

static AVLNode* rotateLeft(AVLNode* x){
    AVLNode* y = x->right;
    x->right = y->left;
    y->left = x;

    avlUpdateHeight(x);
    avlUpdateHeight(y);

    return y;
}

static AVLNode* rotateRight(AVLNode* y){
    AVLNode* x = y->left;
    y->left = x->right;
    x->right = y;

    avlUpdateHeight(y);
    avlUpdateHeight(x);

    return x;
}

static AVLNode* avlBalance(AVLNode* node){
    avlUpdateHeight(node);
    int balance = avlGetBalance(node);

    if (balance > 1){
        if (avlGetBalance(node->right) < 0)
            node->right = rotateRight(node->right);
        return rotateLeft(node);
    }
    if (balance < -1){
        if (avlGetBalance(node->left) > 0)
            node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    return node;
}

static AVLNode* avlNodeInsert(AVLNode* node, const char* key, int doc_id, const char* title, int* tree_size){
    if (node == NULL){
        AVLNode *new_node = createAVLNode(key);
        (*tree_size)++; // увеличиваем размер дерева
        appendPosting(new_node->postings, doc_id, title); // обновляем posting на новой вершине

        return new_node;
    }

    int difference = strcmp(key, node->key);
    if (difference == 0){ // если ключи совпадают то добавляем в posting
        appendPosting(node->postings, doc_id, title);
    } else if (difference > 0){ // если искомый ключ ниже по алфавитному порядку то идем вправое ребро
        node->right = avlNodeInsert(node->right, key, doc_id, title, tree_size);
    } else { // иначе в левое ребро
        node->left = avlNodeInsert(node->left, key, doc_id, title, tree_size);
    }

    return avlBalance(node);
}

void avlInsert(AVLTree* tree, const char* key, int doc_id, const char* title){
    if (tree == NULL) return;
    
    tree->root = avlNodeInsert(tree->root, key, doc_id, title, &tree->size);
}

Vector* avlSearch(const AVLTree* tree, const char* key){
    if (tree == NULL) return NULL;

    AVLNode *node = tree->root;
    while (node != NULL){
        int diff = strcmp(key, node->key);

        if (diff == 0) return node->postings;
        else if (diff > 0)  node = node->right;
        else                node = node->left;
    }
    return NULL;
}

static void avlNodeTraverse(AVLNode* node, void (*visit)(const char*, Vector*, void*), void* ctx){
    if (node == NULL) return;

    // симметричный обход dfs
    avlNodeTraverse(node->left, visit, ctx);
    visit(node->key, node->postings, ctx);
    avlNodeTraverse(node->right, visit, ctx);
}

void avlTraverse(
    const AVLTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
){
    if (tree == NULL) return;
    avlNodeTraverse(tree->root, visit, ctx);
}
