#pragma once

#include "../posting.h"

typedef struct AVLNode {
    const char*     key;
    int             height;
    Vector*         postings;
    struct AVLNode* left;
    struct AVLNode* right;
} AVLNode;

typedef struct {
    AVLNode* root;
    int      size;
} AVLTree;

AVLNode* createAVLNode(const char* key);
AVLTree* createAVLTree(void);
void     freeAVLNode(AVLNode* node);
void     freeAVLTree(AVLTree* tree);

int      avlGetHeight(AVLNode* node);
int      avlGetBalance(AVLNode* node);

void           avlInsert(AVLTree* tree, const char* key, int doc_id, const char* title);
const Vector*  avlSearch(const AVLTree* tree, const char* key);
void     avlTraverse(
    const AVLTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
);
