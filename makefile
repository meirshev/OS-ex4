CC = g++
CFLAGS =  -pthread -std=c++11 -DNDEBUG -Wall  -D_FILE_OFFSET_BITS=64
ARG =  CachingFileSystem /tmp/rootdir /tmp/mountdir 3 0.5 0.5

all : main

main : CachingFileSystem.o CacheManager.o
	${CC} ${CFLAGS} `pkg-config fuse --cflags --libs` CachingFileSystem.o CacheManager.o -o CachingFileSystem


CachingFileSystem.o : CachingFileSystem.cpp
	${CC} ${CFLAGS} CachingFileSystem.cpp -c


CacheManager.o : CacheManager.cpp
	${CC} ${CFLAGS} CacheManager.cpp -c

clean :
	rm -f *.o CachingFileSystem

valgrind : main
	valgrind --leak-check=full --show-possibly-lost=yes --show-reachable=yes --undef-value-errors=yes  ${ARG2}

tar:
	tar -cvf ex4.tar makefile README uthreads.cpp
