#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "index/index.h"
#include "index/search.h"

#define MAX_LINE_LEN 8192
#define MAX_TOKENS 1024


/**
 * @brief Конвертирует строковое представление типа дерева в enum TreeType.
 *
 * @param str Строка с аргументом типа ("avl", "rb", "btree").
 * @return TreeType Перечисление типа дерева. По умолчанию возвращает TREE_AVL.
 */
static TreeType parseType(const char* str) {
    if (strcmp(str, "rb") == 0) return TREE_RB;
    if (strcmp(str, "btree") == 0) return TREE_BTREE;
    return TREE_AVL;
}

/**
 * @brief Возвращает строковое имя для перечисления TreeType (для имен файлов).
 *
 * @param type Перечисление типа дерева.
 * @return const char* Строковое имя.
 */
static const char* typeName(TreeType type) {
    switch (type) {
        case TREE_RB: return "rb";
        case TREE_BTREE: return "btree";
        case TREE_AVL:
        default: return "avl";
    }
}

/**
 * @brief Читает JSONL датасет, строит индекс и сохраняет его на диск.
 *
 * @param type Тип дерева для индексации.
 * @param data_path Путь к JSONL датасету.
 * @param idx_path Путь для сохранения готового индекса.
 */
static void runIndex(TreeType type, const char* data_path, const char* idx_path) {
    FILE* in = fopen(data_path, "r");
    if (!in) {
        fprintf(stderr, "Error: failed to open dataset file %s\n", data_path);
        exit(1);
    }

    Index* idx = createIndex(type);
    if (!idx) {
        fprintf(stderr, "Error: failed to allocate memory for index\n");
        fclose(in);
        exit(1);
    }

    char line[MAX_LINE_LEN];
    int count = 0;
    clock_t start = clock();

    fprintf(stderr, "Starting indexing from %s...\n", data_path);

    // построчное чтение JSONL
    while (fgets(line, sizeof(line), in)) {
        char* doc_id_ptr = strstr(line, "\"doc_id\":");
        char* title_ptr = strstr(line, "\"title\":");
        char* tokens_ptr = strstr(line, "\"tokens\":");

        if (!doc_id_ptr || !title_ptr || !tokens_ptr) continue;

        // извлекаем doc_id (ищем первое вхождение цифры)
        int doc_id = 0;
        doc_id_ptr += 9;
        while (*doc_id_ptr == ' ' || *doc_id_ptr == '"') doc_id_ptr++;
        doc_id = atoi(doc_id_ptr);

        // извлекаем title
        char title[MAX_TITLE_LEN] = {0};
        title_ptr += 8;
        while (*title_ptr == ' ') title_ptr++;
        if (*title_ptr == '"') {
            title_ptr++; // пропускаем открывающую кавычку
            char* title_end = strchr(title_ptr, '"');
            if (title_end) {
                size_t len = title_end - title_ptr;
                if (len >= MAX_TITLE_LEN) len = MAX_TITLE_LEN - 1;
                strncpy(title, title_ptr, len);
            }
        }

        // извлекаем токены в массив строк
        const char* tokens[MAX_TOKENS];
        int n_tokens = 0;
        tokens_ptr += 9;
        char* array_start = strchr(tokens_ptr, '[');
        char* array_end = strchr(tokens_ptr, ']');

        if (array_start && array_end) {
            *array_end = '\0'; // обрубаем строку на конце массива
            char* token_str = array_start + 1;
            char* saveptr;
            // делим строку, используя кавычки, запятые и пробелы как разделители
            char* tok = strtok_r(token_str, "\", []", &saveptr);
            
            while (tok && n_tokens < MAX_TOKENS) {
                tokens[n_tokens++] = tok;
                tok = strtok_r(NULL, "\", []", &saveptr);
            }
        }

        // добавляем документ в дерево
        indexDocument(idx, doc_id, title, tokens, n_tokens);
        count++;

        if (count % 50000 == 0) {
            fprintf(stderr, "Indexed: %d documents...\n", count);
        }
    }

    fclose(in);

    clock_t end = clock();
    double ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    fprintf(stderr, "Indexing is complete. Documents: %d. Time: %.1f ms\n", count, ms);

    fprintf(stderr, "Saving index in %s...\n", idx_path);
    saveIndex(idx, idx_path);
    freeIndex(idx);
    fprintf(stderr, "Successfully.\n");
}

/**
 * @brief Загружает дамп индекса с диска и выполняет поиск.
 *
 * @param type Тип дерева (AVL, RB, BTree).
 * @param idx_path Путь к файлу дампа.
 * @param query Поисковый запрос.
 * @param json_out Флаг вывода результатов (1 - JSON stdout, 0 - текст stdout).
 */
static void runSearch(TreeType type, const char* idx_path, const char* query, int json_out) {
    clock_t t0 = clock();
    Index* idx = loadIndex(idx_path, type);
    clock_t t1 = clock();

    if (!idx) {
        // ошибки выводим строго в stderr, чтобы не сломать JSON-парсер Streamlit'а
        fprintf(stderr, "Erros: can't load index from %s\n", idx_path);
        exit(1);
    }

    if (!json_out) {
        fprintf(stderr, "Index is loaded for %.1f ms\n", ((double)(t1 - t0)) / CLOCKS_PER_SEC * 1000.0);
    }

    SearchResults* sr = search(idx, query);

    if (json_out) {
        printResultsJSON(sr); // streamlit прочитает это из stdout
    } else {
        printResultsText(sr);
    }

    freeSearchResults(sr);
    freeIndex(idx);
}

static void usage(const char* prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s index  --type=<avl|rb|btree> [--data=PATH] [--index=PATH]\n"
        "  %s search --type=<avl|rb|btree> [--index=PATH] [--json] \"query\"\n",
        prog, prog);
}

int main(int argc, char* argv[]) {
    if (argc < 3) { 
        usage(argv[0]); 
        return 1; 
    }

    const char* mode = argv[1];
    TreeType    type = TREE_AVL;
    const char* data_path = "data/processed/docs.jsonl";
    char        idx_path[512] = {0};
    int         json_out = 0;
    const char* query    = NULL;

    for (int i = 2; i < argc; i++) {
        if      (strncmp(argv[i], "--type=",  7) == 0) type = parseType(argv[i] + 7);
        else if (strncmp(argv[i], "--data=",  7) == 0) data_path = argv[i] + 7;
        else if (strncmp(argv[i], "--index=", 8) == 0) strncpy(idx_path, argv[i] + 8, sizeof(idx_path) - 1);
        else if (strcmp(argv[i], "--json")    == 0) json_out = 1;
        else if (argv[i][0] != '-')                 query = argv[i];
    }

    if (idx_path[0] == '\0') {
        snprintf(idx_path, sizeof(idx_path), "data/index_%s.txt", typeName(type));
    }

    if (strcmp(mode, "index") == 0) {
        runIndex(type, data_path, idx_path);
    } else if (strcmp(mode, "search") == 0) {
        if (!query) { 
            fprintf(stderr, "Error: missing search query\n"); 
            return 1; 
        }
        runSearch(type, idx_path, query, json_out);
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        usage(argv[0]);
        return 1;
    }
    
    return 0;
}