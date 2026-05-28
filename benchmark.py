import subprocess
import time
import os
import json
from pathlib import Path


BINARY = "./app.exe"
INDEX_DIR = Path("data/indexes")
INDEX_DIR.mkdir(parents=True, exist_ok=True)

DATASETS = {
    "Small": "data/processed/docs_small.jsonl",
    "Medium": "data/processed/docs_medium.jsonl",
    "Large": "data/processed/docs_large.jsonl"
}

QUERIES = [
    "python",               # очень популярное слово
    "array",                # популярное слово
    "string sorting",       # 2 слова
    "how to list sort",     # 4 слова
    "unexistentword123"     # слово, которого точно нет
]

BACKENDS = ["avl", "rb", "btree"]

def get_file_size_mb(path: Path) -> float:
    if not path.exists():
        return 0.0
    return os.path.getsize(path) / (1024 * 1024)

def run_benchmarks():
    if not os.path.exists(BINARY):
        print(f"Ошибка: нету app.py")
        return

    results = {ds_name: {b: {} for b in BACKENDS} for ds_name in DATASETS.keys()}

    for ds_name, data_path in DATASETS.items():
        print(f"\n{'='*70}")
        print(f"ЗАПУСК БЕНЧМАРКОВ ДЛЯ ДАТАСЕТА: {ds_name.upper()} ({data_path})")
        print(f"{'='*70}")

        print("\n--- ЭТАП 1: ИНДЕКСАЦИЯ ---")
        for backend in BACKENDS:
            idx_path = INDEX_DIR / f"index_{ds_name}_{backend}.txt"
            print(f"[{backend.upper()}] Индексация...")
            
            proc = subprocess.run(
                [BINARY, "index", f"--type={backend}", f"--data={data_path}", f"--index={idx_path}"],
                capture_output=True, text=True
            )
            
            build_time = 0.0
            for line in proc.stderr.splitlines():
                if "Time:" in line or "Время:" in line:
                    build_time = float(line.split()[-2])
            
            results[ds_name][backend]["build_time_ms"] = build_time
            results[ds_name][backend]["file_size_mb"] = get_file_size_mb(idx_path)
            print(f"  -> Время: {build_time:.1f} мс | Размер: {results[ds_name][backend]['file_size_mb']:.2f} MB")

        print("\n--- ЭТАП 2: ПОИСК ---")
        for backend in BACKENDS:
            idx_path = INDEX_DIR / f"index_{ds_name}_{backend}.txt"
            results[ds_name][backend]["search_times"] = {}
            print(f"[{backend.upper()}] Поиск...")

            for q in QUERIES:
                iterations = 5
                total_time = 0.0
                
                for _ in range(iterations):
                    proc_json = subprocess.run(
                        [BINARY, "search", f"--type={backend}", f"--index={idx_path}", "--json", q],
                        capture_output=True, text=True
                    )
                    try:
                        data = json.loads(proc_json.stdout)
                        total_time += data.get("time_ms", 0.0)
                    except:
                        pass
                
                avg_time = total_time / iterations
                results[ds_name][backend]["search_times"][q] = avg_time
                print(f"  -> '{q}': {avg_time:.3f} мс")

    # вывод финального отчета
    print("\n" + "="*50)
    print("ФИНАЛЬНЫЙ ОТЧЕТ ПО ВСЕМ ДАТАСЕТАМ")
    print("="*50)
    
    for ds_name in DATASETS.keys():
        print(f"\nДАТАСЕТ: {ds_name.upper()}")
        print(f"{'Структура':<12} | {'Сборка (мс)':<12} | {'Размер (MB)':<12} | {'Запрос [python]':<16} | {'Запрос [how to list sort]':<26}")
        print("-" * 85)
        for b in BACKENDS:
            print(f"{b.upper():<12} | "
                  f"{results[ds_name][b]['build_time_ms']:<12.1f} | "
                  f"{results[ds_name][b]['file_size_mb']:<12.2f} | "
                  f"{results[ds_name][b]['search_times']['python']:<16.3f} | "
                  f"{results[ds_name][b]['search_times']['how to list sort']:<26.3f}")

if __name__ == "__main__":
    run_benchmarks()