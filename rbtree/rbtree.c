// Полностью на ваше усмотрение (только переиспользуйте код из предыдущих лабораторных, если он вам подходит)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"

static RBNode* createRBNode(RBTree* tree, const char* key){
    RBNode* node = malloc(sizeof(RBNode));
    if (node == NULL){
        printf("error malloc RBNode");
        return NULL;
    }

    node->key = strdup(key);
    node->color = RB_RED; // новый узел всегда красный
    node->postings = createPostingList();
    node->left = tree->nil; // листья sentinel nil, не NULL
    node->right = tree->nil;
    node->parent = tree->nil;

    return node;
}

RBTree* createRBTree(void){
    RBTree* tree = malloc(sizeof(RBTree));
    if (tree == NULL){
        printf("error malloc RBTree");
        return NULL;
    }

    // все листья дерева указывают на sentinel узел, он всегда чёрный
    tree->nil = malloc(sizeof(RBNode));
    if (tree->nil == NULL) {
        printf("error malloc RBTree sentinel");
        free(tree);
        return NULL;
    }

    tree->nil->color = RB_BLACK;
    tree->nil->key = NULL;
    tree->nil->postings = NULL;
    tree->nil->left = tree->nil;
    tree->nil->right = tree->nil;
    tree->nil->parent = tree->nil;

    tree->root = tree->nil; // у пустого дерева корень тоже nil
    tree->size = 0;
    return tree;
}

static void freeRBNode(RBTree* tree, RBNode* node){
    if (node == tree->nil) return;

    freeRBNode(tree, node->left);
    freeRBNode(tree, node->right);
    free((void*)node->key);
    vectorFree(node->postings);
    free(node);
}

void freeRBTree(RBTree* tree){
    if (tree == NULL) return;
    
    freeRBNode(tree, tree->root);
    free(tree->nil);
    free(tree);
}

static void rotateLeft(RBTree* tree, RBNode* node){
    RBNode* right_child = node->right;

    node->right = right_child->left;
    if (right_child->left != tree->nil){
        right_child->left->parent = node;
    }

    right_child->parent = node->parent;
    if (node->parent == tree->nil){
        tree->root = right_child;
    } else if (node == node->parent->left){
        node->parent->left = right_child;
    } else {
        node->parent->right = right_child;
    }

    right_child->left = node;
    node->parent = right_child;
}

static void rotateRight(RBTree* tree, RBNode* node){
    RBNode* left_child = node->left;

    node->left = left_child->right;
    if (left_child->right != tree->nil){
        left_child->right->parent = node;
    }

    left_child->parent = node->parent;
    if (node->parent == tree->nil){
        tree->root = left_child;
    } else if (node == node->parent->left){
        node->parent->left = left_child;
    } else {
        node->parent->right = left_child;
    }

    left_child->right = node;
    node->parent = left_child;
}

static void rbInsertFixup(RBTree* tree, RBNode* node){
    while (node->parent->color == RB_RED){
        if (node->parent == node->parent->parent->left){
            RBNode* uncle = node->parent->parent->right;

            if (uncle->color == RB_RED){ // если дядя красный, то перекрашиваем и идём вверх
                node->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                node = node->parent->parent;
            } else {
                if (node == node->parent->right){ // треугольник, который при повороте становится линией
                    node = node->parent;
                    rotateLeft(tree, node);
                }
                // получается линия поворачиваем и перекрашиваем
                node->parent->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                rotateRight(tree, node->parent->parent);
            }
        } else {
            RBNode* uncle = node->parent->parent->left; // симметричный случай

            if (uncle->color == RB_RED){
                node->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                node = node->parent->parent;
            } else {
                if (node == node->parent->left){
                    node = node->parent;
                    rotateRight(tree, node);
                }
                node->parent->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                rotateLeft(tree, node->parent->parent);
            }
        }
    }

    tree->root->color = RB_BLACK;
}

void rbInsert(RBTree* tree, const char* key, int doc_id, const char* title){
    if (tree == NULL) return;

    RBNode* parent = tree->nil; // будущий родитель нового узла
    RBNode* current = tree->root;

    while (current != tree->nil){ // ищем место вставки или дубликат
        parent = current;
        int cmp = strcmp(key, current->key);

        if (cmp == 0){
            appendPosting(current->postings, doc_id, title);
            return;
        } else if (cmp < 0){
            current = current->left;
        } else {
            current = current->right;
        }
    }

    RBNode* new_node = createRBNode(tree, key);
    appendPosting(new_node->postings, doc_id, title);
    new_node->parent = parent;

    if (parent == tree->nil){
        tree->root = new_node;
    } else if (strcmp(key, parent->key) < 0){
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }

    tree->size++;
    rbInsertFixup(tree, new_node);
}

const Vector* rbSearch(const RBTree* tree, const char* key){
    if (tree == NULL) return NULL;

    RBNode* node = tree->root;
    while (node != tree->nil){
        int diff = strcmp(key, node->key);

        if (diff == 0) return node->postings;
        else if (diff < 0) node = node->left;
        else node = node->right;
    }

    return NULL;
}

static void rbNodeTraverse(const RBTree* tree, RBNode* node, void (*visit)(const char*, Vector*, void*), void* ctx){
    if (node == tree->nil) return;
    
    // симметричный обход dfs
    rbNodeTraverse(tree, node->left, visit, ctx);
    visit(node->key, node->postings, ctx);
    rbNodeTraverse(tree, node->right, visit, ctx);
}

void rbTraverse(
    const RBTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
){
    if (tree == NULL) return;
    rbNodeTraverse(tree, tree->root, visit, ctx);
}
