CC=clang
OBJECTS=main.o render.o input.o editor.o mf_string.o
CFLAGS=-Wall -DMF_BUILD_TESTS

mf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o mf
	./mf --test

.PHONY: clean
clean:
	rm -f mf
	rm -f *.o
