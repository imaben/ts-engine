AUTOMAKE_OPTIONS = subdir-objects
OPTS=-Wno-implicit-function-declaration -Wno-format -Wno-pointer-sign
AM_CFLAGS=`pkg-config --cflags libprotobuf-c` $(OPTS)
AM_LDFLAGS=`pkg-config --libs libprotobuf-c`
bin_PROGRAMS=ts-engine
ts_engine_SOURCES=ts.c server.c threadpool.c engine.c merger.c segment.c docid.c inverted.c query.c queue.c lock.c document.c intersect.c adapter.c net.c ae/ae.c ae/anet.c protocol/protocol.pb-c.c
