all: example1 example2

example1: bt.o example1.o
	gcc bt.o example1.o -o example1 -g -ggdb3
example1.o: example1.c bt.c
	gcc example1.c -c -g -ggdb3
example2: bt.o example2.o
	gcc bt.o example2.o -o example2 -g -ggdb3
example2.o: example2.c bt.c
	gcc example2.c -c -g -ggdb3
bt.o: bt.c
	gcc bt.c -c -g -ggdb3

clean:
	rm -f *.o
	rm -f example1 example2
	rm -f core.*

example:
	@echo "========== compiling examples =========="
	make clean && make
	@echo
	@echo "========== running example 1 =========="
	@./example1
	@echo
	@echo "========== running example 2 =========="
	@./example2

