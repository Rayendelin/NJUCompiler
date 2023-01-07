parser: clean
	gcc -std=c99 -g -c -o optimizer.o optimizer.c
	gcc -std=c99 -g -c -o main.o main.c
	gcc -g -o parser ./optimizer.o ./main.o

.PHONY: test clean
test: clean parser
	./parser ../Test/self_test.ir out.ir

clean:
	rm -f main.o optimizer.o parser