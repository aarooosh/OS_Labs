all:memspan
memspan:main.o memspan.o
	gcc main.o memspan.o -o memspan
memspan.o:memspan.c
	gcc -c memspan.c
clean:
	rm -f memspan memspan.o
