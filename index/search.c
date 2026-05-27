#include "search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Безопасное глубокое копирование (защита от двойного освобождения)
static Vector* safeCloneList(Vector* src) {
    if (!src) return createPostingList();
    Vector* copy = createPostingList();
    for (size_t i = 0; i < src->size; i++) {
        PostingEntry* e = (PostingEntry*)getVectorItem(src, i);
        appendPosting(copy, e->doc_id, e->title);
    }
    return copy;
}

// Вспомогательная функция для безопасного экранирования JSON-строк
static void printEscapedJSONString(const char* str) {
    while (*str) {
        if (*str == '"' || *str == '\\') {
            putchar('\\');
            putchar(*str);
        } else if (*str == '\n') {
            printf("\\n");
        } else if (*str == '\t') {
            printf("\\t");
        } else if (*str >= 0x00 && *str <= 0x1F) {
            // Игнорируем или заменяем непечатные символы
            putchar(' ');
        } else {
            putchar(*str);
        }
        str++;
    }
}

static Vector* intersectTwoPostings(Vector* list1, Vector* list2) {
    if (!list1 || !list2) return createPostingList();

    Vector* result = createPostingList();
    size_t i = 0, j = 0;

    // Алгоритм двух указателей: O(N + M)
    while (i < list1->size && j < list2->size) {
        PostingEntry* e1 = (PostingEntry*)getVectorItem(list1, i);
        PostingEntry* e2 = (PostingEntry*)getVectorItem(list2, j);

        if (e1->doc_id == e2->doc_id) {
            appendPosting(result, e1->doc_id, e1->title);
            i++; j++;
        } else if (e1->doc_id < e2->doc_id) {
            i++;
        } else {
            j++;
        }
    }
    return result;
}

Vector* intersectPostings(Vector** lists, int n) {
    if (n == 0) return NULL;
    if (n == 1) return safeCloneList(lists[0]);

    Vector* current_result = intersectTwoPostings(lists[0], lists[1]);

    for (int i = 2; i < n; i++) {
        Vector* next_result = intersectTwoPostings(current_result, lists[i]);
        vectorFree(current_result); 
        current_result = next_result;
        
        if (current_result->size == 0) break; 
    }
    return current_result;
}


SearchResults* search(Index* idx, const char* query) {
    SearchResults* sr = malloc(sizeof(SearchResults));
    if (!sr) return NULL;

    // Финальный вектор хранит SearchResult, а не PostingEntry
    sr->results = createVector(sizeof(SearchResult)); 
    sr->total = 0;
    sr->time_ms = 0.0;

    if (!idx || !query || strlen(query) == 0) return sr;

    clock_t start_time = clock();

    char query_copy[512];
    strncpy(query_copy, query, sizeof(query_copy) - 1);
    query_copy[sizeof(query_copy) - 1] = '\0';

    char* tokens[32];
    int n_tokens = 0;
    char* token = strtok(query_copy, " \t\n");
    while (token && n_tokens < 32) {
        tokens[n_tokens++] = token;
        token = strtok(NULL, " \t\n");
    }

    if (n_tokens == 0) {
        clock_t end_time = clock();
        sr->time_ms = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0;
        return sr;
    }

    Vector** lists = malloc(n_tokens * sizeof(Vector*));
    int valid_lists = 0;

    for (int i = 0; i < n_tokens; i++) {
        Vector* list = lookupTerm(idx, tokens[i]);
        if (list) {
            lists[valid_lists++] = list;
        } else {
            free(lists);
            clock_t end_time = clock();
            sr->time_ms = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0;
            return sr;
        }
    }

    Vector* intersection = intersectPostings(lists, valid_lists);
    free(lists);

    if (intersection) {
        sr->total = intersection->size;
        size_t limit = (intersection->size < 10) ? intersection->size : 10;
        
        for (size_t i = 0; i < limit; i++) {
            PostingEntry* entry = (PostingEntry*)getVectorItem(intersection, i);
            
            // Перекладываем данные из PostingEntry в SearchResult
            SearchResult res;
            res.doc_id = entry->doc_id;
            strncpy(res.title, entry->title, MAX_TITLE_LEN - 1);
            res.title[MAX_TITLE_LEN - 1] = '\0';
            res.score = n_tokens; // Базовый скор: количество совпавших слов
            
            appendVectorItem(sr->results, &res);
        }
        vectorFree(intersection);
    }

    clock_t end_time = clock();
    sr->time_ms = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0;

    return sr;
}


void printResultsText(const SearchResults* sr) {
    if (!sr) {
        printf("Ошибка: пустой результат поиска.\n");
        return;
    }

    printf("Время: %.2f мс | Найдено: %d документов\n\n", sr->time_ms, sr->total);

    for (size_t i = 0; i < sr->results->size; i++) {
        // Теперь мы кастуем к SearchResult*, так как внутри вектора лежат именно они
        SearchResult* res = (SearchResult*)getVectorItem(sr->results, i);
        printf(" %2zu. [id=%d] %s\n", i + 1, res->doc_id, res->title);
    }
}

void printResultsJSON(const SearchResults* sr) {
    if (!sr) {
        printf("{\"error\": \"Invalid search results\"}\n");
        return;
    }

    printf("{\n");
    printf("  \"time_ms\": %.2f,\n", sr->time_ms);
    printf("  \"total\": %d,\n", sr->total);
    printf("  \"results\": [\n");

    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* res = (SearchResult*)getVectorItem(sr->results, i);
        printf("    {\n");
        printf("      \"doc_id\": %d,\n", res->doc_id);
        
        printf("      \"title\": \"");
        printEscapedJSONString(res->title); // <--- Вот здесь безопасный вывод
        printf("\",\n");
        
        printf("      \"score\": %d\n", res->score);
        printf("    }%s\n", (i < sr->results->size - 1) ? "," : "");
    }

    printf("  ]\n");
    printf("}\n");
}

void freeSearchResults(SearchResults* sr) {
    if (!sr) return;
    
    if (sr->results) {
        vectorFree(sr->results);
    }
    free(sr);
}