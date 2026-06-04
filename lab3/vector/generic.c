#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
        void *new_data = realloc(vector->data, (vector->capacity * 2) * vector->elem_size);
        if (new_data == NULL) {
            return -1;
        }
        vector->data = new_data;
        vector->capacity *= 2;
    }
    
    else {
        void *new_data = realloc(vector->data, (vector->capacity / 2) * vector->elem_size);
        if (new_data == NULL) {
            return -1;
        }
        vector->data = new_data;
        vector->capacity /= 2;
    }

    return 0;
}

Vector *createVector(size_t elem_size)
{
    Vector *vector = malloc(sizeof(Vector));
    if (vector == NULL) {
        fprintf(stderr, "malloc error");
        return NULL;
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

int resizeVector(Vector *vector, size_t new_capacity) {
    if (vector == NULL) {
        fprintf(stderr, "Vector is NULL!");
        return -1;
    }

    void *new_data = realloc(vector->data, new_capacity * vector->elem_size);

    if (!new_data) {
        fprintf(stderr, "Memory allocation failed!");
        return -1;
    }

    vector->capacity = new_capacity;
    vector->data = new_data;

    return 0;
}

int reserveVector(Vector *vector, size_t new_capacity) {
    if (new_capacity > vector->capacity) {
        return resizeVector(vector, new_capacity);
    }
    return 0;
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

void *getVectorItem(const Vector *vector, size_t index)
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
        return NULL;
    }
    memcpy(cpy, el, vector->elem_size);
    memmove(el, (char*)el + vector->elem_size, (vector->size - index - 1) * vector->elem_size);

    vector->size -= 1;

    bool increase;
    if (needToResize(vector, &increase)) {
        resize(vector, increase);
    }

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
