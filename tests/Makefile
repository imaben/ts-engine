CC = gcc
OPT = -Wno-implicit-function-declaration -Wno-format -Wno-pointer-sign -D _DEBUG -D_XOPEN_SOURCE=500 -I../ -g -std=gnu9x
all:
	$(CC) $(OPT) document_test.c ../document.c -o document_test
	$(CC) $(OPT) inverted_test.c ../document.c ../docid.c -o inverted_test
	$(CC) $(OPT) segment_test.c ../segment.c ../document.c ../inverted.c ../docid.c ../lock.c ../query.c ../intersect.c -o segment_test -lpthread
	$(CC) $(OPT) bitmap_test.c -o bitmap_test
	$(CC) $(OPT) docid_test.c ../docid.c -o docid_test
	$(CC) $(OPT) intersect_test.c ../docid.c ../intersect.c -o intersect_test
	$(CC) $(OPT) queue_test.c ../queue.c ../lock.c -o queue_test -lpthread
	$(CC) $(OPT) engine_test.c ../queue.c ../lock.c ../engine.c ../document.c ../segment.c ../inverted.c ../docid.c ../intersect.c ../merger.c -o engine_test -lpthread
	$(CC) $(OPT) threadpool_test.c ../threadpool.c ../queue.c ../lock.c -o threadpool_test -lpthread
	$(CC) $(OPT) protobuf_test.c ../document.c ../protocol/protocol.pb-c.c -o protobuf_test `pkg-config --cflags --libs libprotobuf-c`

clean:
	rm -rf ./*_test
	rm -rf ./*.dSYM
