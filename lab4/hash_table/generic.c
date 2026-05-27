#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../../lab3/vector/generic.h"
#include "generic.h"

//3 вариант Свободная адресация с линейным исследованием + хеш-функция методом деления

// Получить указатель на флаг состояния слота
static unsigned char* getSlotFlag(HashTable* table, size_t index) {
    return (unsigned char*)(table->values->data + index * table->values->elem_size);
}

// Получить указатель на ключ в слоте
static void* getSlotKey(HashTable* table, size_t index) {
    return table->values->data + index * table->values->elem_size + 1;
}

// Получить указатель на значение в слоте
static void* getSlotValue(HashTable* table, size_t index) {
    return table->values->data + index * table->values->elem_size + 1 + table->key_size;
}

// Проверить, занят ли слот
static bool isSlotOccupied(HashTable* table, size_t index) {
    return *getSlotFlag(table, index) == SLOT_OCCUPIED;
}

int HashInt(const void *key)
{
    return *(int *)key;
}

int HashString(const void *key)
{
    const char *str = (const char *)key;
    int hash = 0;

    while (*str)
    {
        hash = hash * 31 + *str;
        str++;
    }

    return hash;
}

unsigned long long HashUInt64(const void *key) {
    unsigned long long id = *(const unsigned long long *)key;
    return id * 17; 
}

HashTable *createHashTable(size_t key_size, size_t val_size)
{
    HashTable *ht = malloc(sizeof(HashTable));
    if (!ht) return NULL;

    ht->capacity = TABLE_MIN_SIZE;
    ht->key_size = key_size;
    ht->val_size = val_size;
    ht->size = 0;

    ht->values = createVector(1 + key_size + val_size);
    if (!ht->values) {
        free(ht);
        return NULL;
    }

    size_t slot_size = ht->values->elem_size;
    unsigned char slot[slot_size];
    memset(slot, 0, slot_size); 

    for (size_t i = 0; i < ht->capacity; i++) {
        if (appendVectorItem(ht->values, slot) != 0) {
            vectorFree(ht->values);
            free(ht);
            return NULL;
        }
    }

    return ht;
}

void setItemHashTable(HashTable *table, void *key, void *data, HashFunc hash, CmpFunc cmp)
{
    if (!table || !key || !data || !hash || !cmp) return;

    if (table->capacity < table->size * 2)
        rehashHashTable(table, hash, cmp);

    size_t index = hash(key) % table->capacity;
    size_t first_deleted = (size_t)-1;

    for (size_t i = 0; i < table->capacity; i++) {
        unsigned char state = *getSlotFlag(table, index);

        if (state == SLOT_EMPTY) {
            if (first_deleted != (size_t)-1)
                index = first_deleted;

            *getSlotFlag(table, index) = SLOT_OCCUPIED;
            memcpy(getSlotKey(table, index), key, table->key_size);
            memcpy(getSlotValue(table, index), data, table->val_size);
            table->size++;
            return;
        }

        if (state == SLOT_DELETED) {
            if (first_deleted == (size_t)-1)
                first_deleted = index;
        }
        else if (cmp(getSlotKey(table, index), key) == 0) {
            memcpy(getSlotValue(table, index), data, table->val_size);
            return;
        }

        index = (index + 1) % table->capacity;
    }
}


void rehashHashTable(HashTable *table, HashFunc hash, CmpFunc cmp)
{
    size_t old_capacity = table->capacity;
    Vector *old_values = table->values;

    Vector *new_values = createVector(old_values->elem_size);
    if (new_values == NULL) return;

    table->capacity = old_capacity * 2;
    table->values = new_values;
    table->size = 0;

    char empty_slot[new_values->elem_size];
    memset(empty_slot, SLOT_EMPTY, new_values->elem_size);

    for (size_t i = 0; i < table->capacity; i++) {
        appendVectorItem(new_values, empty_slot);
    }

    for (size_t i = 0; i < old_capacity; ++i) {
        if (*((char *)old_values->data +
              i * old_values->elem_size) == SLOT_OCCUPIED) {

            void *key = (char *)old_values->data +
                        i * old_values->elem_size + 1;
            void *value = (char *)old_values->data +
                          i * old_values->elem_size + 1 + table->key_size;

            setItemHashTable(table, key, value, hash, cmp);
        }
    }

    vectorFree(old_values);
}


void *getItemHashTable(HashTable *table, void *key, HashFunc hash, CmpFunc cmp)
{
    if (!table || !key || !hash || !cmp) return NULL;

    size_t index = hash(key) % table->capacity;

    for (size_t i = 0; i < table->capacity; i++) {
        unsigned char state = *getSlotFlag(table, index);

        if (state == SLOT_EMPTY) {
            return NULL;
        }

        if (state == SLOT_OCCUPIED && cmp(key, getSlotKey(table, index)) == 0) {
            return getSlotValue(table, index);
        }

        index = (index + 1) % table->capacity;
    }

    return NULL;
}

void *popItemHashTable(HashTable *table, void *key, HashFunc hash, CmpFunc cmp)
{
    if (!table || !key || !hash || !cmp) return NULL;

    size_t index = hash(key) % table->capacity;

    for (size_t i = 0; i < table->capacity; i++) {
        unsigned char state = *getSlotFlag(table, index);

        if (state == SLOT_EMPTY) {
            return NULL;
        }

        if (state == SLOT_OCCUPIED && cmp(key, getSlotKey(table, index)) == 0) {

            void *value = getSlotValue(table, index);

            *getSlotFlag(table, index) = SLOT_DELETED;
            table->size--;

            return value;
        }

        index = (index + 1) % table->capacity;
    }

    return NULL;
}

unsigned long int getCollisionCount(HashTable *table, HashFunc hash)
{
    unsigned long int count = 0;
    for (size_t index = 0; index < table->capacity; index++) {
        if (isSlotOccupied(table, index)) {
            if (hash(getSlotKey(table, index)) % table->capacity != index) count++;
        }
    }
    return count;
}

void freeHashTable(HashTable *table)
{
    if (table == NULL) return;

    vectorFree(table->values);
    free(table);
}
