UPDATE_CTAGS := $(shell ctags -R .)

test: refcy.c
	$(CC) $(CFLAGS) $< -o $@ -D_TEST -g

run:
	./test

clean:
	rm test
