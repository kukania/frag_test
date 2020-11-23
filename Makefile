all: frag_test

frag_test: fragmentation.c
	g++ -O3 -o frag_test fragmentation.c

clean:
	$(RM) frag_test
