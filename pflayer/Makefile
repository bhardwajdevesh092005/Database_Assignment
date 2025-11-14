CC=gcc -std=c89 -Wno-implicit-function-declaration
SRC= buf.c hash.c pf.c
OBJ= buf.o hash.o pf.o
HDR = pftypes.h pf.h 

pflayer.o: $(OBJ)
	ld -r -o pflayer.o $(OBJ)

tests: testhash testpf test_replacement test_lru_mru

testpf: testpf.o pflayer.o
	$(CC) -o testpf testpf.o pflayer.o

test_replacement: test_replacement.o pflayer.o
	$(CC) -o test_replacement test_replacement.o pflayer.o

test_lru_mru: test_lru_mru.o pflayer.o
	$(CC) -o test_lru_mru test_lru_mru.o pflayer.o

testhash: testhash.o pflayer.o
	$(CC) -o testhash testhash.o pflayer.o

$(OBJ): $(HDR)

testhash.o: $(HDR)

testpf.o: $(HDR)

test_replacement.o: $(HDR)

test_lru_mru.o: $(HDR)

lint: 
	lint $(SRC)

install: pflayer.o 


clean:
	rm -f *.o pflayer.o testhash testpf test_replacement test_lru_mru