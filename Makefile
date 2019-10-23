CC = gcc
OPT = -Wno-implicit-function-declaration -Wno-format -Wno-pointer-sign -D _DEBUG -g -std=gnu9x -lpthread `pkg-config --cflags --libs libprotobuf-c`

all:
	$(CC) $(OPT) ts.c server.c threadpool.c engine.c merger.c segment.c docid.c inverted.c query.c queue.c lock.c document.c intersect.c adapter.c net.c ae/ae.c ae/anet.c protocol/protocol.pb-c.c -o tiny-searcher

clean:
	rm -rf tiny-searcher
	rm -rf tiny-searcher.dSYM
