#include <string.h>
#include "posting.h"

Vector* createPostingList(void) 
{
    // создаем вектор для хранения записей
    return createVector(sizeof(PostingEntry));
}

void appendPosting(Vector* list, int doc_id, const char* title) 
{
    if (list == NULL) 
    {
        return;
    }


    // отсекаем дубликаты
    if (list->size > 0) 
    {
        PostingEntry* last_entry = (PostingEntry*)getVectorItem(list, list->size - 1);
        

        if (last_entry->doc_id == doc_id) 
        {
            return; 
        }
    }

    // добавление записи в вектор
    PostingEntry entry;
    entry.doc_id = doc_id;
    strncpy(entry.title, title, MAX_TITLE_LEN - 1);
    entry.title[MAX_TITLE_LEN - 1] = '\0';
    appendVectorItem(list, &entry);
}

Vector* clonePostingList(const Vector* src) {
    if (src == NULL) 
    {
        return NULL;
    }


    Vector* clone = createPostingList();


    for (size_t i = 0; i < src->size; i++)
    {
        appendVectorItem(clone, getVectorItem((Vector*)src, i));
    }


    return clone;
}
