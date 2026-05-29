#!/bin/bash
# Останавливаем выполнение при любой ошибке
set -e 

echo "Запуск Trio-Search Engine..."

echo "Шаг 0: Запуск юнит-тестов..."
./test_avl
./test_rb
./test_btree
echo "Все тесты успешно пройдены!"
echo "----------------------------------------"

DATA_DIR="data"
RAW_CSV="$DATA_DIR/Questions.csv"
JSONL="$DATA_DIR/docs.jsonl"

if [ ! -f "$JSONL" ]; then
    if [ -f "$RAW_CSV" ]; then
        echo "Индексы не найдены. Запускаю первоначальную обработку..."
        
        echo "1) Нарезка датасета (500k строк)..."
        # Запускаем Python через uv run
        uv run python preprocess.py --input "$RAW_CSV" --output "$JSONL" --limit 500000

        echo "2) Построение индексов (AVL, RB, B-Tree)..."
        ./app index --type=avl --data="$JSONL" --index="$DATA_DIR/index_avl.txt"
        ./app index --type=rb --data="$JSONL" --index="$DATA_DIR/index_rb.txt"
        ./app index --type=btree --data="$JSONL" --index="$DATA_DIR/index_btree.txt"

        echo "Индексация успешно завершена!"
    else
        echo "Ошибка: Исходный файл $RAW_CSV не найден!"
        echo "Пожалуйста, положи Questions.csv в папку data и перезапусти контейнер."
        exit 1
    fi
else
    echo "Готовые индексы найдены, этап сборки пропущен."
fi

echo "Запуск веб-интерфейса Streamlit..."
# Запускаем Streamlit через uv run
exec uv run streamlit run app.py --server.port=8501 --server.address=0.0.0.0