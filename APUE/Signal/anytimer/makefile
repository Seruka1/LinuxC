all:anytimer
CFLAGS += -lstdc++ -Wall -g

mytbf:main.c anytimer.o
	gcc $^ -o $@

mytbf.o:anytimer.c
	gcc $^ -c -o $@
clean:
	rm -rf *.o anytimer