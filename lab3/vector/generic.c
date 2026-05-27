#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../macros.h"
#include "generic.h"

// Определяем увеличивать размер или уменьшать
static bool needToResize(Vector *vector, bool *increase)
{
    if (vector == NULL || increase == NULL) return false;

    if (vector->size >= vector->capacity){
        *increase = true;
        return true;
    }
    
    if (vector->size <= vector->capacity/4 && vector->capacity > MIN_SIZE){
        *increase = false;
        return true;
    }

    return false;
}

// Вспомогательная функция для изменения размера
static int resize(Vector *vector, bool increase)
{
    if (vector == NULL) return -1;

    if (increase == true) {
        vector->data = realloc(vector->data, (vector->capacity * 2) * vector->elem_size);
        if (vector->data == NULL) return -1;
        vector->capacity = vector->capacity * 2;
    }
    
    else {
        vector->data = realloc(vector->data, (vector->capacity / 2) * vector->elem_size);
        if (vector->data == NULL) return -1;
        vector->capacity = vector->capacity / 2;
    }

    return 0;
}

Vector *createVector(size_t elem_size)
{
    Vector *vector = malloc(sizeof(Vector));
    if (vector == NULL) {
        RETURN_ERROR("malloc error", NULL);
    }

    vector->elem_size = elem_size;
    vector->size = 0;
    vector->capacity = MIN_SIZE;
    vector->data = NULL;

    void* data = malloc(vector->capacity * elem_size);
    if (data == NULL) {
        free(vector);
        return NULL;
    }
    vector->data = data;

    return vector;
}

int appendVectorItem(Vector *vector, void *el)
{
    bool increase;
    if (needToResize(vector, &increase)){
        if(resize(vector, increase) != 0) return -1;
    }
    memcpy((char*)vector->data + vector->size * vector->elem_size, el, vector->elem_size);
    vector->size++;
    return 0;
}

void *getVectorItem(Vector *vector, size_t index)
{
    if (vector == NULL || index >= vector->size){
        return NULL;
    }

    void* el = (char*)vector->data + index * vector->elem_size;
    return el;
}

int setVectorItem(Vector *vector, size_t index, void *value)
{
    if (vector == NULL || index >= vector->size || value == NULL){
        return -1;
    }

    void* el = (char*)vector->data + index * vector->elem_size;
    memcpy(el, value, vector->elem_size);
    return 0;
}

void *popVectorItem(Vector *vector, size_t index)
{
    if (vector == NULL || index >= vector->size){
        return NULL;
    }

    void* el = getVectorItem(vector, index);
    void* cpy = malloc(vector->elem_size);
    if (cpy == NULL){
        free(cpy);
        return NULL;
    }
    memcpy(cpy, el, vector->elem_size);
    memmove(el, (char*)el + vector->elem_size, (vector->size - index - 1) * vector->elem_size);

    vector->size -= 1;

    return cpy;
}

long int findVectorItem(Vector *vector, void *value, EqualsFunc cmp)
{
    if (vector == NULL) return -1;
    for(size_t i = 0; i < vector->size; i++){
        void* el = getVectorItem(vector, i);
        if (cmp(el, value)) return i;
    }

    return -1;
}

int vectorFree(Vector *vector)
{
    if (vector == NULL) return 0;
    free(vector->data);
    free(vector);
    return 0;
}
