# Используем легковесный образ Python
FROM dockerhub.timeweb.cloud/library/python:3.12-slim

# Ставим компилятор GCC и утилиту Make для сборки сишного кода
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    dos2unix \
    && rm -rf /var/lib/apt/lists/*

# Устанавливаем современный менеджер пакетов uv
RUN pip install --no-cache-dir uv

# Задаем рабочую директорию внутри контейнера
WORKDIR /app

# Копируем весь исходный код проекта внутрь контейнера
COPY . .

# Синхронизируем зависимости (uv сам всё скачает строго по лок-файлу)
RUN uv sync --frozen

# Компилируем основной движок и бинарники тестов
RUN make app test_avl test_rb test_btree

# Фиксим переносы строк для bash-скрипта и даем ему права на исполнение
RUN dos2unix start.sh && chmod +x start.sh

# Открываем порт для веб-интерфейса Streamlit
EXPOSE 8501

# Команда, которая запускает сервер при старте контейнера
CMD ["./start.sh"]