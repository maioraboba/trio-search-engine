#include "index.h"
#include <stdlib.h>
#include <stdio.h>
#include "../avl/avl.h"
#include "../rbtree/rbtree.h"
#include "../btree/btree.h"

Index* createIndex(TreeType type) {
    Index* idx = (Index*)malloc(sizeof(Index));

    if (!idx) {
        return NULL;
    }

    idx->type = type;

    switch(type){
        case TREE_AVL:
            idx->tree = createAVLTree();
            break;
        
        case TREE_RB:
            idx->tree = createRBTree();
            break;
        
        case TREE_BTREE:
            idx->tree = createBTree();
            break;

        default:
            idx->tree = NULL;
            break;
    }

    return idx;
}

void freeIndex(Index* idx) {

    if (idx == NULL) return;

    switch(idx->type){
        case TREE_AVL:
            freeAVLTree((TREE_AVL*) idx->tree);
            break;

        case TREE_RB:
            freeRBTree((TREE_RB) idx->tree);
            break;

        case TREE_BTREE:
            freeBTree((TREE_BTREE*) idx->tree);
            break;
    }
 
    free(idx);
}

void insertTerm(Index* idx, const char* term, int doc_id, const char* title) {

    if (!idx) return;

    switch (idx->type){
        case TREE_AVL:
        avlInsert((TREE_AVL*) idx->tree, term, doc_id, title);
        break;

        case TREE_RB:
        rbInsert((TREE_RB*) idx->tree, term, doc_id, title);
        break;

        case TREE_BTREE:
        btreeInsert((TREE_BTREE*) idx->tree, term, doc_id, title);
        break;
    }
}

void indexDocument(Index* idx, int doc_id, const char* title, const char** tokens, int n_tokens) {
    if (!idx || !tokens) return;

    for (int i = 0; i < n_tokens; i++) {
        insertTerm(idx, tokens[i], doc_id, title);
    }
}

void traverseIndex(
    const Index* idx,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
) {
    if (!idx || !visit) return;

    switch (idx->type) {
        case TREE_AVL:
            avlTraverse((AVLTree*)idx->tree, visit, ctx);
            break;

        case TREE_RB:
            rbTraverse((RBTree*)idx->tree, visit, ctx);
            break;

        case TREE_BTREE:
            btreeTraverse((BTree*)idx->tree, visit, ctx);
            break;
    }
}

static void writePostingListToFile(const char* key, Vector* postings, void* ctx) {
    FILE* out = (FILE*)ctx;

    fprintf(out, "%s\t%zu\n", key, postings->size);

    for (size_t i = 0; i < postings->size; i++) {
        PostingEntry* entry = (PostingEntry*)getVectorItem(postings, i);
        
        fprintf(out, "%d\t%s\n", entry->doc_id, entry->title);
    }
}

void saveIndex(const Index* idx, const char* path) {
    if (!idx || !path) return;

    FILE* out = fopen(path,"w");
    if (!out) return;
    traverseIndex(idx, writePostingListToFile, out);
    fclose(out);
}

Index* loadIndex(const char* path, TreeType type) {
    if (!path) return NULL;

    FILE* in = fopen(path, "r");
    if (!in) {
        return NULL;
    }

    Index* idx = createIndex(type);
    if (!idx) {
        fclose(in);
        return NULL;
    }

    char term[256];
    size_t count;

    while (fscanf(in, "%255s\t%zu\n", term, &count) == 2) {
        for (size_t i = 0; i < count; i++) {
            int doc_id;
            char title[256];
            if (fscanf(in, "%d\t%255[^\n]\n", &doc_id, title) == 2) {
                insertTerm(idx, term, doc_id, title);
            }
        }
    }

    fclose(in);
    return idx;
}