CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g -D_POSIX_C_SOURCE=200809L

ifeq ($(OS),Windows_NT)
    EXE = .exe
    PYTHON = python
    MKDIR_TEST = if not exist data\test mkdir data\test
else
    EXE =
    PYTHON = python3
    MKDIR_TEST = mkdir -p data/test
endif

APP = app$(EXE)
TEST_AVL = test_avl$(EXE)
TEST_RB = test_rb$(EXE)
TEST_BTREE = test_btree$(EXE)

OBJ_SHARED = posting.o avl/avl.o rbtree/rbtree.o btree/btree.o \
             index/index.o index/search.o lab3/vector/generic.o

.PHONY: all app u_tests test clean

all: app u_tests

app: $(OBJ_SHARED) main.o
	$(CC) $(CFLAGS) -o $(APP) $(OBJ_SHARED) main.o

test_avl: posting.o avl/avl.o avl/tests.o lab3/vector/generic.o
	$(CC) $(CFLAGS) -o $(TEST_AVL) posting.o avl/avl.o avl/tests.o lab3/vector/generic.o

test_rb: posting.o rbtree/rbtree.o rbtree/tests.o lab3/vector/generic.o
	$(CC) $(CFLAGS) -o $(TEST_RB) posting.o rbtree/rbtree.o rbtree/tests.o lab3/vector/generic.o

test_btree: posting.o btree/btree.o btree/tests.o lab3/vector/generic.o
	$(CC) $(CFLAGS) -o $(TEST_BTREE) posting.o btree/btree.o btree/tests.o lab3/vector/generic.o

u_tests: test_avl test_rb test_btree
	./$(TEST_AVL)
	./$(TEST_RB)
	./$(TEST_BTREE)

test: app
	@echo "=== E2E: preprocessing ==="
	$(MKDIR_TEST)
	$(PYTHON) preprocess.py \
		--input  data/Questions.csv \
		--output data/test/docs.jsonl \
		--limit  1000
	@echo "=== E2E: indexing ==="
	./$(APP) index --type=avl   --data=data/test/docs.jsonl --index=data/test/idx_avl.txt
	./$(APP) index --type=rb    --data=data/test/docs.jsonl --index=data/test/idx_rb.txt
	./$(APP) index --type=btree --data=data/test/docs.jsonl --index=data/test/idx_btree.txt
	@echo "=== E2E: searching ==="
	./$(APP) search --type=avl   --index=data/test/idx_avl.txt   --json "python list"
	./$(APP) search --type=rb    --index=data/test/idx_rb.txt    --json "python list"
	./$(APP) search --type=btree --index=data/test/idx_btree.txt --json "python list"
	@echo "=== E2E OK ==="


%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
ifeq ($(OS),Windows_NT)
	-del /Q /F app.exe test_avl.exe test_rb.exe test_btree.exe 2>nul
	-del /Q /F *.o avl\*.o rbtree\*.o btree\*.o index\*.o lab3\vector\*.o 2>nul
	-del /Q /F data\index_*.txt data\test\docs.jsonl data\test\idx_*.txt 2>nul
else
	rm -f $(APP) $(TEST_AVL) $(TEST_RB) $(TEST_BTREE)
	rm -f *.o avl/*.o rbtree/*.o btree/*.o index/*.o lab3/vector/*.o
	rm -f data/index_*.txt data/test/docs.jsonl data/test/idx_*.txt
endif
